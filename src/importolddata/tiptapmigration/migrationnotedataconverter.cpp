// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationnotedataconverter.h"

#include "migrationjsonbuilder.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QStringList>

#include <cmath>
#include <limits>

namespace {
constexpr int kTextBlockType = 1;
constexpr int kVoiceBlockType = 2;

enum class ConversionMode {
    TextOnly,
    TextAndVoice,
};

void addError(MigrationNoteDataConversionResult &result,
              const QString &path,
              const QString &code,
              const QString &message)
{
    result.errors.append({ path, code, message });
}

void addWarning(MigrationNoteDataConversionResult &result,
                const QString &path,
                const QString &code,
                const QString &message)
{
    result.warnings.append({ path, code, message });
}

QString blockPath(int index)
{
    return QStringLiteral("noteDatas[%1]").arg(index);
}

QString normalizedNewlines(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

QJsonObject paragraphFromText(const QString &text)
{
    if (text.isEmpty()) {
        return MigrationJsonBuilder::makeParagraph();
    }

    QJsonArray content;
    QStringList lines = normalizedNewlines(text).split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    while (lines.size() > 1 && lines.constLast().isEmpty()) {
        lines.removeLast();
    }

    for (int index = 0; index < lines.size(); ++index) {
        if (!lines.at(index).isEmpty()) {
            content.append(MigrationJsonBuilder::makeText(lines.at(index)));
        }

        if (index != lines.size() - 1) {
            content.append(MigrationJsonBuilder::makeHardBreak());
        }
    }

    return MigrationJsonBuilder::makeParagraph(content);
}

QString stringField(const QJsonObject &block,
                    const QString &field,
                    const QString &path,
                    MigrationNoteDataConversionResult &result)
{
    const QJsonValue value = block.value(field);
    if (value.isUndefined() || value.isNull()) {
        return QString();
    }

    if (value.isString()) {
        return value.toString();
    }

    addWarning(result,
               path + QLatin1Char('.') + field,
               QStringLiteral("invalid-voice-string-field"),
               QStringLiteral("voice block string field was ignored because it is not a string"));
    return QString();
}

QString normalizedPathSeparators(QString path)
{
    path = path.trimmed();
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return path;
}

bool isUnsafePathSegment(const QString &segment)
{
    return segment.isEmpty()
        || segment == QStringLiteral(".")
        || segment == QStringLiteral("..")
        || segment.contains(QLatin1Char(':'));
}

bool hasUnsafeInputPathSegments(const QString &path)
{
    const QStringList segments = path.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (int index = 0; index < segments.size(); ++index) {
        if (index == 0 && segments.at(index).isEmpty() && path.startsWith(QLatin1Char('/'))) {
            continue;
        }

        if (isUnsafePathSegment(segments.at(index))) {
            return true;
        }
    }

    return false;
}

bool isSafeVoicenoteRelativePath(const QString &path)
{
    if (!path.startsWith(QStringLiteral("voicenote/")) || QDir::isAbsolutePath(path)) {
        return false;
    }

    const QStringList segments = path.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &segment : segments) {
        if (isUnsafePathSegment(segment)) {
            return false;
        }
    }

    return true;
}

void addInvalidVoicePathError(MigrationNoteDataConversionResult &result, const QString &path)
{
    addError(result,
             path + QStringLiteral(".voicePath"),
             QStringLiteral("invalid-voice-path"),
             QStringLiteral("voicePath could not be normalized to a safe voicenote relative path"));
}

QString normalizeVoicePath(const QJsonObject &block,
                           const QString &path,
                           MigrationNoteDataConversionResult &result,
                           bool *ok)
{
    *ok = false;
    const QJsonValue value = block.value(QStringLiteral("voicePath"));
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        addError(result,
                 path + QStringLiteral(".voicePath"),
                 QStringLiteral("missing-voice-path"),
                 QStringLiteral("voice noteDatas block requires a non-empty voicePath"));
        return QString();
    }

    const QString voicePath = normalizedPathSeparators(value.toString());
    const int voicenoteIndex = voicePath.indexOf(QStringLiteral("voicenote/"));
    if (voicenoteIndex >= 0) {
        const QString candidate = voicePath.mid(voicenoteIndex);
        if (!isSafeVoicenoteRelativePath(candidate)) {
            addInvalidVoicePathError(result, path);
            return QString();
        }

        *ok = true;
        return candidate;
    }

    if (hasUnsafeInputPathSegments(voicePath)) {
        addInvalidVoicePathError(result, path);
        return QString();
    }

    const QString fileName = QFileInfo(voicePath).fileName();
    if (fileName.isEmpty() || isUnsafePathSegment(fileName)) {
        addInvalidVoicePathError(result, path);
        return QString();
    }

    addWarning(result,
               path + QStringLiteral(".voicePath"),
               QStringLiteral("normalized-voice-path"),
               QStringLiteral("voicePath was normalized to voicenote/<fileName>"));
    *ok = true;
    return QStringLiteral("voicenote/") + fileName;
}

qint64 sanitizedVoiceSize(double size,
                          const QString &path,
                          MigrationNoteDataConversionResult &result)
{
    if (!std::isfinite(size)) {
        addWarning(result,
                   path + QStringLiteral(".voiceSize"),
                   QStringLiteral("invalid-voice-size"),
                   QStringLiteral("voiceSize is not finite and was converted to 0"));
        return 0;
    }

    if (size < 0) {
        addWarning(result,
                   path + QStringLiteral(".voiceSize"),
                   QStringLiteral("negative-voice-size"),
                   QStringLiteral("negative voiceSize was converted to 0"));
        return 0;
    }

    const double maxSize = static_cast<double>(std::numeric_limits<qint64>::max());
    if (size > maxSize) {
        addWarning(result,
                   path + QStringLiteral(".voiceSize"),
                   QStringLiteral("oversized-voice-size"),
                   QStringLiteral("voiceSize is too large and was clamped"));
        return std::numeric_limits<qint64>::max();
    }

    return static_cast<qint64>(size);
}

qint64 voiceSizeFromBlock(const QJsonObject &block, const QString &path, MigrationNoteDataConversionResult &result)
{
    const QJsonValue value = block.value(QStringLiteral("voiceSize"));
    if (value.isUndefined() || value.isNull()) {
        addWarning(result,
                   path + QStringLiteral(".voiceSize"),
                   QStringLiteral("missing-voice-size"),
                   QStringLiteral("missing voiceSize was converted to 0"));
        return 0;
    }

    if (value.isDouble()) {
        return sanitizedVoiceSize(value.toDouble(), path, result);
    }

    if (value.isString()) {
        bool parsed = false;
        const double size = value.toString().trimmed().toDouble(&parsed);
        if (parsed) {
            return sanitizedVoiceSize(size, path, result);
        }
    }

    addWarning(result,
               path + QStringLiteral(".voiceSize"),
               QStringLiteral("invalid-voice-size"),
               QStringLiteral("invalid voiceSize was converted to 0"));
    return 0;
}

QString generatedVoiceId(int index, const QString &voicePath, const QString &createTime, const QString &title)
{
    const QString seed = QStringLiteral("%1\n%2\n%3\n%4")
                             .arg(index)
                             .arg(voicePath, createTime, title);
    const QByteArray hash = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("legacy-voice-") + QString::fromLatin1(hash.left(16));
}

QString voiceIdFromBlock(const QJsonObject &block,
                         int index,
                         const QString &voicePath,
                         const QString &createTime,
                         const QString &title,
                         const QString &path,
                         MigrationNoteDataConversionResult &result)
{
    const QJsonValue value = block.value(QStringLiteral("voiceId"));
    if (value.isString() && !value.toString().trimmed().isEmpty()) {
        return value.toString().trimmed();
    }

    addWarning(result,
               path + QStringLiteral(".voiceId"),
               QStringLiteral("generated-voice-id"),
               QStringLiteral("voiceId was missing or invalid and a stable legacy id was generated"));
    return generatedVoiceId(index, voicePath, createTime, title);
}

QJsonObject voiceBlockFromBlock(const QJsonObject &block,
                                int index,
                                const QString &path,
                                MigrationNoteDataConversionResult &result,
                                bool *ok)
{
    *ok = false;

    bool voicePathOk = false;
    const QString voicePath = normalizeVoicePath(block, path, result, &voicePathOk);
    if (!voicePathOk) {
        return QJsonObject();
    }

    const QString createTime = stringField(block, QStringLiteral("createTime"), path, result);
    const QString title = stringField(block, QStringLiteral("title"), path, result);
    const QString text = stringField(block, QStringLiteral("text"), path, result);
    const qint64 voiceSize = voiceSizeFromBlock(block, path, result);
    const QString voiceId = voiceIdFromBlock(block, index, voicePath, createTime, title, path, result);

    *ok = true;
    return MigrationJsonBuilder::makeVoiceBlock(voiceId, voicePath, voiceSize, createTime, title, text);
}

MigrationNoteDataConversionResult convertNoteDataBlocks(const QJsonObject &metadata, ConversionMode mode)
{
    MigrationNoteDataConversionResult result;

    const QJsonValue noteDatasValue = metadata.value(QStringLiteral("noteDatas"));
    if (!noteDatasValue.isArray()) {
        addError(result,
                 QStringLiteral("noteDatas"),
                 QStringLiteral("invalid-note-datas"),
                 QStringLiteral("legacy noteDatas metadata requires a noteDatas array"));
        return result;
    }

    QJsonArray docContent;
    const QJsonArray noteDatas = noteDatasValue.toArray();
    for (int index = 0; index < noteDatas.size(); ++index) {
        const QString path = blockPath(index);
        const QJsonValue blockValue = noteDatas.at(index);
        if (!blockValue.isObject()) {
            addWarning(result,
                       path,
                       QStringLiteral("invalid-note-data-block"),
                       QStringLiteral("legacy noteDatas block must be an object and was skipped"));
            continue;
        }

        const QJsonObject block = blockValue.toObject();
        const QJsonValue typeValue = block.value(QStringLiteral("type"));
        if (!typeValue.isDouble()) {
            addWarning(result,
                       path,
                       QStringLiteral("skipped-non-text-block"),
                       QStringLiteral("non-text noteDatas block is reserved for later migration steps"));
            continue;
        }

        const int type = typeValue.toInt();
        if (type == kTextBlockType) {
            const QJsonValue textValue = block.value(QStringLiteral("text"));
            if (!textValue.isString()) {
                addWarning(result,
                           path + QStringLiteral(".text"),
                           QStringLiteral("missing-text"),
                           QStringLiteral("text noteDatas block has no string text and was converted as empty paragraph"));
            }

            docContent.append(paragraphFromText(textValue.toString()));
            continue;
        }

        if (mode == ConversionMode::TextAndVoice && type == kVoiceBlockType) {
            bool voiceBlockOk = false;
            const QJsonObject voiceBlock = voiceBlockFromBlock(block, index, path, result, &voiceBlockOk);
            if (voiceBlockOk) {
                docContent.append(voiceBlock);
            }
            continue;
        }

        addWarning(result,
                   path,
                   QStringLiteral("skipped-non-text-block"),
                   QStringLiteral("non-text noteDatas block is reserved for later migration steps"));
    }

    result.envelope = MigrationJsonBuilder::makeEnvelope(MigrationJsonBuilder::makeDoc(docContent));
    return result;
}

MigrationNoteDataConversionResult convertNoteDataBlocks(const QString &metadataJson, ConversionMode mode)
{
    MigrationNoteDataConversionResult result;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(metadataJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        addError(result,
                 QStringLiteral("/"),
                 QStringLiteral("invalid-json"),
                 QStringLiteral("legacy noteDatas metadata must be a JSON object"));
        return result;
    }

    return convertNoteDataBlocks(document.object(), mode);
}

} // namespace

bool MigrationNoteDataConversionResult::ok() const
{
    return errors.isEmpty();
}

MigrationNoteDataConversionResult MigrationNoteDataConverter::convertTextBlocks(const QString &metadataJson)
{
    return convertNoteDataBlocks(metadataJson, ConversionMode::TextOnly);
}

MigrationNoteDataConversionResult MigrationNoteDataConverter::convertTextBlocks(const QJsonObject &metadata)
{
    return convertNoteDataBlocks(metadata, ConversionMode::TextOnly);
}

MigrationNoteDataConversionResult MigrationNoteDataConverter::convertBlocks(const QString &metadataJson)
{
    return convertNoteDataBlocks(metadataJson, ConversionMode::TextAndVoice);
}

MigrationNoteDataConversionResult MigrationNoteDataConverter::convertBlocks(const QJsonObject &metadata)
{
    return convertNoteDataBlocks(metadata, ConversionMode::TextAndVoice);
}
