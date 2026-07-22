// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/tiptapmigration/migrationhtmlparser.h"

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>

namespace {

const MigrationHtmlNode *findFirstByTag(const MigrationHtmlNode &node, const QString &tagName)
{
    if (node.type == MigrationHtmlNodeType::Element && node.tagName == tagName) {
        return &node;
    }

    for (const MigrationHtmlNode &child : node.children) {
        const MigrationHtmlNode *found = findFirstByTag(child, tagName);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

int countByTag(const MigrationHtmlNode &node, const QString &tagName)
{
    int count = node.type == MigrationHtmlNodeType::Element && node.tagName == tagName ? 1 : 0;
    for (const MigrationHtmlNode &child : node.children) {
        count += countByTag(child, tagName);
    }
    return count;
}

bool hasWarningCode(const MigrationHtmlParseResult &result, const QString &code)
{
    for (const MigrationHtmlParseWarning &warning : result.warnings) {
        if (warning.code == code) {
            return true;
        }
    }
    return false;
}

int countTextNodes(const MigrationHtmlNode &node)
{
    int count = node.type == MigrationHtmlNodeType::Text ? 1 : 0;
    for (const MigrationHtmlNode &child : node.children) {
        count += countTextNodes(child);
    }
    return count;
}

} // namespace

TEST(UT_MigrationHtmlParser, PreservesStyleEntityAndUnicodeText)
{
    const MigrationHtmlParseResult result = MigrationHtmlParser::parse(
        QStringLiteral("<p style=\"font-weight:bold;color:#ff0000\">"
                       "<span style=\"font-size:18px\">Hello&nbsp;中文🙂</span></p>"));

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.warnings.isEmpty());
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("Hello")));
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("中文🙂")));

    const MigrationHtmlNode *paragraph = findFirstByTag(result.root, QStringLiteral("p"));
    ASSERT_NE(nullptr, paragraph);
    EXPECT_EQ(QStringLiteral("font-weight:bold;color:#ff0000"),
              MigrationHtmlParser::attribute(*paragraph, QStringLiteral("style")));

    const MigrationHtmlNode *span = findFirstByTag(result.root, QStringLiteral("span"));
    ASSERT_NE(nullptr, span);
    EXPECT_EQ(QStringLiteral("font-size:18px"), MigrationHtmlParser::attribute(*span, QStringLiteral("style")));
}

TEST(UT_MigrationHtmlParser, PreservesImageAttributes)
{
    const MigrationHtmlParseResult result = MigrationHtmlParser::parse(
        QStringLiteral("<p><img src=\"/home/u/images/a.png\" data-rel-path=\"images/a.png\" "
                       "alt=\"preview\" title=\"photo\"></p>"));

    EXPECT_TRUE(result.ok());
    const MigrationHtmlNode *image = findFirstByTag(result.root, QStringLiteral("img"));
    ASSERT_NE(nullptr, image);
    EXPECT_EQ(QStringLiteral("/home/u/images/a.png"), MigrationHtmlParser::attribute(*image, QStringLiteral("src")));
    EXPECT_EQ(QStringLiteral("images/a.png"), MigrationHtmlParser::attribute(*image, QStringLiteral("data-rel-path")));
    EXPECT_EQ(QStringLiteral("preview"), MigrationHtmlParser::attribute(*image, QStringLiteral("alt")));
    EXPECT_EQ(QStringLiteral("photo"), MigrationHtmlParser::attribute(*image, QStringLiteral("title")));
}

TEST(UT_MigrationHtmlParser, PreservesVoiceBoxJsonKeyCaseInsensitively)
{
    const QString jsonKey = QStringLiteral(
        "{&quot;voiceId&quot;:&quot;v1&quot;,&quot;voicePath&quot;:&quot;voicenote/a.mp3&quot;,&quot;text&quot;:&quot;你好&quot;}");
    const MigrationHtmlParseResult result = MigrationHtmlParser::parse(
        QStringLiteral("<div class=\"li voiceBox\" contenteditable=\"false\" jsonKey=\"%1\">"
                       "<span>00:01</span></div>")
            .arg(jsonKey));

    EXPECT_TRUE(result.ok());
    const MigrationHtmlNode *voiceBox = findFirstByTag(result.root, QStringLiteral("div"));
    ASSERT_NE(nullptr, voiceBox);
    EXPECT_TRUE(MigrationHtmlParser::hasClass(*voiceBox, QStringLiteral("voiceBox")));
    EXPECT_FALSE(MigrationHtmlParser::hasClass(*voiceBox, QStringLiteral("missingClass")));
    EXPECT_EQ(QStringLiteral("false"), MigrationHtmlParser::attribute(*voiceBox, QStringLiteral("contentEditable")));
    EXPECT_TRUE(MigrationHtmlParser::attribute(*voiceBox, QStringLiteral("missing-attr")).isEmpty());

    const QString storedJsonKey = MigrationHtmlParser::attribute(*voiceBox, QStringLiteral("jsonKey"));
    EXPECT_TRUE(storedJsonKey.contains(QStringLiteral("\"voiceId\":\"v1\"")));
    EXPECT_TRUE(storedJsonKey.contains(QStringLiteral("\"voicePath\":\"voicenote/a.mp3\"")));

    const QJsonDocument parsed = QJsonDocument::fromJson(storedJsonKey.toUtf8());
    ASSERT_TRUE(parsed.isObject());
    EXPECT_EQ(QStringLiteral("你好"), parsed.object().value(QStringLiteral("text")).toString());
}

TEST(UT_MigrationHtmlParser, PreservesNestedListHierarchy)
{
    const MigrationHtmlParseResult result = MigrationHtmlParser::parse(
        QStringLiteral("<ol><li>one<ul><li>child</li></ul></li><li>two</li></ol>"));

    EXPECT_TRUE(result.ok());
    const MigrationHtmlNode *orderedList = findFirstByTag(result.root, QStringLiteral("ol"));
    ASSERT_NE(nullptr, orderedList);
    EXPECT_EQ(3, countByTag(*orderedList, QStringLiteral("li")));

    const MigrationHtmlNode *nestedList = findFirstByTag(*orderedList, QStringLiteral("ul"));
    ASSERT_NE(nullptr, nestedList);
    EXPECT_EQ(1, countByTag(*nestedList, QStringLiteral("li")));
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("one")));
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("child")));
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("two")));
}

TEST(UT_MigrationHtmlParser, PreservesInlineWhitespaceAndBlockPlainTextBoundaries)
{
    const MigrationHtmlParseResult inlineResult = MigrationHtmlParser::parse(
        QStringLiteral("<span>A</span> <span>B</span>"));

    EXPECT_TRUE(inlineResult.ok());
    EXPECT_EQ(QStringLiteral("A B"), inlineResult.plainText);
    EXPECT_EQ(3, countTextNodes(inlineResult.root));

    const MigrationHtmlParseResult blockResult = MigrationHtmlParser::parse(
        QStringLiteral("<p>C</p><p>D</p><div>E<br>F</div>"));

    EXPECT_TRUE(blockResult.ok());
    EXPECT_EQ(QStringLiteral("C\nD\nE\nF\n"), blockResult.plainText);
}

TEST(UT_MigrationHtmlParser, RecoversInvalidHtmlAndWarnsDangerousNodes)
{
    const MigrationHtmlParseResult result = MigrationHtmlParser::parse(
        QStringLiteral("<div><p>broken<span>still text</div><script>alert(1)</script>tail"));

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("broken")));
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("still text")));
    EXPECT_TRUE(result.plainText.contains(QStringLiteral("tail")));
    EXPECT_FALSE(result.plainText.contains(QStringLiteral("alert(1)")));
    EXPECT_NE(nullptr, findFirstByTag(result.root, QStringLiteral("script")));
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("dangerous-html-node")));
}

TEST(UT_MigrationHtmlParser, WarnsAndStopsAtExcessiveDepth)
{
    QString html;
    for (int index = 0; index < 105; ++index) {
        html.append(QStringLiteral("<div>"));
    }
    html.append(QStringLiteral("leaf"));
    for (int index = 0; index < 105; ++index) {
        html.append(QStringLiteral("</div>"));
    }

    const MigrationHtmlParseResult result = MigrationHtmlParser::parse(html);

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("depth-exceeded")));
}

TEST(UT_MigrationHtmlParser, HandlesEmptyAndPlainTextInputs)
{
    const MigrationHtmlParseResult empty = MigrationHtmlParser::parse(QString());
    EXPECT_TRUE(empty.ok());
    EXPECT_TRUE(empty.root.children.isEmpty());
    EXPECT_TRUE(empty.plainText.isEmpty());

    const MigrationHtmlParseResult plain = MigrationHtmlParser::parse(QStringLiteral("plain 中文🙂"));
    EXPECT_TRUE(plain.ok());
    EXPECT_TRUE(plain.plainText.contains(QStringLiteral("plain 中文🙂")));
}
