// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationjsonbuilder.h"

#include <QJsonDocument>
#include <QJsonValue>

namespace {
constexpr int kSchemaVersion = 1;
}

QJsonObject MigrationJsonBuilder::makeEnvelope()
{
    return makeEnvelope(makeDoc());
}

QJsonObject MigrationJsonBuilder::makeEnvelope(const QJsonObject &content)
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("format"), QStringLiteral("tiptap"));
    envelope.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    envelope.insert(QStringLiteral("content"), content.isEmpty() ? makeDoc() : content);
    return envelope;
}

QJsonObject MigrationJsonBuilder::makeDoc(const QJsonArray &content)
{
    QJsonObject doc;
    doc.insert(QStringLiteral("type"), QStringLiteral("doc"));
    doc.insert(QStringLiteral("content"), normalizedDocContent(content));
    return doc;
}

QJsonObject MigrationJsonBuilder::makeParagraph(const QJsonArray &content)
{
    return makeContainerNode(QStringLiteral("paragraph"), content);
}

QJsonObject MigrationJsonBuilder::makeText(const QString &text, const QJsonArray &marks)
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("text"));
    node.insert(QStringLiteral("text"), text);
    if (!marks.isEmpty()) {
        node.insert(QStringLiteral("marks"), marks);
    }
    return node;
}

QJsonObject MigrationJsonBuilder::makeHardBreak()
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("hardBreak"));
    return node;
}

QJsonObject MigrationJsonBuilder::makeHeading(int level, const QJsonArray &content)
{
    QJsonObject node = makeContainerNode(QStringLiteral("heading"), content);
    node.insert(QStringLiteral("attrs"), QJsonObject { { QStringLiteral("level"), level } });
    return node;
}

QJsonObject MigrationJsonBuilder::makeMark(const QString &type, const QJsonObject &attrs)
{
    QJsonObject mark;
    mark.insert(QStringLiteral("type"), type);
    if (!attrs.isEmpty()) {
        mark.insert(QStringLiteral("attrs"), attrs);
    }
    return mark;
}

QJsonObject MigrationJsonBuilder::makeImage(const QString &src,
                                         const QString &relPath,
                                         const QString &alt,
                                         const QString &title)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("src"), src);
    attrs.insert(QStringLiteral("relPath"), nullableString(relPath));
    attrs.insert(QStringLiteral("alt"), alt);
    attrs.insert(QStringLiteral("title"), nullableString(title));

    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("image"));
    node.insert(QStringLiteral("attrs"), attrs);
    return node;
}

QJsonObject MigrationJsonBuilder::makeVoiceBlock(const QString &voiceId,
                                              const QString &voicePath,
                                              qint64 voiceSize,
                                              const QString &createTime,
                                              const QString &title,
                                              const QString &text,
                                              bool translateUnfold)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("voiceId"), voiceId);
    attrs.insert(QStringLiteral("voicePath"), voicePath);
    attrs.insert(QStringLiteral("voiceSize"), static_cast<double>(voiceSize));
    attrs.insert(QStringLiteral("createTime"), nullableString(createTime));
    attrs.insert(QStringLiteral("title"), nullableString(title));
    attrs.insert(QStringLiteral("text"), nullableString(text));
    attrs.insert(QStringLiteral("translateUnfold"), translateUnfold);

    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("voiceBlock"));
    node.insert(QStringLiteral("attrs"), attrs);
    return node;
}

QJsonObject MigrationJsonBuilder::makeBulletList(const QJsonArray &content)
{
    return makeContainerNode(QStringLiteral("bulletList"), content);
}

QJsonObject MigrationJsonBuilder::makeOrderedList(const QJsonArray &content)
{
    return makeContainerNode(QStringLiteral("orderedList"), content);
}

QJsonObject MigrationJsonBuilder::makeListItem(const QJsonArray &content)
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("listItem"));
    node.insert(QStringLiteral("content"), normalizedListItemContent(content));
    return node;
}

QJsonObject MigrationJsonBuilder::makeTaskList(const QJsonArray &content)
{
    return makeContainerNode(QStringLiteral("taskList"), content);
}

QJsonObject MigrationJsonBuilder::makeTaskItem(bool checked, const QJsonArray &content)
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("taskItem"));
    node.insert(QStringLiteral("attrs"), QJsonObject { { QStringLiteral("checked"), checked } });
    node.insert(QStringLiteral("content"), normalizedListItemContent(content));
    return node;
}

QString MigrationJsonBuilder::toCompactJson(const QJsonObject &object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonValue MigrationJsonBuilder::nullableString(const QString &value)
{
    return value.isNull() || value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

QJsonArray MigrationJsonBuilder::normalizedDocContent(const QJsonArray &content)
{
    if (!content.isEmpty()) {
        return content;
    }

    return QJsonArray { makeParagraph() };
}

QJsonArray MigrationJsonBuilder::normalizedListItemContent(const QJsonArray &content)
{
    if (!content.isEmpty()) {
        return content;
    }

    return QJsonArray { makeParagraph() };
}

QJsonObject MigrationJsonBuilder::makeContainerNode(const QString &type, const QJsonArray &content)
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), type);
    if (!content.isEmpty()) {
        node.insert(QStringLiteral("content"), content);
    }
    return node;
}
