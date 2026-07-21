// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationjsonvalidator.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QStringList>

#include <cmath>

namespace {
constexpr int kSchemaVersion = 1;
constexpr int kMaxNodeDepth = 100;

const QStringList kSupportedNodes = {
    QStringLiteral("doc"),
    QStringLiteral("paragraph"),
    QStringLiteral("text"),
    QStringLiteral("hardBreak"),
    QStringLiteral("heading"),
    QStringLiteral("blockquote"),
    QStringLiteral("bulletList"),
    QStringLiteral("orderedList"),
    QStringLiteral("listItem"),
    QStringLiteral("taskList"),
    QStringLiteral("taskItem"),
    QStringLiteral("image"),
    QStringLiteral("voiceBlock"),
};

const QStringList kSupportedMarks = {
    QStringLiteral("bold"),
    QStringLiteral("italic"),
    QStringLiteral("underline"),
    QStringLiteral("strike"),
    QStringLiteral("color"),
    QStringLiteral("highlight"),
    QStringLiteral("fontFamily"),
    QStringLiteral("fontSize"),
};

void addError(MigrationJsonValidationResult &result,
              const QString &path,
              const QString &code,
              const QString &message)
{
    result.errors.append({ path, code, message });
}

bool hasOwnProperty(const QJsonObject &object, const QString &key)
{
    return object.constFind(key) != object.constEnd();
}

bool isNonEmptyString(const QJsonValue &value)
{
    return value.isString() && !value.toString().trimmed().isEmpty();
}

bool isFiniteNumber(const QJsonValue &value)
{
    return value.isDouble() && std::isfinite(value.toDouble());
}

bool isSchemaVersionV1(const QJsonValue &value)
{
    if (!isFiniteNumber(value)) {
        return false;
    }

    return value.toDouble() == kSchemaVersion;
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

    const QChar first = src.at(0);
    if (!first.isLetter()) {
        return QString();
    }

    for (int index = 1; index < colonIndex; ++index) {
        if (!isAllowedSchemeChar(src.at(index))) {
            return QString();
        }
    }

    return src.left(colonIndex).toLower();
}

bool isSafeImageSrc(const QString &src)
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

void validateMarks(const QJsonObject &node,
                   const QString &path,
                   MigrationJsonValidationResult &result)
{
    const QJsonValue marksValue = node.value(QStringLiteral("marks"));
    if (marksValue.isUndefined()) {
        return;
    }

    if (!marksValue.isArray()) {
        addError(result,
                 path + QStringLiteral(".marks"),
                 QStringLiteral("invalid-marks"),
                 QStringLiteral("marks must be an array"));
        return;
    }

    const QJsonArray marks = marksValue.toArray();
    for (int index = 0; index < marks.size(); ++index) {
        const QString markPath = QStringLiteral("%1.marks[%2]").arg(path).arg(index);
        const QJsonValue markValue = marks.at(index);
        if (!markValue.isObject()) {
            addError(result,
                     markPath,
                     QStringLiteral("invalid-mark"),
                     QStringLiteral("mark must be an object"));
            continue;
        }

        const QJsonObject mark = markValue.toObject();
        const QString markType = mark.value(QStringLiteral("type")).toString();
        if (!kSupportedMarks.contains(markType)) {
            addError(result,
                     markPath + QStringLiteral(".type"),
                     QStringLiteral("unsupported-mark"),
                     QStringLiteral("unsupported mark type: %1").arg(markType));
        }
    }
}

void validateImageNode(const QJsonObject &node,
                       const QString &path,
                       MigrationJsonValidationResult &result)
{
    const QJsonObject attrs = node.value(QStringLiteral("attrs")).toObject();
    const QJsonValue srcValue = attrs.value(QStringLiteral("src"));
    if (!srcValue.isString() || srcValue.toString().isEmpty()) {
        addError(result,
                 path + QStringLiteral(".attrs.src"),
                 QStringLiteral("invalid-image-src"),
                 QStringLiteral("image src is required"));
        return;
    }

    if (!isSafeImageSrc(srcValue.toString())) {
        addError(result,
                 path + QStringLiteral(".attrs.src"),
                 QStringLiteral("unsafe-image-src"),
                 QStringLiteral("image src uses an unsafe URL scheme"));
    }
}

void validateVoiceBlockNode(const QJsonObject &node,
                            const QString &path,
                            MigrationJsonValidationResult &result)
{
    const QJsonObject attrs = node.value(QStringLiteral("attrs")).toObject();
    if (!isNonEmptyString(attrs.value(QStringLiteral("voiceId")))) {
        addError(result,
                 path + QStringLiteral(".attrs.voiceId"),
                 QStringLiteral("invalid-voice-id"),
                 QStringLiteral("voiceId is required"));
    }

    if (!isNonEmptyString(attrs.value(QStringLiteral("voicePath")))) {
        addError(result,
                 path + QStringLiteral(".attrs.voicePath"),
                 QStringLiteral("invalid-voice-path"),
                 QStringLiteral("voicePath is required"));
    }

    const QJsonValue voiceSize = attrs.value(QStringLiteral("voiceSize"));
    if (!isFiniteNumber(voiceSize) || voiceSize.toDouble() < 0) {
        addError(result,
                 path + QStringLiteral(".attrs.voiceSize"),
                 QStringLiteral("invalid-voice-size"),
                 QStringLiteral("voiceSize must be a non-negative number"));
    }

    if (hasOwnProperty(attrs, QStringLiteral("translating"))) {
        addError(result,
                 path + QStringLiteral(".attrs.translating"),
                 QStringLiteral("runtime-state-persisted"),
                 QStringLiteral("translating is runtime state and is not part of Schema V1 persistence"));
    }
}

void validateNode(const QJsonValue &nodeValue,
                  const QString &path,
                  MigrationJsonValidationResult &result,
                  int depth = 0)
{
    if (depth > kMaxNodeDepth) {
        addError(result,
                 path,
                 QStringLiteral("max-node-depth-exceeded"),
                 QStringLiteral("node depth must not exceed %1").arg(kMaxNodeDepth));
        return;
    }

    if (!nodeValue.isObject()) {
        addError(result, path, QStringLiteral("invalid-node"), QStringLiteral("node must be an object"));
        return;
    }

    const QJsonObject node = nodeValue.toObject();
    const QString nodeType = node.value(QStringLiteral("type")).toString();
    if (!kSupportedNodes.contains(nodeType)) {
        addError(result,
                 path + QStringLiteral(".type"),
                 QStringLiteral("unsupported-node"),
                 QStringLiteral("unsupported node type: %1").arg(nodeType));
    }

    if (nodeType == QStringLiteral("text") && !node.value(QStringLiteral("text")).isString()) {
        addError(result,
                 path + QStringLiteral(".text"),
                 QStringLiteral("invalid-text"),
                 QStringLiteral("text node requires string text"));
    }

    if (nodeType == QStringLiteral("heading")) {
        const QJsonValue level = node.value(QStringLiteral("attrs")).toObject().value(QStringLiteral("level"));
        if (!isFiniteNumber(level) || std::floor(level.toDouble()) != level.toDouble()
            || level.toInt() < 1 || level.toInt() > 6) {
            addError(result,
                     path + QStringLiteral(".attrs.level"),
                     QStringLiteral("invalid-heading-level"),
                     QStringLiteral("heading level must be 1..6"));
        }
    }

    if (nodeType == QStringLiteral("image")) {
        validateImageNode(node, path, result);
    }

    if (nodeType == QStringLiteral("voiceBlock")) {
        validateVoiceBlockNode(node, path, result);
    }

    validateMarks(node, path, result);

    const QJsonValue contentValue = node.value(QStringLiteral("content"));
    if (contentValue.isUndefined()) {
        return;
    }

    if (!contentValue.isArray()) {
        addError(result,
                 path + QStringLiteral(".content"),
                 QStringLiteral("invalid-content-array"),
                 QStringLiteral("node content must be an array"));
        return;
    }

    const QJsonArray content = contentValue.toArray();
    for (int index = 0; index < content.size(); ++index) {
        validateNode(content.at(index),
                     QStringLiteral("%1.content[%2]").arg(path).arg(index),
                     result,
                     depth + 1);
    }
}

void validateDocumentContent(const QJsonObject &content, MigrationJsonValidationResult &result)
{
    if (content.value(QStringLiteral("type")).toString() != QStringLiteral("doc")) {
        return;
    }

    const QJsonValue docContent = content.value(QStringLiteral("content"));
    if (!docContent.isArray()) {
        addError(result,
                 QStringLiteral("content.content"),
                 QStringLiteral("invalid-content-array"),
                 QStringLiteral("doc content must be an array"));
        return;
    }

    if (docContent.toArray().isEmpty()) {
        addError(result,
                 QStringLiteral("content.content"),
                 QStringLiteral("empty-doc-content"),
                 QStringLiteral("empty documents must be persisted as one empty paragraph"));
    }
}

} // namespace

bool MigrationJsonValidationResult::ok() const
{
    return errors.isEmpty();
}

MigrationJsonValidationResult MigrationJsonValidator::validateEnvelope(const QJsonObject &envelope)
{
    MigrationJsonValidationResult result;

    if (envelope.value(QStringLiteral("format")).toString() != QStringLiteral("tiptap")) {
        addError(result,
                 QStringLiteral("format"),
                 QStringLiteral("invalid-format"),
                 QStringLiteral("format must be tiptap"));
    }

    if (!isSchemaVersionV1(envelope.value(QStringLiteral("schemaVersion")))) {
        addError(result,
                 QStringLiteral("schemaVersion"),
                 QStringLiteral("unsupported-schema-version"),
                 QStringLiteral("schemaVersion must be 1"));
    }

    const QJsonValue contentValue = envelope.value(QStringLiteral("content"));
    if (!contentValue.isObject()) {
        addError(result,
                 QStringLiteral("content"),
                 QStringLiteral("invalid-content"),
                 QStringLiteral("content must be a ProseMirror document object"));
        return result;
    }

    const QJsonObject content = contentValue.toObject();
    if (content.value(QStringLiteral("type")).toString() != QStringLiteral("doc")) {
        addError(result,
                 QStringLiteral("content.type"),
                 QStringLiteral("invalid-root"),
                 QStringLiteral("content.type must be doc"));
    }

    validateDocumentContent(content, result);
    validateNode(contentValue, QStringLiteral("content"), result);

    return result;
}
