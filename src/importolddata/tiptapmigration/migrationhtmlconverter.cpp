// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationhtmlconverter.h"

#include "migrationhtmlparser.h"
#include "migrationjsonbuilder.h"

#include "migrationhtmlcodes.h"

#include <QJsonArray>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <cmath>

#include <limits>

namespace {

constexpr int kVoiceBlockType = 2;

const QSet<QString> &supportedStyleProperties()
{
    static const QSet<QString> properties {
        QStringLiteral("background-color"),
        QStringLiteral("color"),
        QStringLiteral("font-family"),
        QStringLiteral("font-size"),
        QStringLiteral("font-style"),
        QStringLiteral("font-weight"),
        QStringLiteral("text-align"),
        QStringLiteral("text-decoration"),
        QStringLiteral("text-decoration-line")
    };
    return properties;
}

void addIssue(QVector<MigrationHtmlConversionIssue> &issues,
              const QString &path,
              const QString &code,
              const QString &message)
{
    issues.append({ path, code, message });
}

void addWarning(MigrationHtmlConversionResult &result,
                const QString &path,
                const QString &code,
                const QString &message)
{
    addIssue(result.warnings, path, code, message);
}

QString elementPath(const MigrationHtmlNode &node)
{
    return QStringLiteral("/%1").arg(node.tagName.isEmpty() ? QStringLiteral("node") : node.tagName);
}

void copyParseIssues(const MigrationHtmlParseResult &parsed, MigrationHtmlConversionResult &result)
{
    for (const MigrationHtmlParseWarning &warning : parsed.warnings) {
        if (warning.code == kCodeParseFailed) {
            addIssue(result.errors, warning.path, warning.code, warning.message);
            continue;
        }

        addWarning(result, warning.path, warning.code, warning.message);
    }
}

bool isDangerousElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element && dangerousTags().contains(node.tagName);
}

bool isBlockElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element && blockTags().contains(node.tagName);
}

bool isDisplayAffectingInlineTag(const QString &tagName)
{
    static const QSet<QString> displayAffecting {
        QStringLiteral("mark"),
        QStringLiteral("sub"),
        QStringLiteral("sup"),
        QStringLiteral("small"),
        QStringLiteral("big"),
        QStringLiteral("code"),
        QStringLiteral("kbd"),
        QStringLiteral("samp"),
        QStringLiteral("tt"),
        QStringLiteral("var"),
        QStringLiteral("ins"),
        QStringLiteral("q"),
        QStringLiteral("bdi"),
        QStringLiteral("bdo"),
        QStringLiteral("abbr"),
        QStringLiteral("cite")
    };
    return displayAffecting.contains(tagName);
}

bool isHeadingElement(const MigrationHtmlNode &node)
{
    if (node.type != MigrationHtmlNodeType::Element || node.tagName.size() != 2 || node.tagName.at(0) != QLatin1Char('h')) {
        return false;
    }

    const QChar level = node.tagName.at(1);
    return level >= QLatin1Char('1') && level <= QLatin1Char('6');
}

bool isListElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element
        && (node.tagName == QStringLiteral("ul") || node.tagName == QStringLiteral("ol"));
}

bool isListItemElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element && node.tagName == QStringLiteral("li");
}

bool isVoiceBoxElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element
        && MigrationHtmlParser::hasClass(node, QStringLiteral("voiceBox"));
}

QString normalizedNewlines(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

QString normalizedVisibleText(const QString &text)
{
    QString normalized;
    bool pendingSpace = false;

    for (const QChar &character : normalizedNewlines(text)) {
        if (character.isSpace()) {
            pendingSpace = true;
            continue;
        }

        if (pendingSpace && !normalized.isEmpty()) {
            normalized.append(QLatin1Char(' '));
        }

        normalized.append(character);
        pendingSpace = false;
    }

    return normalized.trimmed();
}

QString visibleTextOf(const MigrationHtmlNode &node)
{
    if (node.type == MigrationHtmlNodeType::Text) {
        return node.text;
    }

    QString text;
    for (const MigrationHtmlNode &child : node.children) {
        if (isDangerousElement(child)) {
            continue;
        }
        const QString childText = visibleTextOf(child);
        if (!text.isEmpty() && !childText.isEmpty()) {
            text.append(QLatin1Char(' '));
        }
        text.append(childText);
    }

    return text;
}

QString translatedTextOf(const MigrationHtmlNode &node)
{
    if (node.type != MigrationHtmlNodeType::Element) {
        return QString();
    }

    if (MigrationHtmlParser::hasClass(node, QStringLiteral("translateText"))) {
        return visibleTextOf(node);
    }

    QString text;
    for (const MigrationHtmlNode &child : node.children) {
        const QString childText = translatedTextOf(child);
        if (!text.isEmpty() && !childText.isEmpty()) {
            text.append(QLatin1Char('\n'));
        }
        text.append(childText);
    }

    return text;
}


QString normalizedVoicePathSeparators(QString path)
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



void addVoiceBoxWarning(MigrationHtmlConversionResult &result,
                        const MigrationHtmlNode &node,
                        const QString &code,
                        const QString &message,
                        const QString &suffix = QString())
{
    addWarning(result,
               elementPath(node) + suffix,
               code,
               message);
}

QJsonObject voiceBoxFallbackParagraph(const MigrationHtmlNode &node,
                                      MigrationHtmlConversionResult &result,
                                      const QString &code,
                                      const QString &message,
                                      const QString &suffix = QStringLiteral(".jsonKey"))
{
    addVoiceBoxWarning(result, node, code, message, suffix);

    const QString fallbackText = normalizedVisibleText(visibleTextOf(node));
    if (fallbackText.isEmpty()) {
        return MigrationJsonBuilder::makeParagraph();
    }

    return MigrationJsonBuilder::makeParagraph(QJsonArray { MigrationJsonBuilder::makeText(fallbackText) });
}

QJsonObject parsedVoiceBoxJsonKey(const MigrationHtmlNode &node,
                                  MigrationHtmlConversionResult &result,
                                  bool *ok)
{
    *ok = false;
    const QString jsonKey = MigrationHtmlParser::attribute(node, QStringLiteral("jsonKey")).trimmed();
    if (jsonKey.isEmpty()) {
        return voiceBoxFallbackParagraph(node,
                                         result,
                                         kCodeMissingVoiceboxJsonKey,
                                         QStringLiteral("voiceBox jsonKey is missing; visible text was preserved as a paragraph"));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonKey.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return voiceBoxFallbackParagraph(node,
                                         result,
                                         kCodeInvalidVoiceboxJsonKey,
                                         QStringLiteral("voiceBox jsonKey could not be parsed; visible text was preserved as a paragraph"));
    }

    *ok = true;
    return document.object();
}

QString stringField(const QJsonObject &block,
                    const QString &field,
                    const MigrationHtmlNode &node,
                    MigrationHtmlConversionResult &result)
{
    const QJsonValue value = block.value(field);
    if (value.isUndefined() || value.isNull()) {
        return QString();
    }

    if (value.isString()) {
        return value.toString();
    }

    addVoiceBoxWarning(result,
                       node,
                       kCodeInvalidVoiceboxStringField,
                       QStringLiteral("voiceBox string field '%1' was ignored because it is not a string").arg(field),
                       QStringLiteral(".") + field);
    return QString();
}

QString normalizeVoicePath(const QJsonObject &block,
                           const MigrationHtmlNode &node,
                           MigrationHtmlConversionResult &result,
                           bool *ok)
{
    *ok = false;
    const QJsonValue value = block.value(QStringLiteral("voicePath"));
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        return QString();
    }

    const QString voicePath = normalizedVoicePathSeparators(value.toString());
    const int voicenoteIndex = voicePath.indexOf(QStringLiteral("voicenote/"));
    if (voicenoteIndex >= 0) {
        const QString candidate = voicePath.mid(voicenoteIndex);
        if (!isSafeVoicenoteRelativePath(candidate)) {
            return QString();
        }

        *ok = true;
        return candidate;
    }

    if (hasUnsafeInputPathSegments(voicePath)) {
        return QString();
    }

    const QString fileName = QFileInfo(voicePath).fileName();
    if (fileName.isEmpty() || isUnsafePathSegment(fileName)) {
        return QString();
    }

    addVoiceBoxWarning(result,
                       node,
                       QStringLiteral("normalized-voice-path"),
                       QStringLiteral("voicePath was normalized to voicenote/<fileName>"),
                       QStringLiteral(".voicePath"));
    *ok = true;
    return QStringLiteral("voicenote/") + fileName;
}

qint64 sanitizedVoiceSize(double size,
                          const MigrationHtmlNode &node,
                          MigrationHtmlConversionResult &result)
{
    if (!std::isfinite(size)) {
        addVoiceBoxWarning(result,
                           node,
                           kCodeInvalidVoiceSize,
                           QStringLiteral("voiceSize is not finite and was converted to 0"),
                           QStringLiteral(".voiceSize"));
        return 0;
    }

    if (size < 0) {
        addVoiceBoxWarning(result,
                           node,
                           QStringLiteral("negative-voice-size"),
                           QStringLiteral("negative voiceSize was converted to 0"),
                           QStringLiteral(".voiceSize"));
        return 0;
    }

    const double maxSize = static_cast<double>(std::numeric_limits<qint64>::max());
    if (size > maxSize) {
        addVoiceBoxWarning(result,
                           node,
                           QStringLiteral("oversized-voice-size"),
                           QStringLiteral("voiceSize is too large and was clamped"),
                           QStringLiteral(".voiceSize"));
        return std::numeric_limits<qint64>::max();
    }

    return static_cast<qint64>(size);
}

qint64 voiceSizeFromBlock(const QJsonObject &block,
                          const MigrationHtmlNode &node,
                          MigrationHtmlConversionResult &result)
{
    const QJsonValue value = block.value(QStringLiteral("voiceSize"));
    if (value.isUndefined() || value.isNull()) {
        addVoiceBoxWarning(result,
                           node,
                           kCodeMissingVoiceSize,
                           QStringLiteral("missing voiceSize was converted to 0"),
                           QStringLiteral(".voiceSize"));
        return 0;
    }

    if (value.isDouble()) {
        return sanitizedVoiceSize(value.toDouble(), node, result);
    }

    if (value.isString()) {
        bool parsed = false;
        const double size = value.toString().trimmed().toDouble(&parsed);
        if (parsed) {
            return sanitizedVoiceSize(size, node, result);
        }
    }

    addVoiceBoxWarning(result,
                       node,
                       kCodeInvalidVoiceSize,
                       QStringLiteral("invalid voiceSize was converted to 0"),
                       QStringLiteral(".voiceSize"));
    return 0;
}

QString generatedVoiceId(const MigrationHtmlNode &node,
                         const QString &voicePath,
                         const QString &createTime,
                         const QString &title)
{
    const QString seed = QStringLiteral("%1\n%2\n%3\n%4")
                             .arg(elementPath(node), voicePath, createTime, title);
    const QByteArray hash = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("legacy-voice-") + QString::fromLatin1(hash.left(16));
}

QString voiceIdFromBlock(const QJsonObject &block,
                         const MigrationHtmlNode &node,
                         const QString &voicePath,
                         const QString &createTime,
                         const QString &title,
                         MigrationHtmlConversionResult &result)
{
    const QJsonValue value = block.value(QStringLiteral("voiceId"));
    if (value.isString() && !value.toString().trimmed().isEmpty()) {
        return value.toString().trimmed();
    }

    addVoiceBoxWarning(result,
                       node,
                       QStringLiteral("generated-voice-id"),
                       QStringLiteral("voiceId was missing or invalid and a stable legacy id was generated"),
                       QStringLiteral(".voiceId"));
    return generatedVoiceId(node, voicePath, createTime, title);
}

bool translateUnfoldFromBlock(const QJsonObject &block)
{
    const QJsonValue value = block.value(QStringLiteral("translateUnfold"));
    return value.isBool() ? value.toBool() : true;
}

QJsonObject voiceBlockFromElement(const MigrationHtmlNode &node,
                                  MigrationHtmlConversionResult &result)
{
    bool jsonKeyOk = false;
    const QJsonObject block = parsedVoiceBoxJsonKey(node, result, &jsonKeyOk);
    if (!jsonKeyOk) {
        return block;
    }

    bool voicePathOk = false;
    const QString voicePath = normalizeVoicePath(block, node, result, &voicePathOk);
    if (!voicePathOk) {
        return voiceBoxFallbackParagraph(node,
                                         result,
                                         kCodeInvalidVoicePath,
                                         QStringLiteral("voiceBox voicePath could not be normalized; visible text was preserved as a paragraph"),
                                         QStringLiteral(".voicePath"));
    }

    if (block.value(QStringLiteral("type")).isDouble() && block.value(QStringLiteral("type")).toInt() != kVoiceBlockType) {
        addVoiceBoxWarning(result,
                           node,
                           QStringLiteral("unexpected-voicebox-type"),
                           QStringLiteral("voiceBox jsonKey type is not the legacy voice block type and was ignored"),
                           QStringLiteral(".type"));
    }

    const QString createTime = stringField(block, QStringLiteral("createTime"), node, result);
    const QString title = stringField(block, QStringLiteral("title"), node, result);
    QString text = stringField(block, QStringLiteral("text"), node, result);
    if (text.isEmpty()) {
        text = normalizedVisibleText(translatedTextOf(node));
    }

    const qint64 voiceSize = voiceSizeFromBlock(block, node, result);
    const QString voiceId = voiceIdFromBlock(block, node, voicePath, createTime, title, result);

    return MigrationJsonBuilder::makeVoiceBlock(voiceId,
                                                voicePath,
                                                voiceSize,
                                                createTime,
                                                title,
                                                text,
                                                translateUnfoldFromBlock(block));
}



bool isImageElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element && node.tagName == QStringLiteral("img");
}

bool isAllowedSchemeChar(QChar character)
{
    return character.isLetterOrNumber()
        || character == QLatin1Char('+')
        || character == QLatin1Char('.')
        || character == QLatin1Char('-');
}

QString normalizedUrlForSchemeCheck(const QString &src)
{
    QString normalized;
    normalized.reserve(src.size());

    const QString trimmed = src.trimmed();
    for (const QChar character : trimmed) {
        const ushort code = character.unicode();
        if (code <= 0x20 || code == 0x7f || character.isSpace()) {
            continue;
        }
        normalized.append(character);
    }

    return normalized;
}

bool startsWithNetworkPath(const QString &src)
{
    return src.startsWith(QStringLiteral("//")) || src.startsWith(QStringLiteral("\\\\"));
}

QString urlScheme(const QString &src)
{
    const int colonIndex = src.indexOf(QLatin1Char(':'));
    if (colonIndex <= 0) {
        return QString();
    }

    if (!src.at(0).isLetter()) {
        return QString();
    }

    for (int index = 1; index < colonIndex; ++index) {
        if (!isAllowedSchemeChar(src.at(index))) {
            return QString();
        }
    }

    return src.left(colonIndex).toLower();
}

bool isWindowsAbsolutePath(const QString &src)
{
    const QString trimmed = src.trimmed();
    return trimmed.size() >= 3
        && trimmed.at(0).isLetter()
        && trimmed.at(1) == QLatin1Char(':')
        && (trimmed.at(2) == QLatin1Char('\\') || trimmed.at(2) == QLatin1Char('/'));
}

QString withoutUrlQueryOrFragment(const QString &value)
{
    int cutIndex = -1;
    const int queryIndex = value.indexOf(QLatin1Char('?'));
    const int fragmentIndex = value.indexOf(QLatin1Char('#'));
    if (queryIndex >= 0) {
        cutIndex = queryIndex;
    }
    if (fragmentIndex >= 0 && (cutIndex < 0 || fragmentIndex < cutIndex)) {
        cutIndex = fragmentIndex;
    }

    return cutIndex >= 0 ? value.left(cutIndex) : value;
}

QString normalizedPathSeparators(QString value)
{
    value.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return value;
}

QString extractImagesRelPath(const QString &value)
{
    QString normalized = normalizedPathSeparators(withoutUrlQueryOrFragment(value.trimmed()));
    if (normalized.startsWith(QStringLiteral("./"))) {
        normalized.remove(0, 2);
    }

    int index = normalized.indexOf(QStringLiteral("images/"), 0, Qt::CaseInsensitive);
    while (index >= 0) {
        if (index == 0 || normalized.at(index - 1) == QLatin1Char('/')) {
            const QString relPath = normalized.mid(index);
            return relPath.isEmpty() ? QString() : relPath;
        }
        index = normalized.indexOf(QStringLiteral("images/"), index + 1, Qt::CaseInsensitive);
    }

    return QString();
}

bool isSafeResolvedImageSrc(const QString &src)
{
    const QString normalized = normalizedUrlForSchemeCheck(src);
    if (normalized.isEmpty() || startsWithNetworkPath(normalized)) {
        return false;
    }

    const QString scheme = urlScheme(normalized);
    return scheme.isEmpty()
        || scheme == QStringLiteral("http")
        || scheme == QStringLiteral("https");
}

struct ImageReference {
    QString src;
    QString relPath;
    QString warningCode;
    QString warningMessage;

    bool valid() const { return !src.isEmpty(); }
};

ImageReference resolvedImageReference(const QString &rawValue, bool preferRelPath)
{
    const QString trimmed = rawValue.trimmed();
    if (trimmed.isEmpty()) {
        return { QString(), QString(), kCodeMissingHtmlImageSrc, QStringLiteral("HTML image source is empty and was skipped") };
    }

    const QString normalized = normalizedUrlForSchemeCheck(trimmed);
    if (startsWithNetworkPath(normalized)) {
        return { QString(), QString(), kCodeUnsafeHtmlImageSrc, QStringLiteral("HTML image source uses an unsafe URL form and was skipped") };
    }

    const QString scheme = urlScheme(normalized);
    if (scheme == QStringLiteral("data")) {
        return { QString(), QString(), kCodeDowngradedBase64Image, QStringLiteral("Base64 HTML image data is not embedded during migration and was skipped") };
    }

    if (!scheme.isEmpty()
        && scheme != QStringLiteral("http")
        && scheme != QStringLiteral("https")
        && scheme != QStringLiteral("file")
        && !isWindowsAbsolutePath(trimmed)) {
        return { QString(), QString(), kCodeUnsafeHtmlImageSrc, QStringLiteral("HTML image source uses an unsafe URL scheme and was skipped") };
    }

    if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
        return { trimmed, QString(), QString(), QString() };
    }

    const QString relPath = extractImagesRelPath(trimmed);
    if (!relPath.isEmpty()) {
        return { relPath, relPath, QString(), QString() };
    }

    if (scheme == QStringLiteral("file")) {
        return { QString(), QString(), kCodeFileImageOutsideImagesDir, QStringLiteral("HTML file image URL has no images/ path segment and was skipped; manual relocation needed") };
    }

    if (!isSafeResolvedImageSrc(trimmed)) {
        return { QString(), QString(), kCodeUnsafeHtmlImageSrc, QStringLiteral("HTML image source uses an unsafe URL scheme and was skipped") };
    }

    QString relPathAttr;
    if (preferRelPath) {
        relPathAttr = normalizedPathSeparators(withoutUrlQueryOrFragment(trimmed));
        if (relPathAttr.startsWith(QStringLiteral("./"))) {
            relPathAttr.remove(0, 2);
        }
    } else {
        relPathAttr = extractImagesRelPath(trimmed);
    }

    return { trimmed, relPathAttr, QString(), QString() };
}

void warnSkippedImage(const MigrationHtmlNode &node,
                      MigrationHtmlConversionResult &result,
                      const QString &code,
                      const QString &message)
{
    if (!code.isEmpty()) {
        addWarning(result, elementPath(node) + QStringLiteral(".attrs.src"), code, message);
    }
}

QJsonObject imageFromElement(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    ImageReference reference;
    const QString dataRelPath = MigrationHtmlParser::attribute(node, QStringLiteral("data-rel-path"));
    if (!dataRelPath.trimmed().isEmpty()) {
        reference = resolvedImageReference(dataRelPath, true);
    }

    if (!reference.valid()) {
        const QString src = MigrationHtmlParser::attribute(node, QStringLiteral("src"));
        reference = resolvedImageReference(src, false);
    }

    if (!reference.valid()) {
        warnSkippedImage(node, result, reference.warningCode, reference.warningMessage);
        return QJsonObject();
    }

    return MigrationJsonBuilder::makeImage(reference.src,
                                           reference.relPath,
                                           MigrationHtmlParser::attribute(node, QStringLiteral("alt")),
                                           MigrationHtmlParser::attribute(node, QStringLiteral("title")));
}

const MigrationHtmlNode *singleImageChildIfOnlyImageContent(const MigrationHtmlNode &node)
{
    const MigrationHtmlNode *image = nullptr;
    for (const MigrationHtmlNode &child : node.children) {
        if (child.type == MigrationHtmlNodeType::Text) {
            if (!child.text.trimmed().isEmpty()) {
                return nullptr;
            }
            continue;
        }

        if (isImageElement(child) && image == nullptr) {
            image = &child;
            continue;
        }

        return nullptr;
    }

    return image;
}

// Summernote uses <p><br></p> / <div><br></div> as an empty paragraph placeholder.
// Persist it as an empty paragraph instead of paragraph + hardBreak; otherwise
// ProseMirror renders both the explicit hardBreak and its trailingBreak, creating
// one extra visible blank line between adjacent voice blocks.
bool isSingleBrPlaceholderContent(const MigrationHtmlNode &node)
{
    bool hasBr = false;
    for (const MigrationHtmlNode &child : node.children) {
        if (child.type == MigrationHtmlNodeType::Text) {
            if (!child.text.trimmed().isEmpty()) {
                return false;
            }
            continue;
        }

        if (child.type == MigrationHtmlNodeType::Element && child.tagName == QStringLiteral("br") && !hasBr) {
            hasBr = true;
            continue;
        }

        return false;
    }

    return hasBr;
}

int headingLevel(const MigrationHtmlNode &node)
{
    return node.tagName.right(1).toInt();
}

bool isTextNode(const QJsonValue &value)
{
    return value.isObject() && value.toObject().value(QStringLiteral("type")).toString() == QStringLiteral("text");
}

bool isHardBreakNode(const QJsonValue &value)
{
    return value.isObject() && value.toObject().value(QStringLiteral("type")).toString() == QStringLiteral("hardBreak");
}

QJsonArray marksOfTextNode(const QJsonObject &node)
{
    return node.value(QStringLiteral("marks")).toArray();
}

bool contentEndsWithSpace(const QJsonArray &content)
{
    if (content.isEmpty() || !isTextNode(content.at(content.size() - 1))) {
        return false;
    }

    return content.at(content.size() - 1).toObject().value(QStringLiteral("text")).toString().endsWith(QLatin1Char(' '));
}

QString markTypeOf(const QJsonValue &value)
{
    return value.isObject() ? value.toObject().value(QStringLiteral("type")).toString() : QString();
}

QJsonArray withoutMarkType(const QJsonArray &marks, const QString &type)
{
    QJsonArray filtered;
    for (const QJsonValue &mark : marks) {
        if (markTypeOf(mark) != type) {
            filtered.append(mark);
        }
    }
    return filtered;
}

QJsonArray withMark(QJsonArray marks, const QJsonObject &mark)
{
    const QString type = mark.value(QStringLiteral("type")).toString();
    if (type.isEmpty()) {
        return marks;
    }

    marks = withoutMarkType(marks, type);
    marks.append(mark);
    return marks;
}

QJsonArray withSimpleMark(const QJsonArray &marks, const QString &type)
{
    return withMark(marks, MigrationJsonBuilder::makeMark(type));
}

QJsonArray withoutSimpleMark(const QJsonArray &marks, const QString &type)
{
    return withoutMarkType(marks, type);
}

QJsonArray withAttrMark(const QJsonArray &marks,
                        const QString &type,
                        const QString &attrName,
                        const QString &attrValue)
{
    if (attrValue.isEmpty()) {
        return marks;
    }

    return withMark(marks, MigrationJsonBuilder::makeMark(type, QJsonObject { { attrName, attrValue } }));
}

void appendTextNode(QJsonArray &content, const QString &text, const QJsonArray &marks)
{
    if (text.isEmpty()) {
        return;
    }

    if (!content.isEmpty() && isTextNode(content.at(content.size() - 1))) {
        QJsonObject last = content.at(content.size() - 1).toObject();
        if (marksOfTextNode(last) == marks) {
            last.insert(QStringLiteral("text"), last.value(QStringLiteral("text")).toString() + text);
            content.replace(content.size() - 1, last);
            return;
        }
    }

    content.append(MigrationJsonBuilder::makeText(text, marks));
}

void appendVisibleText(QJsonArray &content, const QString &text, const QJsonArray &marks)
{
    QString normalized;
    bool pendingSpace = false;

    for (const QChar &character : text) {
        if (character.isSpace()) {
            pendingSpace = true;
            continue;
        }

        if (pendingSpace && (!normalized.isEmpty()
                             || (!content.isEmpty() && !isHardBreakNode(content.at(content.size() - 1)) && !contentEndsWithSpace(content)))) {
            normalized.append(QLatin1Char(' '));
        }

        normalized.append(character);
        pendingSpace = false;
    }

    if (pendingSpace && (!normalized.isEmpty()
                         || (!content.isEmpty() && !isHardBreakNode(content.at(content.size() - 1)) && !contentEndsWithSpace(content)))) {
        normalized.append(QLatin1Char(' '));
    }

    appendTextNode(content, normalized, marks);
}

void trimTrailingTextSpace(QJsonArray &content)
{
    if (content.isEmpty() || !isTextNode(content.at(content.size() - 1))) {
        return;
    }

    QJsonObject last = content.at(content.size() - 1).toObject();
    QString text = last.value(QStringLiteral("text")).toString();
    while (text.endsWith(QLatin1Char(' '))) {
        text.chop(1);
    }

    if (text.isEmpty()) {
        content.removeAt(content.size() - 1);
        return;
    }

    last.insert(QStringLiteral("text"), text);
    content.replace(content.size() - 1, last);
}

void appendHardBreak(QJsonArray &content)
{
    content.append(MigrationJsonBuilder::makeHardBreak());
}

QString stripImportant(QString value)
{
    value = value.trimmed();
    if (value.endsWith(QStringLiteral("!important"), Qt::CaseInsensitive)) {
        value.chop(QStringLiteral("!important").size());
        value = value.trimmed();
    }
    return value;
}

QStringList styleDeclarations(const MigrationHtmlNode &node)
{
    return MigrationHtmlParser::attribute(node, QStringLiteral("style")).split(QLatin1Char(';'), Qt::SkipEmptyParts);
}

void warnUnsupportedStyle(const MigrationHtmlNode &node,
                          const QString &property,
                          MigrationHtmlConversionResult &result)
{
    addWarning(result,
               elementPath(node) + QStringLiteral(".style.%1").arg(property),
               kCodeUnsupportedHtmlStyle,
               QStringLiteral("HTML style '%1' is not supported by this migration step and was ignored").arg(property));
}

void warnInvalidStyleValue(const MigrationHtmlNode &node,
                           const QString &property,
                           MigrationHtmlConversionResult &result)
{
    addWarning(result,
               elementPath(node) + QStringLiteral(".style.%1").arg(property),
               kCodeInvalidHtmlStyleValue,
               QStringLiteral("HTML style '%1' has an unsupported value and was ignored").arg(property));
}

void warnDowngradedTextAlign(const MigrationHtmlNode &node,
                             const QString &value,
                             MigrationHtmlConversionResult &result)
{
    addWarning(result,
               elementPath(node) + QStringLiteral(".style.text-align"),
               kCodeDowngradedTextAlign,
               QStringLiteral("HTML text-align '%1' is not supported by Schema V1 and was ignored").arg(value));
}

void warnInvalidListChild(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    addWarning(result,
               elementPath(node),
               kCodeDowngradedHtmlListChild,
               QStringLiteral("HTML node <%1> inside a list was wrapped into a list item").arg(node.tagName));
}

QString normalizedHexByte(int value)
{
    return QStringLiteral("%1").arg(value, 2, 16, QLatin1Char('0'));
}

QString normalizedColorValue(QString value)
{
    value = stripImportant(value).toLower();
    value.remove(QLatin1Char(' '));
    if (value.isEmpty()) {
        return QString();
    }

    static const QRegularExpression shortHex(QStringLiteral("^#([0-9a-f]{3})$"));
    const QRegularExpressionMatch shortHexMatch = shortHex.match(value);
    if (shortHexMatch.hasMatch()) {
        const QString digits = shortHexMatch.captured(1);
        return QStringLiteral("#%1%1%2%2%3%3")
            .arg(digits.at(0))
            .arg(digits.at(1))
            .arg(digits.at(2));
    }

    static const QRegularExpression longHex(QStringLiteral("^#[0-9a-f]{6}$"));
    if (longHex.match(value).hasMatch()) {
        return value;
    }

    static const QRegularExpression rgbColor(
        QStringLiteral("^rgba?\\((\\d{1,3}),(\\d{1,3}),(\\d{1,3})(?:,(?:0|1|0?\\.\\d+))?\\)$"));
    const QRegularExpressionMatch rgbMatch = rgbColor.match(value);
    if (rgbMatch.hasMatch()) {
        bool redOk = false;
        bool greenOk = false;
        bool blueOk = false;
        const int red = rgbMatch.captured(1).toInt(&redOk);
        const int green = rgbMatch.captured(2).toInt(&greenOk);
        const int blue = rgbMatch.captured(3).toInt(&blueOk);
        if (redOk && greenOk && blueOk && red >= 0 && red <= 255 && green >= 0 && green <= 255 && blue >= 0 && blue <= 255) {
            return QStringLiteral("#%1%2%3")
                .arg(normalizedHexByte(red), normalizedHexByte(green), normalizedHexByte(blue));
        }
    }

    static const QSet<QString> namedColors {
        QStringLiteral("black"),
        QStringLiteral("blue"),
        QStringLiteral("cyan"),
        QStringLiteral("gray"),
        QStringLiteral("green"),
        QStringLiteral("grey"),
        QStringLiteral("magenta"),
        QStringLiteral("orange"),
        QStringLiteral("purple"),
        QStringLiteral("red"),
        QStringLiteral("transparent"),
        QStringLiteral("white"),
        QStringLiteral("yellow")
    };
    if (namedColors.contains(value)) {
        return value;
    }

    return QString();
}

QString normalizedFontFamily(QString value)
{
    value = stripImportant(value).trimmed();
    return value.contains(QLatin1Char(';')) ? QString() : value;
}

QString normalizedFontSize(QString value)
{
    value = stripImportant(value).trimmed().toLower();
    static const QRegularExpression cssSize(QStringLiteral("^\\d+(?:\\.\\d+)?(?:px|pt|em|rem|%)$"));
    return cssSize.match(value).hasMatch() ? value : QString();
}

void warnTextAlignDeclarations(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    for (const QString &declaration : styleDeclarations(node)) {
        const int colonIndex = declaration.indexOf(QLatin1Char(':'));
        if (colonIndex <= 0) {
            continue;
        }

        const QString property = declaration.left(colonIndex).trimmed().toLower();
        if (property != QStringLiteral("text-align")) {
            continue;
        }

        const QString value = stripImportant(declaration.mid(colonIndex + 1)).trimmed().toLower();
        if (value == QStringLiteral("left") || value == QStringLiteral("start") || value == QStringLiteral("initial")) {
            continue;
        }

        if (value == QStringLiteral("center") || value == QStringLiteral("right")
            || value == QStringLiteral("end") || value == QStringLiteral("justify")) {
            warnDowngradedTextAlign(node, value, result);
        } else {
            warnInvalidStyleValue(node, property, result);
        }
    }
}

bool isBoldFontWeight(QString value, bool *known)
{
    value = stripImportant(value).trimmed().toLower();
    *known = true;
    if (value == QStringLiteral("bold") || value == QStringLiteral("bolder")) {
        return true;
    }
    if (value == QStringLiteral("normal") || value == QStringLiteral("lighter")) {
        return false;
    }

    bool ok = false;
    const int numericWeight = value.toInt(&ok);
    if (ok) {
        return numericWeight >= 600;
    }

    *known = false;
    return false;
}

bool isItalicFontStyle(QString value, bool *known)
{
    value = stripImportant(value).trimmed().toLower();
    *known = true;
    if (value == QStringLiteral("italic") || value == QStringLiteral("oblique")) {
        return true;
    }
    if (value == QStringLiteral("normal")) {
        return false;
    }

    *known = false;
    return false;
}

void applyTextDecorationMarks(const MigrationHtmlNode &node,
                              const QString &property,
                              QString value,
                              QJsonArray *marks,
                              MigrationHtmlConversionResult &result)
{
    value = stripImportant(value).toLower();
    const QStringList tokens = value.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        warnInvalidStyleValue(node, property, result);
        return;
    }

    bool hasUnsupportedToken = false;
    bool hasDecorationToken = false;
    bool hasNoneToken = false;
    for (const QString &token : tokens) {
        if (token == QStringLiteral("underline")) {
            *marks = withSimpleMark(*marks, QStringLiteral("underline"));
            hasDecorationToken = true;
        } else if (token == QStringLiteral("line-through")) {
            *marks = withSimpleMark(*marks, QStringLiteral("strike"));
            hasDecorationToken = true;
        } else if (token == QStringLiteral("none")) {
            hasNoneToken = true;
        } else {
            hasUnsupportedToken = true;
        }
    }

    // Treat a standalone 'none' as an explicit close for inherited decoration marks.
    if (hasNoneToken && !hasDecorationToken) {
        *marks = withoutSimpleMark(*marks, QStringLiteral("underline"));
        *marks = withoutSimpleMark(*marks, QStringLiteral("strike"));
    }

    if (hasUnsupportedToken) {
        warnInvalidStyleValue(node, property, result);
    }
}

void warnUnsupportedStyleDeclarations(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    for (const QString &declaration : styleDeclarations(node)) {
        const int colonIndex = declaration.indexOf(QLatin1Char(':'));
        if (colonIndex <= 0) {
            continue;
        }

        const QString property = declaration.left(colonIndex).trimmed().toLower();
        if (!property.isEmpty() && !supportedStyleProperties().contains(property)) {
            warnUnsupportedStyle(node, property, result);
        }
    }
}

void applyStyleMarks(const MigrationHtmlNode &node, QJsonArray *marks, MigrationHtmlConversionResult &result)
{
    for (const QString &declaration : styleDeclarations(node)) {
        const int colonIndex = declaration.indexOf(QLatin1Char(':'));
        if (colonIndex <= 0) {
            continue;
        }

        const QString property = declaration.left(colonIndex).trimmed().toLower();
        const QString value = declaration.mid(colonIndex + 1).trimmed();
        if (property == QStringLiteral("color")) {
            const QString color = normalizedColorValue(value);
            if (color.isEmpty()) {
                warnInvalidStyleValue(node, property, result);
            } else {
                *marks = withAttrMark(*marks, QStringLiteral("color"), QStringLiteral("color"), color);
            }
            continue;
        }

        if (property == QStringLiteral("background-color")) {
            const QString color = normalizedColorValue(value);
            if (color.isEmpty()) {
                warnInvalidStyleValue(node, property, result);
            } else {
                *marks = withAttrMark(*marks, QStringLiteral("highlight"), QStringLiteral("color"), color);
            }
            continue;
        }

        if (property == QStringLiteral("font-family")) {
            const QString fontFamily = normalizedFontFamily(value);
            if (fontFamily.isEmpty()) {
                warnInvalidStyleValue(node, property, result);
            } else {
                *marks = withAttrMark(*marks, QStringLiteral("fontFamily"), QStringLiteral("fontFamily"), fontFamily);
            }
            continue;
        }

        if (property == QStringLiteral("font-size")) {
            const QString fontSize = normalizedFontSize(value);
            if (fontSize.isEmpty()) {
                warnInvalidStyleValue(node, property, result);
            } else {
                *marks = withAttrMark(*marks, QStringLiteral("fontSize"), QStringLiteral("fontSize"), fontSize);
            }
            continue;
        }

        if (property == QStringLiteral("font-weight")) {
            bool known = false;
            if (isBoldFontWeight(value, &known)) {
                *marks = withSimpleMark(*marks, QStringLiteral("bold"));
            } else if (known) {
                *marks = withoutSimpleMark(*marks, QStringLiteral("bold"));
            } else if (!known) {
                warnInvalidStyleValue(node, property, result);
            }
            continue;
        }

        if (property == QStringLiteral("font-style")) {
            bool known = false;
            if (isItalicFontStyle(value, &known)) {
                *marks = withSimpleMark(*marks, QStringLiteral("italic"));
            } else if (known) {
                *marks = withoutSimpleMark(*marks, QStringLiteral("italic"));
            } else if (!known) {
                warnInvalidStyleValue(node, property, result);
            }
            continue;
        }

        if (property == QStringLiteral("text-decoration") || property == QStringLiteral("text-decoration-line")) {
            applyTextDecorationMarks(node, property, value, marks, result);
        }
    }
}

QJsonArray marksForElement(const MigrationHtmlNode &node,
                           const QJsonArray &inheritedMarks,
                           MigrationHtmlConversionResult &result)
{
    QJsonArray marks = inheritedMarks;
    if (node.tagName == QStringLiteral("b") || node.tagName == QStringLiteral("strong")) {
        marks = withSimpleMark(marks, QStringLiteral("bold"));
    } else if (node.tagName == QStringLiteral("i") || node.tagName == QStringLiteral("em")) {
        marks = withSimpleMark(marks, QStringLiteral("italic"));
    } else if (node.tagName == QStringLiteral("u")) {
        marks = withSimpleMark(marks, QStringLiteral("underline"));
    } else if (node.tagName == QStringLiteral("s") || node.tagName == QStringLiteral("strike") || node.tagName == QStringLiteral("del")) {
        marks = withSimpleMark(marks, QStringLiteral("strike"));
    } else if (node.tagName == QStringLiteral("font")) {
        const QString color = normalizedColorValue(MigrationHtmlParser::attribute(node, QStringLiteral("color")));
        const QString face = normalizedFontFamily(MigrationHtmlParser::attribute(node, QStringLiteral("face")));
        if (!color.isEmpty()) {
            marks = withAttrMark(marks, QStringLiteral("color"), QStringLiteral("color"), color);
        }
        if (!face.isEmpty()) {
            marks = withAttrMark(marks, QStringLiteral("fontFamily"), QStringLiteral("fontFamily"), face);
        }
    }

    warnUnsupportedStyleDeclarations(node, result);
    applyStyleMarks(node, &marks, result);
    return marks;
}

void appendInlineNode(const MigrationHtmlNode &node,
                      QJsonArray &content,
                      const QJsonArray &marks,
                      MigrationHtmlConversionResult &result);

void appendInlineChildren(const MigrationHtmlNode &node,
                          QJsonArray &content,
                          const QJsonArray &marks,
                          MigrationHtmlConversionResult &result)
{
    for (const MigrationHtmlNode &child : node.children) {
        appendInlineNode(child, content, marks, result);
    }
}

void appendInlineNode(const MigrationHtmlNode &node,
                      QJsonArray &content,
                      const QJsonArray &marks,
                      MigrationHtmlConversionResult &result)
{
    if (node.type == MigrationHtmlNodeType::Text) {
        appendVisibleText(content, node.text, marks);
        return;
    }

    if (node.type != MigrationHtmlNodeType::Element) {
        appendInlineChildren(node, content, marks, result);
        return;
    }

    if (isDangerousElement(node)) {
        return;
    }

    if (isVoiceBoxElement(node)) {
        appendVisibleText(content, visibleTextOf(node), marks);
        addVoiceBoxWarning(result,
                           node,
                           kCodeDowngradedInlineVoicebox,
                           QStringLiteral("inline voiceBox was downgraded to visible text"));
        return;
    }

    if (isImageElement(node)) {
        const QJsonObject image = imageFromElement(node, result);
        if (!image.isEmpty()) {
            content.append(image);
        }
        return;
    }

    if (node.tagName == QStringLiteral("br")) {
        appendHardBreak(content);
        return;
    }

    if (node.tagName == QStringLiteral("a")) {
        appendInlineChildren(node, content, marksForElement(node, marks, result), result);
        addWarning(result,
                   elementPath(node),
                   kCodeDowngradedHtmlLink,
                   QStringLiteral("HTML hyperlink was downgraded to plain text during migration"));
        return;
    }

    if (isBlockElement(node) && !content.isEmpty() && !isHardBreakNode(content.at(content.size() - 1))) {
        appendHardBreak(content);
    }

    if (!isBlockElement(node) && isDisplayAffectingInlineTag(node.tagName)) {
        addWarning(result,
                   elementPath(node),
                   kCodeDowngradedInlineElement,
                   QStringLiteral("Unknown inline HTML element was downgraded to plain text during migration"));
    }

    appendInlineChildren(node, content, marksForElement(node, marks, result), result);
}

void appendParagraphIfContent(QJsonArray &inlineContent, QJsonArray &blocks)
{
    trimTrailingTextSpace(inlineContent);
    if (!inlineContent.isEmpty()) {
        blocks.append(MigrationJsonBuilder::makeParagraph(inlineContent));
    }
    inlineContent = QJsonArray();
}

void appendBlockWithInitialParagraph(QJsonArray &blocks,
                                     const QJsonObject &block,
                                     bool ensureInitialParagraphBeforeBlock)
{
    if (block.isEmpty()) {
        return;
    }

    if (ensureInitialParagraphBeforeBlock
        && blocks.isEmpty()
        && block.value(QStringLiteral("type")).toString() != QStringLiteral("paragraph")) {
        blocks.append(MigrationJsonBuilder::makeParagraph());
    }

    blocks.append(block);
}

QJsonArray inlineContentFrom(const MigrationHtmlNode &node,
                             MigrationHtmlConversionResult &result,
                             const QJsonArray &inheritedMarks = QJsonArray())
{
    QJsonArray content;
    appendInlineChildren(node, content, marksForElement(node, inheritedMarks, result), result);
    trimTrailingTextSpace(content);
    return content;
}

QJsonObject blockFromElement(const MigrationHtmlNode &node,
                             MigrationHtmlConversionResult &result,
                             const QJsonArray &inheritedMarks = QJsonArray());

QJsonObject listFromElement(const MigrationHtmlNode &node,
                            MigrationHtmlConversionResult &result,
                            const QJsonArray &inheritedMarks = QJsonArray());

void appendBlocksFromElement(const MigrationHtmlNode &node,
                             QJsonArray &blocks,
                             MigrationHtmlConversionResult &result,
                             const QJsonArray &inheritedMarks = QJsonArray(),
                             bool ensureInitialParagraphBeforeBlock = false);

void appendImageBlock(const MigrationHtmlNode &node,
                      QJsonArray &inlineContent,
                      QJsonArray &blocks,
                      MigrationHtmlConversionResult &result,
                      bool ensureInitialParagraphBeforeBlock)
{
    appendParagraphIfContent(inlineContent, blocks);
    const QJsonObject image = imageFromElement(node, result);
    if (image.isEmpty()) {
        return;
    }

    appendBlockWithInitialParagraph(blocks, image, ensureInitialParagraphBeforeBlock);
}

void appendInlineOrImageBlocks(const MigrationHtmlNode &node,
                               QJsonArray &inlineContent,
                               QJsonArray &blocks,
                               const QJsonArray &marks,
                               MigrationHtmlConversionResult &result,
                               bool ensureInitialParagraphBeforeBlock)
{
    if (node.type == MigrationHtmlNodeType::Text) {
        appendVisibleText(inlineContent, node.text, marks);
        return;
    }

    if (node.type != MigrationHtmlNodeType::Element) {
        for (const MigrationHtmlNode &child : node.children) {
            appendInlineOrImageBlocks(child, inlineContent, blocks, marks, result, ensureInitialParagraphBeforeBlock);
        }
        return;
    }

    if (isDangerousElement(node)) {
        return;
    }

    if (isImageElement(node)) {
        appendImageBlock(node, inlineContent, blocks, result, ensureInitialParagraphBeforeBlock);
        return;
    }

    if (node.tagName == QStringLiteral("br")) {
        appendHardBreak(inlineContent);
        return;
    }

    if (isBlockElement(node) && !inlineContent.isEmpty() && !isHardBreakNode(inlineContent.at(inlineContent.size() - 1))) {
        appendHardBreak(inlineContent);
    }

    if (node.tagName == QStringLiteral("a")) {
        addWarning(result,
                   elementPath(node),
                   kCodeDowngradedHtmlLink,
                   QStringLiteral("HTML hyperlink was downgraded to plain text during migration"));
    }

    if (!isBlockElement(node) && isDisplayAffectingInlineTag(node.tagName)) {
        addWarning(result,
                   elementPath(node),
                   kCodeDowngradedInlineElement,
                   QStringLiteral("Unknown inline HTML element was downgraded to plain text during migration"));
    }

    const QJsonArray childMarks = marksForElement(node, marks, result);
    for (const MigrationHtmlNode &child : node.children) {
        appendInlineOrImageBlocks(child, inlineContent, blocks, childMarks, result, ensureInitialParagraphBeforeBlock);
    }
}

void appendInlineContainerAsBlocks(const MigrationHtmlNode &node,
                                   QJsonArray &blocks,
                                   MigrationHtmlConversionResult &result,
                                   const QJsonArray &inheritedMarks,
                                   bool ensureInitialParagraphBeforeBlock)
{
    const int initialBlockCount = blocks.size();
    QJsonArray inlineContent;
    const QJsonArray marks = marksForElement(node, inheritedMarks, result);
    for (const MigrationHtmlNode &child : node.children) {
        appendInlineOrImageBlocks(child, inlineContent, blocks, marks, result, ensureInitialParagraphBeforeBlock);
    }
    appendParagraphIfContent(inlineContent, blocks);

    if (blocks.size() == initialBlockCount) {
        blocks.append(MigrationJsonBuilder::makeParagraph());
    }
}

void appendHeadingIfContent(QJsonArray &inlineContent,
                            QJsonArray &blocks,
                            int level,
                            bool ensureInitialParagraphBeforeBlock)
{
    trimTrailingTextSpace(inlineContent);
    if (!inlineContent.isEmpty()) {
        appendBlockWithInitialParagraph(blocks,
                                        MigrationJsonBuilder::makeHeading(level, inlineContent),
                                        ensureInitialParagraphBeforeBlock);
    }
    inlineContent = QJsonArray();
}

void appendHeadingImageBlock(const MigrationHtmlNode &node,
                             QJsonArray &inlineContent,
                             QJsonArray &blocks,
                             int level,
                             MigrationHtmlConversionResult &result,
                             bool ensureInitialParagraphBeforeBlock)
{
    appendHeadingIfContent(inlineContent, blocks, level, ensureInitialParagraphBeforeBlock);
    const QJsonObject image = imageFromElement(node, result);
    if (image.isEmpty()) {
        return;
    }

    appendBlockWithInitialParagraph(blocks, image, ensureInitialParagraphBeforeBlock);
}

void appendHeadingInlineOrImageBlocks(const MigrationHtmlNode &node,
                                      QJsonArray &inlineContent,
                                      QJsonArray &blocks,
                                      int level,
                                      const QJsonArray &marks,
                                      MigrationHtmlConversionResult &result,
                                      bool ensureInitialParagraphBeforeBlock)
{
    if (node.type == MigrationHtmlNodeType::Text) {
        appendVisibleText(inlineContent, node.text, marks);
        return;
    }

    if (node.type != MigrationHtmlNodeType::Element) {
        for (const MigrationHtmlNode &child : node.children) {
            appendHeadingInlineOrImageBlocks(child, inlineContent, blocks, level, marks, result, ensureInitialParagraphBeforeBlock);
        }
        return;
    }

    if (isDangerousElement(node)) {
        return;
    }

    if (isImageElement(node)) {
        appendHeadingImageBlock(node, inlineContent, blocks, level, result, ensureInitialParagraphBeforeBlock);
        return;
    }

    if (node.tagName == QStringLiteral("br")) {
        appendHardBreak(inlineContent);
        return;
    }

    if (isBlockElement(node) && !inlineContent.isEmpty() && !isHardBreakNode(inlineContent.at(inlineContent.size() - 1))) {
        appendHardBreak(inlineContent);
    }

    const QJsonArray childMarks = marksForElement(node, marks, result);
    for (const MigrationHtmlNode &child : node.children) {
        appendHeadingInlineOrImageBlocks(child, inlineContent, blocks, level, childMarks, result, ensureInitialParagraphBeforeBlock);
    }
}

void appendHeadingElementAsBlocks(const MigrationHtmlNode &node,
                                  QJsonArray &blocks,
                                  MigrationHtmlConversionResult &result,
                                  const QJsonArray &inheritedMarks,
                                  bool ensureInitialParagraphBeforeBlock)
{
    const int initialBlockCount = blocks.size();
    QJsonArray inlineContent;
    const int level = headingLevel(node);
    const QJsonArray marks = marksForElement(node, inheritedMarks, result);
    for (const MigrationHtmlNode &child : node.children) {
        appendHeadingInlineOrImageBlocks(child, inlineContent, blocks, level, marks, result, ensureInitialParagraphBeforeBlock);
    }
    appendHeadingIfContent(inlineContent, blocks, level, ensureInitialParagraphBeforeBlock);

    if (blocks.size() == initialBlockCount) {
        appendBlockWithInitialParagraph(blocks,
                                        MigrationJsonBuilder::makeHeading(level),
                                        ensureInitialParagraphBeforeBlock);
    }
}

void warnDowngradedBlock(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    if (node.tagName == QStringLiteral("p") || node.tagName == QStringLiteral("div")) {
        return;
    }

    addWarning(result,
               QStringLiteral("/%1").arg(node.tagName),
               kCodeDowngradedHtmlBlock,
               QStringLiteral("HTML block <%1> was downgraded to paragraph").arg(node.tagName));
}

void appendBlocksFromChildren(const MigrationHtmlNode &node,
                              QJsonArray &blocks,
                              MigrationHtmlConversionResult &result,
                              const QJsonArray &inheritedMarks = QJsonArray())
{
    QJsonArray inlineContent;
    for (const MigrationHtmlNode &child : node.children) {
        if (isDangerousElement(child)) {
            continue;
        }

        if (isVoiceBoxElement(child)) {
            appendParagraphIfContent(inlineContent, blocks);
            blocks.append(voiceBlockFromElement(child, result));
            continue;
        }

        if (isImageElement(child)) {
            appendImageBlock(child, inlineContent, blocks, result, false);
            continue;
        }

        if (isBlockElement(child)) {
            if (!inlineContent.isEmpty()) {
                appendParagraphIfContent(inlineContent, blocks);
            }
            appendBlocksFromElement(child, blocks, result, inheritedMarks);
            continue;
        }

        appendInlineOrImageBlocks(child, inlineContent, blocks, inheritedMarks, result, false);
    }

    if (!inlineContent.isEmpty()) {
        appendParagraphIfContent(inlineContent, blocks);
    }
}

QJsonObject downgradedParagraphFromBlock(const MigrationHtmlNode &node,
                                         MigrationHtmlConversionResult &result,
                                         const QJsonArray &inheritedMarks = QJsonArray())
{
    warnDowngradedBlock(node, result);

    return MigrationJsonBuilder::makeParagraph(inlineContentFrom(node, result, inheritedMarks));
}

void appendInlineContentAsParagraph(QJsonArray &inlineContent, QJsonArray &blocks)
{
    trimTrailingTextSpace(inlineContent);
    if (!inlineContent.isEmpty()) {
        blocks.append(MigrationJsonBuilder::makeParagraph(inlineContent));
    }
    inlineContent = QJsonArray();
}

QJsonArray listItemContentFromElement(const MigrationHtmlNode &node,
                                      MigrationHtmlConversionResult &result,
                                      const QJsonArray &parentMarks = QJsonArray())
{
    QJsonArray content;
    QJsonArray inlineContent;
    warnTextAlignDeclarations(node, result);
    const QJsonArray inheritedMarks = marksForElement(node, parentMarks, result);

    for (const MigrationHtmlNode &child : node.children) {
        if (isDangerousElement(child)) {
            continue;
        }

        if (isListElement(child)) {
            appendInlineContentAsParagraph(inlineContent, content);
            if (content.isEmpty()) {
                content.append(MigrationJsonBuilder::makeParagraph());
            }
            content.append(listFromElement(child, result, inheritedMarks));
            continue;
        }

        if (isVoiceBoxElement(child)) {
            appendInlineContentAsParagraph(inlineContent, content);
            appendBlockWithInitialParagraph(content, voiceBlockFromElement(child, result), true);
            continue;
        }

        if (isImageElement(child)) {
            appendImageBlock(child, inlineContent, content, result, true);
            continue;
        }

        if (isBlockElement(child)) {
            appendInlineContentAsParagraph(inlineContent, content);
            appendBlocksFromElement(child, content, result, inheritedMarks, true);
            continue;
        }

        appendInlineOrImageBlocks(child, inlineContent, content, inheritedMarks, result, true);
    }

    appendInlineContentAsParagraph(inlineContent, content);
    return content;
}

QJsonObject listItemFromInvalidListChild(const MigrationHtmlNode &node,
                                         MigrationHtmlConversionResult &result,
                                         const QJsonArray &parentMarks = QJsonArray())
{
    warnInvalidListChild(node, result);

    if (isListElement(node)) {
        return MigrationJsonBuilder::makeListItem(QJsonArray {
            MigrationJsonBuilder::makeParagraph(),
            listFromElement(node, result, parentMarks)
        });
    }

    if (isBlockElement(node)) {
        QJsonArray content;
        appendBlocksFromElement(node, content, result, parentMarks, true);
        return MigrationJsonBuilder::makeListItem(content);
    }

    QJsonArray inlineContent;
    appendInlineNode(node, inlineContent, parentMarks, result);
    trimTrailingTextSpace(inlineContent);
    return MigrationJsonBuilder::makeListItem(QJsonArray { MigrationJsonBuilder::makeParagraph(inlineContent) });
}

QJsonObject listFromElement(const MigrationHtmlNode &node,
                            MigrationHtmlConversionResult &result,
                            const QJsonArray &inheritedMarks)
{
    warnTextAlignDeclarations(node, result);
    const QJsonArray listMarks = marksForElement(node, inheritedMarks, result);

    QJsonArray items;
    for (const MigrationHtmlNode &child : node.children) {
        if (isDangerousElement(child)) {
            continue;
        }

        if (child.type == MigrationHtmlNodeType::Text && child.text.trimmed().isEmpty()) {
            continue;
        }

        if (isListItemElement(child)) {
            if (isVoiceBoxElement(child)) {
                items.append(MigrationJsonBuilder::makeListItem(QJsonArray {
                    MigrationJsonBuilder::makeParagraph(),
                    voiceBlockFromElement(child, result)
                }));
            } else {
                items.append(MigrationJsonBuilder::makeListItem(
                    listItemContentFromElement(child, result, listMarks)));
            }
        } else {
            items.append(listItemFromInvalidListChild(child, result, listMarks));
        }
    }

    return node.tagName == QStringLiteral("ol")
        ? MigrationJsonBuilder::makeOrderedList(items)
        : MigrationJsonBuilder::makeBulletList(items);
}

QJsonObject blockquoteFromElement(const MigrationHtmlNode &node,
                                  MigrationHtmlConversionResult &result,
                                  const QJsonArray &inheritedMarks = QJsonArray())
{
    QJsonArray content;
    appendBlocksFromChildren(node, content, result, inheritedMarks);
    return MigrationJsonBuilder::makeBlockquote(content);
}

void appendBlocksFromElement(const MigrationHtmlNode &node,
                             QJsonArray &blocks,
                             MigrationHtmlConversionResult &result,
                             const QJsonArray &inheritedMarks,
                             bool ensureInitialParagraphBeforeBlock)
{
    if (isVoiceBoxElement(node)) {
        appendBlockWithInitialParagraph(blocks,
                                        voiceBlockFromElement(node, result),
                                        ensureInitialParagraphBeforeBlock);
        return;
    }

    if (isImageElement(node)) {
        QJsonArray inlineContent;
        appendImageBlock(node, inlineContent, blocks, result, ensureInitialParagraphBeforeBlock);
        return;
    }

    if (node.tagName == QStringLiteral("p") || node.tagName == QStringLiteral("div")) {
        warnTextAlignDeclarations(node, result);
        if (isSingleBrPlaceholderContent(node)) {
            appendBlockWithInitialParagraph(blocks,
                                            MigrationJsonBuilder::makeParagraph(),
                                            ensureInitialParagraphBeforeBlock);
            return;
        }

        if (const MigrationHtmlNode *image = singleImageChildIfOnlyImageContent(node)) {
            const int initialBlockCount = blocks.size();
            QJsonArray inlineContent;
            appendImageBlock(*image, inlineContent, blocks, result, ensureInitialParagraphBeforeBlock);
            if (blocks.size() == initialBlockCount) {
                blocks.append(MigrationJsonBuilder::makeParagraph());
            }
            return;
        }

        appendInlineContainerAsBlocks(node, blocks, result, inheritedMarks, ensureInitialParagraphBeforeBlock);
        return;
    }

    if (isHeadingElement(node)) {
        warnTextAlignDeclarations(node, result);
        appendHeadingElementAsBlocks(node, blocks, result, inheritedMarks, ensureInitialParagraphBeforeBlock);
        return;
    }

    if (isListElement(node) || node.tagName == QStringLiteral("blockquote")) {
        appendBlockWithInitialParagraph(blocks,
                                        blockFromElement(node, result, inheritedMarks),
                                        ensureInitialParagraphBeforeBlock);
        return;
    }

    if (isListItemElement(node)) {
        warnTextAlignDeclarations(node, result);
        addWarning(result,
                   elementPath(node),
                   kCodeDowngradedOrphanListItem,
                   QStringLiteral("HTML list item outside a list was downgraded to paragraph"));
        appendInlineContainerAsBlocks(node, blocks, result, inheritedMarks, ensureInitialParagraphBeforeBlock);
        return;
    }

    warnTextAlignDeclarations(node, result);
    warnDowngradedBlock(node, result);
    appendInlineContainerAsBlocks(node, blocks, result, inheritedMarks, ensureInitialParagraphBeforeBlock);
}

QJsonObject blockFromElement(const MigrationHtmlNode &node,
                             MigrationHtmlConversionResult &result,
                             const QJsonArray &inheritedMarks)
{
    if (isVoiceBoxElement(node)) {
        return voiceBlockFromElement(node, result);
    }

    if (isImageElement(node)) {
        return imageFromElement(node, result);
    }

    if (node.tagName == QStringLiteral("p") || node.tagName == QStringLiteral("div")) {
        warnTextAlignDeclarations(node, result);
        if (isSingleBrPlaceholderContent(node)) {
            return MigrationJsonBuilder::makeParagraph();
        }

        if (const MigrationHtmlNode *image = singleImageChildIfOnlyImageContent(node)) {
            const QJsonObject imageNode = imageFromElement(*image, result);
            if (!imageNode.isEmpty()) {
                return imageNode;
            }
        }
    }

    if (isHeadingElement(node)) {
        warnTextAlignDeclarations(node, result);
        return MigrationJsonBuilder::makeHeading(headingLevel(node), inlineContentFrom(node, result, inheritedMarks));
    }

    if (isListElement(node)) {
        return listFromElement(node, result, inheritedMarks);
    }

    if (isListItemElement(node)) {
        warnTextAlignDeclarations(node, result);
        addWarning(result,
                   elementPath(node),
                   kCodeDowngradedOrphanListItem,
                   QStringLiteral("HTML list item outside a list was downgraded to paragraph"));
        return downgradedParagraphFromBlock(node, result, inheritedMarks);
    }

    if (node.tagName == QStringLiteral("blockquote")) {
        warnTextAlignDeclarations(node, result);
        return blockquoteFromElement(node, result, inheritedMarks);
    }

    warnTextAlignDeclarations(node, result);
    return downgradedParagraphFromBlock(node, result, inheritedMarks);
}

QJsonObject envelopeFromParsed(const MigrationHtmlParseResult &parsed, MigrationHtmlConversionResult &result)
{
    QJsonArray docContent;
    appendBlocksFromChildren(parsed.root, docContent, result);
    return MigrationJsonBuilder::makeEnvelope(MigrationJsonBuilder::makeDoc(docContent));
}

} // namespace

bool MigrationHtmlConversionResult::ok() const
{
    return errors.isEmpty();
}

MigrationHtmlConversionResult MigrationHtmlConverter::convert(const QString &htmlCode)
{
    MigrationHtmlConversionResult result;
    const MigrationHtmlParseResult parsed = MigrationHtmlParser::parse(htmlCode);
    copyParseIssues(parsed, result);
    result.envelope = envelopeFromParsed(parsed, result);
    return result;
}
