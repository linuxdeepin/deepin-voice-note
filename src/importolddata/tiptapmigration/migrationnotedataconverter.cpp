// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationnotedataconverter.h"

#include "migrationjsonbuilder.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QStringList>

namespace {
constexpr int kTextBlockType = 1;

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

} // namespace

bool MigrationNoteDataConversionResult::ok() const
{
    return errors.isEmpty();
}

MigrationNoteDataConversionResult MigrationNoteDataConverter::convertTextBlocks(const QString &metadataJson)
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

    return convertTextBlocks(document.object());
}

MigrationNoteDataConversionResult MigrationNoteDataConverter::convertTextBlocks(const QJsonObject &metadata)
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
        if (!typeValue.isDouble() || typeValue.toInt() != kTextBlockType) {
            addWarning(result,
                       path,
                       QStringLiteral("skipped-non-text-block"),
                       QStringLiteral("non-text noteDatas block is reserved for later migration steps"));
            continue;
        }

        const QJsonValue textValue = block.value(QStringLiteral("text"));
        if (!textValue.isString()) {
            addWarning(result,
                       path + QStringLiteral(".text"),
                       QStringLiteral("missing-text"),
                       QStringLiteral("text noteDatas block has no string text and was converted as empty paragraph"));
        }

        docContent.append(paragraphFromText(textValue.toString()));
    }

    result.envelope = MigrationJsonBuilder::makeEnvelope(MigrationJsonBuilder::makeDoc(docContent));
    return result;
}
