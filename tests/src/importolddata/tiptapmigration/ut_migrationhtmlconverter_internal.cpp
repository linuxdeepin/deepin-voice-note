// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>

// Include the .cpp directly to access anonymous-namespace functions
// (inlineContentFrom, downgradedParagraphFromBlock) that are unreachable
// through the public API MigrationHtmlConverter::convert().
// The .cpp is excluded from VNOTE_SRC_TEST in CMakeLists.txt to avoid
// duplicate symbol errors.
#include "migrationhtmlconverter.cpp"

namespace {

MigrationHtmlNode makeTextNode(const QString &text)
{
    MigrationHtmlNode node;
    node.type = MigrationHtmlNodeType::Text;
    node.text = text;
    return node;
}

MigrationHtmlNode makeElementNode(const QString &tagName,
                                  const QVector<MigrationHtmlNode> &children = {})
{
    MigrationHtmlNode node;
    node.type = MigrationHtmlNodeType::Element;
    node.tagName = tagName;
    node.children = children;
    return node;
}

} // namespace

// --- inlineContentFrom ---

TEST(UT_MigrationHtmlConverterInternal, InlineContentFromSpanWithText)
{
    MigrationHtmlNode child = makeTextNode(QStringLiteral("inline text"));
    MigrationHtmlNode node = makeElementNode(QStringLiteral("span"), {child});
    MigrationHtmlConversionResult result;
    QJsonArray content = inlineContentFrom(node, result);
    EXPECT_FALSE(content.isEmpty());
}

TEST(UT_MigrationHtmlConverterInternal, InlineContentFromDisplayAffectingTag)
{
    MigrationHtmlNode child = makeTextNode(QStringLiteral("marked"));
    MigrationHtmlNode node = makeElementNode(QStringLiteral("mark"), {child});
    MigrationHtmlConversionResult result;
    QJsonArray content = inlineContentFrom(node, result);
    EXPECT_FALSE(content.isEmpty());
}

TEST(UT_MigrationHtmlConverterInternal, InlineContentFromBlockElement)
{
    MigrationHtmlNode child = makeTextNode(QStringLiteral("block text"));
    MigrationHtmlNode node = makeElementNode(QStringLiteral("div"), {child});
    MigrationHtmlConversionResult result;
    QJsonArray content = inlineContentFrom(node, result);
    EXPECT_FALSE(content.isEmpty());
}

TEST(UT_MigrationHtmlConverterInternal, InlineContentFromEmptyElement)
{
    MigrationHtmlNode node = makeElementNode(QStringLiteral("span"));
    MigrationHtmlConversionResult result;
    QJsonArray content = inlineContentFrom(node, result);
    EXPECT_TRUE(content.isEmpty());
}

// --- downgradedParagraphFromBlock ---

TEST(UT_MigrationHtmlConverterInternal, DowngradedParagraphFromBlockWithText)
{
    MigrationHtmlNode child = makeTextNode(QStringLiteral("paragraph text"));
    MigrationHtmlNode node = makeElementNode(QStringLiteral("section"), {child});
    MigrationHtmlConversionResult result;
    QJsonObject block = downgradedParagraphFromBlock(node, result);
    EXPECT_FALSE(block.isEmpty());
}

TEST(UT_MigrationHtmlConverterInternal, DowngradedParagraphFromBlockEmpty)
{
    MigrationHtmlNode node = makeElementNode(QStringLiteral("section"));
    MigrationHtmlConversionResult result;
    QJsonObject block = downgradedParagraphFromBlock(node, result);
    EXPECT_FALSE(block.isEmpty());
}

TEST(UT_MigrationHtmlConverterInternal, DowngradedParagraphFromBlockWithHeadingChild)
{
    MigrationHtmlNode child = makeTextNode(QStringLiteral("heading text"));
    MigrationHtmlNode node = makeElementNode(QStringLiteral("h2"), {child});
    MigrationHtmlConversionResult result;
    QJsonObject block = downgradedParagraphFromBlock(node, result);
    EXPECT_FALSE(block.isEmpty());
}
