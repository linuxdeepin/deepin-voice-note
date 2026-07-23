// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationhtmlconverter.h"

#include "migrationhtmlparser.h"
#include "migrationjsonbuilder.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

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

bool isListElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element
        && (node.tagName == QStringLiteral("ul") || node.tagName == QStringLiteral("ol"));
}

bool isListItemElement(const MigrationHtmlNode &node)
{
    return node.type == MigrationHtmlNodeType::Element && node.tagName == QStringLiteral("li");
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
               QStringLiteral("unsupported-html-style"),
               QStringLiteral("HTML style '%1' is not supported by this migration step and was ignored").arg(property));
}

void warnInvalidStyleValue(const MigrationHtmlNode &node,
                           const QString &property,
                           MigrationHtmlConversionResult &result)
{
    addWarning(result,
               elementPath(node) + QStringLiteral(".style.%1").arg(property),
               QStringLiteral("invalid-html-style-value"),
               QStringLiteral("HTML style '%1' has an unsupported value and was ignored").arg(property));
}

void warnDowngradedTextAlign(const MigrationHtmlNode &node,
                             const QString &value,
                             MigrationHtmlConversionResult &result)
{
    addWarning(result,
               elementPath(node) + QStringLiteral(".style.text-align"),
               QStringLiteral("downgraded-text-align"),
               QStringLiteral("HTML text-align '%1' is not supported by Schema V1 and was ignored").arg(value));
}

void warnInvalidListChild(const MigrationHtmlNode &node, MigrationHtmlConversionResult &result)
{
    addWarning(result,
               elementPath(node),
               QStringLiteral("downgraded-html-list-child"),
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

    if (node.tagName == QStringLiteral("br")) {
        appendHardBreak(content);
        return;
    }

    if (isBlockElement(node) && !content.isEmpty() && !isHardBreakNode(content.at(content.size() - 1))) {
        appendHardBreak(content);
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

        if (isBlockElement(child)) {
            if (!inlineContent.isEmpty()) {
                appendParagraphIfContent(inlineContent, blocks);
            }
            blocks.append(blockFromElement(child, result, inheritedMarks));
            continue;
        }

        appendInlineNode(child, inlineContent, inheritedMarks, result);
    }

    if (!inlineContent.isEmpty()) {
        appendParagraphIfContent(inlineContent, blocks);
    }
}

QJsonObject downgradedParagraphFromBlock(const MigrationHtmlNode &node,
                                         MigrationHtmlConversionResult &result,
                                         const QJsonArray &inheritedMarks = QJsonArray())
{
    if (node.tagName != QStringLiteral("p") && node.tagName != QStringLiteral("div")) {
        addWarning(result,
                   QStringLiteral("/%1").arg(node.tagName),
                   QStringLiteral("downgraded-html-block"),
                   QStringLiteral("HTML block <%1> was downgraded to paragraph").arg(node.tagName));
    }

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

        if (isBlockElement(child)) {
            appendInlineContentAsParagraph(inlineContent, content);
            content.append(blockFromElement(child, result, inheritedMarks));
            continue;
        }

        appendInlineNode(child, inlineContent, inheritedMarks, result);
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
        return MigrationJsonBuilder::makeListItem(QJsonArray { blockFromElement(node, result, parentMarks) });
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
            items.append(MigrationJsonBuilder::makeListItem(
                listItemContentFromElement(child, result, listMarks)));
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

QJsonObject blockFromElement(const MigrationHtmlNode &node,
                             MigrationHtmlConversionResult &result,
                             const QJsonArray &inheritedMarks)
{
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
                   QStringLiteral("downgraded-orphan-list-item"),
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
