// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationhtmlparser.h"

#include "migrationhtmlcodes.h"

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

#include <QByteArray>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <memory>

namespace {
constexpr int kMaxHtmlNodeDepth = 100;

struct HtmlDocDeleter {
    void operator()(htmlDocPtr doc) const
    {
        if (doc) {
            xmlFreeDoc(doc);
        }
    }
};

using HtmlDocPtr = std::unique_ptr<xmlDoc, HtmlDocDeleter>;

QString xmlStringToQString(const xmlChar *value)
{
    if (!value) {
        return QString();
    }

    return QString::fromUtf8(reinterpret_cast<const char *>(value));
}

QString nodeName(xmlNodePtr node)
{
    if (!node || !node->name) {
        return QString();
    }

    return xmlStringToQString(node->name).toLower();
}

QString nodeContent(xmlNodePtr node)
{
    xmlChar *content = xmlNodeGetContent(node);
    const QString result = xmlStringToQString(content);
    if (content) {
        xmlFree(content);
    }
    return result;
}

QString attributeValue(xmlNodePtr node, xmlAttrPtr attribute)
{
    if (!node || !attribute || !attribute->children) {
        return QString();
    }

    xmlChar *value = xmlNodeListGetString(node->doc, attribute->children, 1);
    const QString result = xmlStringToQString(value);
    if (value) {
        xmlFree(value);
    }
    return result;
}

QMap<QString, QString> attributesFor(xmlNodePtr node)
{
    QMap<QString, QString> attributes;
    for (xmlAttrPtr attribute = node->properties; attribute; attribute = attribute->next) {
        if (!attribute->name) {
            continue;
        }

        const QString name = xmlStringToQString(attribute->name).toLower();
        attributes.insert(name, attributeValue(node, attribute));
    }
    return attributes;
}

QString childPath(const QString &parentPath, const QString &name, int index)
{
    return parentPath + QStringLiteral("/") + name + QStringLiteral("[") + QString::number(index) + QStringLiteral("]");
}

bool isBlockElement(const QString &tagName)
{
    return blockTags().contains(tagName);
}

void appendLineBreak(QString *plainText)
{
    while (plainText->endsWith(QLatin1Char(' '))) {
        plainText->chop(1);
    }

    if (!plainText->isEmpty() && !plainText->endsWith(QLatin1Char('\n'))) {
        plainText->append(QLatin1Char('\n'));
    }
}

void appendText(const QString &text, QString *plainText)
{
    bool pendingSpace = false;
    for (const QChar &character : text) {
        if (character.isSpace()) {
            pendingSpace = true;
            continue;
        }

        if (pendingSpace && !plainText->isEmpty()
            && !plainText->endsWith(QLatin1Char(' '))
            && !plainText->endsWith(QLatin1Char('\n'))) {
            plainText->append(QLatin1Char(' '));
        }

        plainText->append(character);
        pendingSpace = false;
    }

    if (pendingSpace && !plainText->isEmpty()
        && !plainText->endsWith(QLatin1Char(' '))
        && !plainText->endsWith(QLatin1Char('\n'))) {
        plainText->append(QLatin1Char(' '));
    }
}

void appendPlainText(const MigrationHtmlNode &node, QString *plainText, int depth = 0)
{
    if (depth > kMaxHtmlNodeDepth) {
        return;
    }

    if (node.type == MigrationHtmlNodeType::Text) {
        appendText(node.text, plainText);
        return;
    }

    if (node.type == MigrationHtmlNodeType::Element && dangerousTags().contains(node.tagName)) {
        return;
    }

    if (node.type == MigrationHtmlNodeType::Element && node.tagName == QStringLiteral("br")) {
        appendLineBreak(plainText);
        return;
    }

    const bool block = node.type == MigrationHtmlNodeType::Element && isBlockElement(node.tagName);
    if (block) {
        appendLineBreak(plainText);
    }

    for (const MigrationHtmlNode &child : node.children) {
        appendPlainText(child, plainText, depth + 1);
    }

    if (block) {
        appendLineBreak(plainText);
    }
}

bool isDangerousAttribute(const QString &name, const QString &value)
{
    if (name.startsWith(QStringLiteral("on")) && name.size() > 2) {
        return true;
    }

    if (name == QStringLiteral("src")) {
        return false;
    }

    return isUnsafeUrlScheme(value);
}

MigrationHtmlNode convertNode(xmlNodePtr node,
                              const QString &path,
                              QVector<MigrationHtmlParseWarning> *warnings,
                              int depth = 0)
{
    MigrationHtmlNode converted;

    if (node->type == XML_TEXT_NODE || node->type == XML_CDATA_SECTION_NODE) {
        converted.type = MigrationHtmlNodeType::Text;
        converted.text = nodeContent(node);
        return converted;
    }

    converted.type = MigrationHtmlNodeType::Element;
    converted.tagName = nodeName(node);
    converted.attributes = attributesFor(node);

    if (depth > kMaxHtmlNodeDepth) {
        warnings->append(MigrationHtmlParseWarning {
            path,
            kCodeDepthExceeded,
            QStringLiteral("HTML node depth must not exceed %1.").arg(kMaxHtmlNodeDepth)
        });
        return converted;
    }

    if (dangerousTags().contains(converted.tagName)) {
        warnings->append(MigrationHtmlParseWarning {
            path,
            kCodeDangerousHtmlNode,
            QStringLiteral("Dangerous HTML node should be downgraded by migration converter.")
        });
    } else {
        QStringList dangerousAttrNames;
        for (auto it = converted.attributes.constBegin(); it != converted.attributes.constEnd(); ++it) {
            if (isDangerousAttribute(it.key(), it.value())) {
                dangerousAttrNames.append(it.key());
                warnings->append(MigrationHtmlParseWarning {
                    path + QStringLiteral(".attrs.") + it.key(),
                    kCodeDangerousHtmlAttribute,
                    QStringLiteral("Dangerous HTML attribute was removed during migration.")
                });
            }
        }
        for (const QString &name : dangerousAttrNames) {
            converted.attributes.remove(name);
        }
    }

    int elementIndex = 0;
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) {
            MigrationHtmlNode textNode;
            textNode.type = MigrationHtmlNodeType::Text;
            textNode.text = nodeContent(child);
            converted.children.append(textNode);
            continue;
        }

        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }

        const QString tagName = nodeName(child);
        converted.children.append(convertNode(child, childPath(path, tagName, elementIndex), warnings, depth + 1));
        ++elementIndex;
    }

    return converted;
}

xmlNodePtr firstElementChildByName(xmlNodePtr node, const QString &name)
{
    for (xmlNodePtr child = node ? node->children : nullptr; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && nodeName(child) == name) {
            return child;
        }
    }
    return nullptr;
}

} // namespace

bool MigrationHtmlParseResult::ok() const
{
    for (const MigrationHtmlParseWarning &warning : warnings) {
        if (warning.code == kCodeParseFailed) {
            return false;
        }
    }
    return true;
}

MigrationHtmlParseResult MigrationHtmlParser::parse(const QString &html)
{
    MigrationHtmlParseResult result;
    result.root.type = MigrationHtmlNodeType::Document;
    result.root.tagName = QStringLiteral("#document");

    if (html.isEmpty()) {
        return result;
    }

    const QByteArray bytes = html.toUtf8();
    HtmlDocPtr doc(htmlReadMemory(bytes.constData(),
                                  bytes.size(),
                                  "migration-html.html",
                                  "UTF-8",
                                  HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET));
    if (!doc) {
        result.warnings.append(MigrationHtmlParseWarning {
            QStringLiteral("/"),
            kCodeParseFailed,
            QStringLiteral("Failed to parse legacy HTML.")
        });
        result.plainText = html;
        return result;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc.get());
    xmlNodePtr body = firstElementChildByName(root, QStringLiteral("body"));
    xmlNodePtr parseRoot = body ? body : root;

    int elementIndex = 0;
    for (xmlNodePtr child = parseRoot ? parseRoot->children : nullptr; child; child = child->next) {
        if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) {
            MigrationHtmlNode textNode;
            textNode.type = MigrationHtmlNodeType::Text;
            textNode.text = nodeContent(child);
            result.root.children.append(textNode);
            continue;
        }

        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }

        const QString tagName = nodeName(child);
        result.root.children.append(convertNode(
            child, childPath(QStringLiteral("/document"), tagName, elementIndex), &result.warnings));
        ++elementIndex;
    }

    appendPlainText(result.root, &result.plainText);
    return result;
}

QString MigrationHtmlParser::attribute(const MigrationHtmlNode &node, const QString &name)
{
    return node.attributes.value(name.toLower());
}

bool MigrationHtmlParser::hasClass(const MigrationHtmlNode &node, const QString &className)
{
    static const QRegularExpression classSeparator(QStringLiteral("\\s+"));
    const QStringList classes = attribute(node, QStringLiteral("class")).split(classSeparator, Qt::SkipEmptyParts);
    return classes.contains(className, Qt::CaseInsensitive);
}
