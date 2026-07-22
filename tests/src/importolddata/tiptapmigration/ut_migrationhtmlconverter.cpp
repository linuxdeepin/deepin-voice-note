// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/tiptapmigration/migrationhtmlconverter.h"
#include "importolddata/tiptapmigration/migrationjsonvalidator.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>

namespace {

QJsonArray docContentOf(const MigrationHtmlConversionResult &result)
{
    return result.envelope.value(QStringLiteral("content")).toObject().value(QStringLiteral("content")).toArray();
}

QJsonArray nodeContentOf(const QJsonObject &node)
{
    return node.value(QStringLiteral("content")).toArray();
}

QJsonObject attrsOf(const QJsonObject &node)
{
    return node.value(QStringLiteral("attrs")).toObject();
}

QString nodeTypeOf(const QJsonObject &node)
{
    return node.value(QStringLiteral("type")).toString();
}

QString textOf(const QJsonObject &node)
{
    return node.value(QStringLiteral("text")).toString();
}

bool hasWarningCode(const MigrationHtmlConversionResult &result, const QString &code)
{
    for (const MigrationHtmlConversionIssue &warning : result.warnings) {
        if (warning.code == code) {
            return true;
        }
    }

    return false;
}

void expectEnvelopeValid(const MigrationHtmlConversionResult &result)
{
    const MigrationJsonValidationResult validation = MigrationJsonValidator::validateEnvelope(result.envelope);
    EXPECT_TRUE(validation.ok()) << (validation.errors.isEmpty()
                                        ? std::string()
                                        : validation.errors.constFirst().message.toStdString());
}

} // namespace

TEST(UT_MigrationHtmlConverter, ConvertsParagraphDivAndHardBreaks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("\n  <p>Hello<br>World</p>\n  <div>Second</div>\n"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(2, blocks.size());

    const QJsonObject firstParagraph = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(firstParagraph));
    const QJsonArray firstContent = nodeContentOf(firstParagraph);
    ASSERT_EQ(3, firstContent.size());
    EXPECT_EQ(QStringLiteral("Hello"), textOf(firstContent.at(0).toObject()));
    EXPECT_EQ(QStringLiteral("hardBreak"), nodeTypeOf(firstContent.at(1).toObject()));
    EXPECT_EQ(QStringLiteral("World"), textOf(firstContent.at(2).toObject()));

    const QJsonObject secondParagraph = blocks.at(1).toObject();
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(secondParagraph));
    ASSERT_EQ(1, nodeContentOf(secondParagraph).size());
    EXPECT_EQ(QStringLiteral("Second"), textOf(nodeContentOf(secondParagraph).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, ConvertsHeadingsAndBlockquotes)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<h2>Title</h2><blockquote><p>Quote</p><h3>Nested</h3></blockquote>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(2, blocks.size());

    const QJsonObject heading = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("heading"), nodeTypeOf(heading));
    EXPECT_EQ(2, attrsOf(heading).value(QStringLiteral("level")).toInt());
    ASSERT_EQ(1, nodeContentOf(heading).size());
    EXPECT_EQ(QStringLiteral("Title"), textOf(nodeContentOf(heading).at(0).toObject()));

    const QJsonObject blockquote = blocks.at(1).toObject();
    EXPECT_EQ(QStringLiteral("blockquote"), nodeTypeOf(blockquote));
    const QJsonArray quoteBlocks = nodeContentOf(blockquote);
    ASSERT_EQ(2, quoteBlocks.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(quoteBlocks.at(0).toObject()));
    EXPECT_EQ(QStringLiteral("Quote"), textOf(nodeContentOf(quoteBlocks.at(0).toObject()).at(0).toObject()));
    EXPECT_EQ(QStringLiteral("heading"), nodeTypeOf(quoteBlocks.at(1).toObject()));
    EXPECT_EQ(3, attrsOf(quoteBlocks.at(1).toObject()).value(QStringLiteral("level")).toInt());
}

TEST(UT_MigrationHtmlConverter, KeepsTopLevelInlineTextInOneParagraph)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("Hello <span>world</span><br>next"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonArray content = nodeContentOf(blocks.at(0).toObject());
    ASSERT_EQ(3, content.size());
    EXPECT_EQ(QStringLiteral("Hello world"), textOf(content.at(0).toObject()));
    EXPECT_EQ(QStringLiteral("hardBreak"), nodeTypeOf(content.at(1).toObject()));
    EXPECT_EQ(QStringLiteral("next"), textOf(content.at(2).toObject()));
}

TEST(UT_MigrationHtmlConverter, NormalizesEmptyDocument)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(QString());

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(0).toObject()));
    EXPECT_TRUE(nodeContentOf(blocks.at(0).toObject()).isEmpty());
}

TEST(UT_MigrationHtmlConverter, DowngradesUnknownBlockTagsToParagraph)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<section><custom>Visible</custom></section>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("downgraded-html-block")));

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(0).toObject()));
    ASSERT_EQ(1, nodeContentOf(blocks.at(0).toObject()).size());
    EXPECT_EQ(QStringLiteral("Visible"), textOf(nodeContentOf(blocks.at(0).toObject()).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, SkipsDangerousNodesAndTheirContent)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p>safe<script>alert(1)</script><iframe>frame</iframe>tail</p><object>bad</object>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("dangerous-html-node")));

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonArray content = nodeContentOf(blocks.at(0).toObject());
    ASSERT_EQ(1, content.size());
    EXPECT_EQ(QStringLiteral("safetail"), textOf(content.at(0).toObject()));
}
