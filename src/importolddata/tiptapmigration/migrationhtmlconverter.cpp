// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationhtmlconverter.h"

#include "migrationhtmlparser.h"
#include "migrationjsonbuilder.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QSet>

namespace {

const QSet<QString> &dangerousTags()
{
    static const QSet<QString> tags {
        QStringLiteral("script"),
        QStringLiteral("iframe"),
        QStringLiteral("object"),
        QStringLiteral("embed")
    };
    return tags;
}

const QSet<QString> &blockTags()
{
    static const QSet<QString> tags {
        QStringLiteral("address"),
        QStringLiteral("article"),
        QStringLiteral("aside"),
        QStringLiteral("blockquote"),
        QStringLiteral("dd"),
        QStringLiteral("div"),
        QStringLiteral("dl"),
        QStringLiteral("dt"),
        QStringLiteral("figcaption"),
        QStringLiteral("figure"),
        QStringLiteral("footer"),
        QStringLiteral("h1"),
        QStringLiteral("h2"),
        QStringLiteral("h3"),
        QStringLiteral("h4"),
        QStringLiteral("h5"),
        QStringLiteral("h6"),
        QStringLiteral("header"),
        QStringLiteral("hr"),
        QStringLiteral("li"),
        QStringLiteral("main"),
        QStringLiteral("ol"),
        QStringLiteral("p"),
        QStringLiteral("pre"),
        QStringLiteral("section"),
        QStringLiteral("table"),
        QStringLiteral("td"),
        QStringLiteral("th"),
        QStringLiteral("tr"),
        QStringLiteral("ul")
    };
    return tags;
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

void copyParseIssues(const MigrationHtmlParseResult &parsed, MigrationHtmlConversionResult &result)
{
    for (const MigrationHtmlParseWarning &warning : parsed.warnings) {
        if (warning.code == QStringLiteral("parse-failed")) {
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

bool isHeadingElement(const MigrationHtmlNode &node)
{
    if (node.type != MigrationHtmlNodeType::Element || node.tagName.size() != 2 || node.tagName.at(0) != QLatin1Char('h')) {
        return false;
    }

    const QChar level = node.tagName.at(1);
    return level >= QLatin1Char('1') && level <= QLatin1Char('6');
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

bool contentEndsWithSpace(const QJsonArray &content)
{
    if (content.isEmpty() || !isTextNode(content.at(content.size() - 1))) {
        return false;
    }

    return content.at(content.size() - 1).toObject().value(QStringLiteral("text")).toString().endsWith(QLatin1Char(' '));
}

void appendTextNode(QJsonArray &content, const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    if (!content.isEmpty() && isTextNode(content.at(content.size() - 1))) {
        QJsonObject last = content.at(content.size() - 1).toObject();
        last.insert(QStringLiteral("text"), last.value(QStringLiteral("text")).toString() + text);
        content.replace(content.size() - 1, last);
        return;
    }

    content.append(MigrationJsonBuilder::makeText(text));
}

void appendVisibleText(QJsonArray &content, const QString &text)
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

    appendTextNode(content, normalized);
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

void appendInlineNode(const MigrationHtmlNode &node, QJsonArray &content, MigrationHtmlConversionResult &result);

void appendInlineChildren(const MigrationHtmlNode &node, QJsonArray &content, MigrationHtmlConversionResult &result)
{
    for (const MigrationHtmlNode &child : node.children) {
        appendInlineNode(child, content, result);
    }
}

void appendInlineNode(const MigrationHtmlNode &node, QJsonArray &content, MigrationHtmlConversionResult &result)
{
    if (node.type == MigrationHtmlNodeType::Text) {
        appendVisibleText(content, node.text);
        return;
    }

    if (node.type != MigrationHtmlNodeType::Element) {
        appendInlineChildren(node, content, result);
        return;
    }

    if (isDangerousElement(node)) {
        return;
    }

    if (node.tagName == QStringLiteral("br")) {
        appendHardBreak(content);
        return;
    }

    if (isBlockElement(node) && !content.isEmpty() && !isHardBreakNode(content.at(content.size() - 1))) {
        appendHardBreak(content);
    }

    appendInlineChildren(node, content, result);
}

void appendParagraphIfContent(QJsonArray &inlineContent, QJsonArray &blocks)
{
    trimTrailingTextSpace(inlineContent);
    if (!inlineContent.isEmpty()) {
        blocks.append(MigrationJsonBuilder::makeParagraph(inlineContent));
    }
    inlineContent = QJsonArray();
}

QJsonArray inlineContentFrom(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    QJsonArray content;
    appendInlineChildren(node, content, result);
    trimTrailingTextSpace(content);
    return content;
}

QJsonObject blockFromElement(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result);

void appendBlocksFromChildren(const MigrationHtmlNode &node, QJsonArray &blocks, MigrationHtmlConversionResult &result)
{
    QJsonArray inlineContent;
    for (const MigrationHtmlNode &child : node.children) {
        if (isDangerousElement(child)) {
            continue;
        }

        if (isBlockElement(child)) {
            if (!inlineContent.isEmpty()) {
                appendParagraphIfContent(inlineContent, blocks);
            }
            blocks.append(blockFromElement(child, result));
            continue;
        }

        appendInlineNode(child, inlineContent, result);
    }

    if (!inlineContent.isEmpty()) {
        appendParagraphIfContent(inlineContent, blocks);
    }
}

QJsonObject downgradedParagraphFromBlock(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    if (node.tagName != QStringLiteral("p") && node.tagName != QStringLiteral("div")) {
        addWarning(result,
                   QStringLiteral("/%1").arg(node.tagName),
                   QStringLiteral("downgraded-html-block"),
                   QStringLiteral("HTML block <%1> was downgraded to paragraph").arg(node.tagName));
    }

    return MigrationJsonBuilder::makeParagraph(inlineContentFrom(node, result));
}

QJsonObject blockquoteFromElement(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    QJsonArray content;
    appendBlocksFromChildren(node, content, result);
    return MigrationJsonBuilder::makeBlockquote(content);
}

QJsonObject blockFromElement(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    if (isHeadingElement(node)) {
        return MigrationJsonBuilder::makeHeading(headingLevel(node), inlineContentFrom(node, result));
    }

    if (node.tagName == QStringLiteral("blockquote")) {
        return blockquoteFromElement(node, result);
    }

    return downgradedParagraphFromBlock(node, result);
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
