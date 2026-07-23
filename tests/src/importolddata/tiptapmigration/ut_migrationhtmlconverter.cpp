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

QJsonArray marksOf(const QJsonObject &node)
{
    return node.value(QStringLiteral("marks")).toArray();
}

QJsonObject markOfType(const QJsonObject &node, const QString &type)
{
    for (const QJsonValue &markValue : marksOf(node)) {
        const QJsonObject mark = markValue.toObject();
        if (mark.value(QStringLiteral("type")).toString() == type) {
            return mark;
        }
    }

    return QJsonObject();
}

bool hasMark(const QJsonObject &node, const QString &type)
{
    return !markOfType(node, type).isEmpty();
}

QJsonObject markAttrsOf(const QJsonObject &node, const QString &type)
{
    return markOfType(node, type).value(QStringLiteral("attrs")).toObject();
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


TEST(UT_MigrationHtmlConverter, ConvertsImageUsingDataRelPathFirst)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><img src=\"/home/u/images/ignored.png\" data-rel-path=\"images/photo.png\" "
                       "alt=\"preview\" title=\"Photo\"></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonObject image = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/photo.png"), attrsOf(image).value(QStringLiteral("src")).toString());
    EXPECT_EQ(QStringLiteral("images/photo.png"), attrsOf(image).value(QStringLiteral("relPath")).toString());
    EXPECT_EQ(QStringLiteral("preview"), attrsOf(image).value(QStringLiteral("alt")).toString());
    EXPECT_EQ(QStringLiteral("Photo"), attrsOf(image).value(QStringLiteral("title")).toString());
}

TEST(UT_MigrationHtmlConverter, ExtractsImageRelPathFromAbsoluteSrc)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<img src=\"/home/u/.local/share/deepin-voice-note/images/note/photo.png?cache=1\">"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonObject image = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/note/photo.png"), attrsOf(image).value(QStringLiteral("src")).toString());
    EXPECT_EQ(QStringLiteral("images/note/photo.png"), attrsOf(image).value(QStringLiteral("relPath")).toString());
}

TEST(UT_MigrationHtmlConverter, ExtractsImageRelPathFromFileUrl)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<img src=\"file:///home/u/.local/share/deepin-voice-note/images/note/photo.png#preview\">"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonObject image = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/note/photo.png"), attrsOf(image).value(QStringLiteral("src")).toString());
    EXPECT_EQ(QStringLiteral("images/note/photo.png"), attrsOf(image).value(QStringLiteral("relPath")).toString());
}

TEST(UT_MigrationHtmlConverter, RejectsUnsafeImageSrc)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p>safe<img src=\"javascript:/images/evil.png\"></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("unsafe-html-image-src")));

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(0).toObject()));
    ASSERT_EQ(1, nodeContentOf(blocks.at(0).toObject()).size());
    EXPECT_EQ(QStringLiteral("safe"), textOf(nodeContentOf(blocks.at(0).toObject()).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, WarnsAndSkipsBase64ImageData)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<img src=\"data:image/png;base64,AAAA\">"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("downgraded-base64-image")));

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(0).toObject()));
}


TEST(UT_MigrationHtmlConverter, SplitsMixedParagraphImageIntoBlocks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p>before<img src=\"images/a.png\">after</p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(3, blocks.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(0).toObject()));
    ASSERT_EQ(1, nodeContentOf(blocks.at(0).toObject()).size());
    EXPECT_EQ(QStringLiteral("before"), textOf(nodeContentOf(blocks.at(0).toObject()).at(0).toObject()));

    const QJsonObject image = blocks.at(1).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/a.png"), attrsOf(image).value(QStringLiteral("src")).toString());

    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(2).toObject()));
    ASSERT_EQ(1, nodeContentOf(blocks.at(2).toObject()).size());
    EXPECT_EQ(QStringLiteral("after"), textOf(nodeContentOf(blocks.at(2).toObject()).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, SplitsMixedListItemImageIntoBlocks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li>before<img src=\"images/a.png\">after</li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonObject list = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("bulletList"), nodeTypeOf(list));
    ASSERT_EQ(1, nodeContentOf(list).size());

    const QJsonObject listItem = nodeContentOf(list).at(0).toObject();
    EXPECT_EQ(QStringLiteral("listItem"), nodeTypeOf(listItem));
    const QJsonArray listItemContent = nodeContentOf(listItem);
    ASSERT_EQ(3, listItemContent.size());

    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(listItemContent.at(0).toObject()));
    ASSERT_EQ(1, nodeContentOf(listItemContent.at(0).toObject()).size());
    EXPECT_EQ(QStringLiteral("before"), textOf(nodeContentOf(listItemContent.at(0).toObject()).at(0).toObject()));

    const QJsonObject image = listItemContent.at(1).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/a.png"), attrsOf(image).value(QStringLiteral("src")).toString());

    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(listItemContent.at(2).toObject()));
    ASSERT_EQ(1, nodeContentOf(listItemContent.at(2).toObject()).size());
    EXPECT_EQ(QStringLiteral("after"), textOf(nodeContentOf(listItemContent.at(2).toObject()).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, KeepsListItemSchemaValidWhenImageIsFirstBlock)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li><img src=\"images/a.png\">after</li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray listItemContent = nodeContentOf(nodeContentOf(docContentOf(result).at(0).toObject()).at(0).toObject());
    ASSERT_EQ(3, listItemContent.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(listItemContent.at(0).toObject()));
    EXPECT_TRUE(nodeContentOf(listItemContent.at(0).toObject()).isEmpty());
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(listItemContent.at(1).toObject()));
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(listItemContent.at(2).toObject()));
    EXPECT_EQ(QStringLiteral("after"), textOf(nodeContentOf(listItemContent.at(2).toObject()).at(0).toObject()));
}


TEST(UT_MigrationHtmlConverter, KeepsListItemSchemaValidWhenHeadingContainsImage)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li><h2>before<img src=\"images/a.png\">after</h2></li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray listItemContent = nodeContentOf(nodeContentOf(docContentOf(result).at(0).toObject()).at(0).toObject());
    ASSERT_EQ(4, listItemContent.size());

    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(listItemContent.at(0).toObject()));
    EXPECT_TRUE(nodeContentOf(listItemContent.at(0).toObject()).isEmpty());

    const QJsonObject firstHeading = listItemContent.at(1).toObject();
    EXPECT_EQ(QStringLiteral("heading"), nodeTypeOf(firstHeading));
    EXPECT_EQ(2, attrsOf(firstHeading).value(QStringLiteral("level")).toInt());
    ASSERT_EQ(1, nodeContentOf(firstHeading).size());
    EXPECT_EQ(QStringLiteral("before"), textOf(nodeContentOf(firstHeading).at(0).toObject()));

    const QJsonObject image = listItemContent.at(2).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/a.png"), attrsOf(image).value(QStringLiteral("src")).toString());

    const QJsonObject secondHeading = listItemContent.at(3).toObject();
    EXPECT_EQ(QStringLiteral("heading"), nodeTypeOf(secondHeading));
    EXPECT_EQ(2, attrsOf(secondHeading).value(QStringLiteral("level")).toInt());
    ASSERT_EQ(1, nodeContentOf(secondHeading).size());
    EXPECT_EQ(QStringLiteral("after"), textOf(nodeContentOf(secondHeading).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, SplitsMixedHeadingImageIntoHeadingAndBlocks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<h2>before<img src=\"images/a.png\">after</h2>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(3, blocks.size());

    const QJsonObject firstHeading = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("heading"), nodeTypeOf(firstHeading));
    EXPECT_EQ(2, attrsOf(firstHeading).value(QStringLiteral("level")).toInt());
    ASSERT_EQ(1, nodeContentOf(firstHeading).size());
    EXPECT_EQ(QStringLiteral("before"), textOf(nodeContentOf(firstHeading).at(0).toObject()));

    const QJsonObject image = blocks.at(1).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/a.png"), attrsOf(image).value(QStringLiteral("src")).toString());

    const QJsonObject secondHeading = blocks.at(2).toObject();
    EXPECT_EQ(QStringLiteral("heading"), nodeTypeOf(secondHeading));
    EXPECT_EQ(2, attrsOf(secondHeading).value(QStringLiteral("level")).toInt());
    ASSERT_EQ(1, nodeContentOf(secondHeading).size());
    EXPECT_EQ(QStringLiteral("after"), textOf(nodeContentOf(secondHeading).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, SplitsMixedDowngradedBlockImageIntoParagraphAndBlocks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<section>before<img src=\"images/a.png\">after</section>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("downgraded-html-block")));

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(3, blocks.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(0).toObject()));
    ASSERT_EQ(1, nodeContentOf(blocks.at(0).toObject()).size());
    EXPECT_EQ(QStringLiteral("before"), textOf(nodeContentOf(blocks.at(0).toObject()).at(0).toObject()));

    const QJsonObject image = blocks.at(1).toObject();
    EXPECT_EQ(QStringLiteral("image"), nodeTypeOf(image));
    EXPECT_EQ(QStringLiteral("images/a.png"), attrsOf(image).value(QStringLiteral("src")).toString());

    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(blocks.at(2).toObject()));
    ASSERT_EQ(1, nodeContentOf(blocks.at(2).toObject()).size());
    EXPECT_EQ(QStringLiteral("after"), textOf(nodeContentOf(blocks.at(2).toObject()).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, ConvertsNestedTagMarksAndDeduplicates)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><strong>A <em>B</em></strong><b><strong>C</strong></b></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonArray content = nodeContentOf(blocks.at(0).toObject());
    ASSERT_EQ(3, content.size());

    const QJsonObject boldText = content.at(0).toObject();
    EXPECT_EQ(QStringLiteral("A"), textOf(boldText).trimmed());
    EXPECT_TRUE(hasMark(boldText, QStringLiteral("bold")));
    EXPECT_EQ(1, marksOf(boldText).size());

    const QJsonObject nestedText = content.at(1).toObject();
    EXPECT_EQ(QStringLiteral("B"), textOf(nestedText));
    EXPECT_TRUE(hasMark(nestedText, QStringLiteral("bold")));
    EXPECT_TRUE(hasMark(nestedText, QStringLiteral("italic")));
    EXPECT_EQ(2, marksOf(nestedText).size());

    const QJsonObject dedupedText = content.at(2).toObject();
    EXPECT_EQ(QStringLiteral("C"), textOf(dedupedText));
    EXPECT_TRUE(hasMark(dedupedText, QStringLiteral("bold")));
    EXPECT_EQ(1, marksOf(dedupedText).size());
}

TEST(UT_MigrationHtmlConverter, ConvertsInlineStyleMarksAndNormalizesColors)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><span style=\"color: rgb(10, 20, 30); background-color: #abc; "
                       "font-family: Noto Sans; font-size: 14PX\">Styled</span></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(result.warnings.isEmpty());

    const QJsonArray content = nodeContentOf(docContentOf(result).at(0).toObject());
    ASSERT_EQ(1, content.size());
    const QJsonObject text = content.at(0).toObject();
    EXPECT_EQ(QStringLiteral("Styled"), textOf(text));
    EXPECT_EQ(QStringLiteral("#0a141e"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
    EXPECT_EQ(QStringLiteral("#aabbcc"), markAttrsOf(text, QStringLiteral("highlight")).value(QStringLiteral("color")).toString());
    EXPECT_EQ(QStringLiteral("Noto Sans"), markAttrsOf(text, QStringLiteral("fontFamily")).value(QStringLiteral("fontFamily")).toString());
    EXPECT_EQ(QStringLiteral("14px"), markAttrsOf(text, QStringLiteral("fontSize")).value(QStringLiteral("fontSize")).toString());
}

TEST(UT_MigrationHtmlConverter, ConvertsStyleEquivalentTextMarks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><span style=\"font-weight: 700; font-style: italic; "
                       "text-decoration: underline line-through\">Decorated</span></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject text = nodeContentOf(docContentOf(result).at(0).toObject()).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Decorated"), textOf(text));
    EXPECT_TRUE(hasMark(text, QStringLiteral("bold")));
    EXPECT_TRUE(hasMark(text, QStringLiteral("italic")));
    EXPECT_TRUE(hasMark(text, QStringLiteral("underline")));
    EXPECT_TRUE(hasMark(text, QStringLiteral("strike")));
}

TEST(UT_MigrationHtmlConverter, NormalFontWeightClearsInheritedBoldMark)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><strong>Bold <span style=\"font-weight: normal\">Plain</span></strong></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray content = nodeContentOf(docContentOf(result).at(0).toObject());
    ASSERT_EQ(2, content.size());
    const QJsonObject boldText = content.at(0).toObject();
    EXPECT_EQ(QStringLiteral("Bold"), textOf(boldText).trimmed());
    EXPECT_TRUE(hasMark(boldText, QStringLiteral("bold")));

    const QJsonObject plainText = content.at(1).toObject();
    EXPECT_EQ(QStringLiteral("Plain"), textOf(plainText));
    EXPECT_FALSE(hasMark(plainText, QStringLiteral("bold")));
}

TEST(UT_MigrationHtmlConverter, NonBoldFontWeightClearsInheritedStyleBoldMark)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><span style=\"font-weight: bold\">Bold "
                       "<span style=\"font-weight: 500\">Plain</span></span></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray content = nodeContentOf(docContentOf(result).at(0).toObject());
    ASSERT_EQ(2, content.size());
    EXPECT_TRUE(hasMark(content.at(0).toObject(), QStringLiteral("bold")));
    EXPECT_EQ(QStringLiteral("Plain"), textOf(content.at(1).toObject()));
    EXPECT_FALSE(hasMark(content.at(1).toObject(), QStringLiteral("bold")));
}

TEST(UT_MigrationHtmlConverter, NormalFontStyleClearsInheritedItalicMark)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><em>Italic <span style=\"font-style: normal\">Plain</span></em></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray content = nodeContentOf(docContentOf(result).at(0).toObject());
    ASSERT_EQ(2, content.size());
    const QJsonObject italicText = content.at(0).toObject();
    EXPECT_EQ(QStringLiteral("Italic"), textOf(italicText).trimmed());
    EXPECT_TRUE(hasMark(italicText, QStringLiteral("italic")));

    const QJsonObject plainText = content.at(1).toObject();
    EXPECT_EQ(QStringLiteral("Plain"), textOf(plainText));
    EXPECT_FALSE(hasMark(plainText, QStringLiteral("italic")));
}

TEST(UT_MigrationHtmlConverter, TextDecorationNoneClearsInheritedDecorationMarks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><u>Under <span style=\"text-decoration: none\">Plain</span></u></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray content = nodeContentOf(docContentOf(result).at(0).toObject());
    ASSERT_EQ(2, content.size());
    EXPECT_TRUE(hasMark(content.at(0).toObject(), QStringLiteral("underline")));
    EXPECT_EQ(QStringLiteral("Plain"), textOf(content.at(1).toObject()));
    EXPECT_FALSE(hasMark(content.at(1).toObject(), QStringLiteral("underline")));
    EXPECT_FALSE(hasMark(content.at(1).toObject(), QStringLiteral("strike")));
}

TEST(UT_MigrationHtmlConverter, WarnsUnsupportedTextDecorationValues)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><span style=\"text-decoration: overline\">Text</span></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("invalid-html-style-value")));

    const QJsonObject text = nodeContentOf(docContentOf(result).at(0).toObject()).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Text"), textOf(text));
    EXPECT_TRUE(marksOf(text).isEmpty());
}

TEST(UT_MigrationHtmlConverter, AppliesSupportedTextDecorationAndWarnsUnsupportedTokens)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><span style=\"text-decoration-line: underline overline\">Text</span></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("invalid-html-style-value")));

    const QJsonObject text = nodeContentOf(docContentOf(result).at(0).toObject()).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Text"), textOf(text));
    EXPECT_TRUE(hasMark(text, QStringLiteral("underline")));
    EXPECT_FALSE(hasMark(text, QStringLiteral("strike")));
}

TEST(UT_MigrationHtmlConverter, WarnsUnsupportedAndInvalidStylesButKeepsText)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><span style=\"line-height: 2; color: not a color; "
                       "font-size: huge\">Text</span></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("unsupported-html-style")));
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("invalid-html-style-value")));

    const QJsonObject text = nodeContentOf(docContentOf(result).at(0).toObject()).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Text"), textOf(text));
    EXPECT_TRUE(marksOf(text).isEmpty());
}

TEST(UT_MigrationHtmlConverter, ConvertsFontTagColorAndFace)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p><font color=\"#ABCDEF\" face=\"Noto Serif\">Font</font></p>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject text = nodeContentOf(docContentOf(result).at(0).toObject()).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Font"), textOf(text));
    EXPECT_EQ(QStringLiteral("#abcdef"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
    EXPECT_EQ(QStringLiteral("Noto Serif"), markAttrsOf(text, QStringLiteral("fontFamily")).value(QStringLiteral("fontFamily")).toString());
}

TEST(UT_MigrationHtmlConverter, ConvertsNestedBulletAndOrderedLists)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li>One<ol><li>Two</li></ol></li>"
                       "<li><p><strong>Three</strong></p></li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(1, blocks.size());
    const QJsonObject bulletList = blocks.at(0).toObject();
    EXPECT_EQ(QStringLiteral("bulletList"), nodeTypeOf(bulletList));

    const QJsonArray bulletItems = nodeContentOf(bulletList);
    ASSERT_EQ(2, bulletItems.size());

    const QJsonArray firstItemContent = nodeContentOf(bulletItems.at(0).toObject());
    ASSERT_EQ(2, firstItemContent.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(firstItemContent.at(0).toObject()));
    EXPECT_EQ(QStringLiteral("One"), textOf(nodeContentOf(firstItemContent.at(0).toObject()).at(0).toObject()));
    EXPECT_EQ(QStringLiteral("orderedList"), nodeTypeOf(firstItemContent.at(1).toObject()));

    const QJsonArray orderedItems = nodeContentOf(firstItemContent.at(1).toObject());
    ASSERT_EQ(1, orderedItems.size());
    const QJsonArray nestedItemContent = nodeContentOf(orderedItems.at(0).toObject());
    ASSERT_EQ(1, nestedItemContent.size());
    EXPECT_EQ(QStringLiteral("Two"), textOf(nodeContentOf(nestedItemContent.at(0).toObject()).at(0).toObject()));

    const QJsonArray secondItemContent = nodeContentOf(bulletItems.at(1).toObject());
    ASSERT_EQ(1, secondItemContent.size());
    const QJsonObject strongText = nodeContentOf(secondItemContent.at(0).toObject()).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Three"), textOf(strongText));
    EXPECT_TRUE(hasMark(strongText, QStringLiteral("bold")));
}

TEST(UT_MigrationHtmlConverter, WrapsInvalidListChildrenIntoListItems)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul style=\"font-weight: bold\"><span>Loose</span><li>Item</li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("downgraded-html-list-child")));

    const QJsonArray items = nodeContentOf(docContentOf(result).at(0).toObject());
    ASSERT_EQ(2, items.size());
    const QJsonArray looseItemContent = nodeContentOf(items.at(0).toObject());
    ASSERT_EQ(1, looseItemContent.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodeTypeOf(looseItemContent.at(0).toObject()));
    const QJsonObject looseText = nodeContentOf(looseItemContent.at(0).toObject()).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Loose"), textOf(looseText));
    EXPECT_TRUE(hasMark(looseText, QStringLiteral("bold")));
}

TEST(UT_MigrationHtmlConverter, AppliesListItemInlineStyleMarksToDirectText)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li style=\"font-weight: bold; color: red\">Item</li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject list = docContentOf(result).at(0).toObject();
    const QJsonObject item = nodeContentOf(list).at(0).toObject();
    const QJsonObject paragraph = nodeContentOf(item).at(0).toObject();
    const QJsonObject text = nodeContentOf(paragraph).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Item"), textOf(text));
    EXPECT_TRUE(hasMark(text, QStringLiteral("bold")));
    EXPECT_EQ(QStringLiteral("red"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
}

TEST(UT_MigrationHtmlConverter, AppliesListItemInlineStyleMarksToBlockChildText)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li style=\"font-weight: bold; color: red\"><p>Item</p></li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject list = docContentOf(result).at(0).toObject();
    const QJsonObject item = nodeContentOf(list).at(0).toObject();
    const QJsonObject paragraph = nodeContentOf(item).at(0).toObject();
    const QJsonObject text = nodeContentOf(paragraph).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Item"), textOf(text));
    EXPECT_TRUE(hasMark(text, QStringLiteral("bold")));
    EXPECT_EQ(QStringLiteral("red"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
}

TEST(UT_MigrationHtmlConverter, BlockChildStyleOverridesListItemInheritedMarks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li style=\"font-weight: bold; color: red\">"
                       "<p style=\"font-weight: normal; color: blue\">Plain</p></li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject list = docContentOf(result).at(0).toObject();
    const QJsonObject item = nodeContentOf(list).at(0).toObject();
    const QJsonObject paragraph = nodeContentOf(item).at(0).toObject();
    const QJsonObject text = nodeContentOf(paragraph).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Plain"), textOf(text));
    EXPECT_FALSE(hasMark(text, QStringLiteral("bold")));
    EXPECT_EQ(QStringLiteral("blue"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
}

TEST(UT_MigrationHtmlConverter, AppliesListInlineStyleMarksToDirectListItemText)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul style=\"font-weight: bold; color: red\"><li>Item</li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject list = docContentOf(result).at(0).toObject();
    const QJsonObject item = nodeContentOf(list).at(0).toObject();
    const QJsonObject paragraph = nodeContentOf(item).at(0).toObject();
    const QJsonObject text = nodeContentOf(paragraph).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Item"), textOf(text));
    EXPECT_TRUE(hasMark(text, QStringLiteral("bold")));
    EXPECT_EQ(QStringLiteral("red"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
}

TEST(UT_MigrationHtmlConverter, AppliesListInlineStyleMarksToBlockChildText)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ol style=\"font-weight: bold; color: red\"><li><p>Item</p></li></ol>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject list = docContentOf(result).at(0).toObject();
    EXPECT_EQ(QStringLiteral("orderedList"), nodeTypeOf(list));
    const QJsonObject item = nodeContentOf(list).at(0).toObject();
    const QJsonObject paragraph = nodeContentOf(item).at(0).toObject();
    const QJsonObject text = nodeContentOf(paragraph).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Item"), textOf(text));
    EXPECT_TRUE(hasMark(text, QStringLiteral("bold")));
    EXPECT_EQ(QStringLiteral("red"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
}

TEST(UT_MigrationHtmlConverter, BlockChildStyleOverridesListInheritedMarks)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul style=\"font-weight: bold; color: red\"><li>"
                       "<p style=\"font-weight: normal; color: blue\">Plain</p></li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);

    const QJsonObject list = docContentOf(result).at(0).toObject();
    const QJsonObject item = nodeContentOf(list).at(0).toObject();
    const QJsonObject paragraph = nodeContentOf(item).at(0).toObject();
    const QJsonObject text = nodeContentOf(paragraph).at(0).toObject();
    EXPECT_EQ(QStringLiteral("Plain"), textOf(text));
    EXPECT_FALSE(hasMark(text, QStringLiteral("bold")));
    EXPECT_EQ(QStringLiteral("blue"), markAttrsOf(text, QStringLiteral("color")).value(QStringLiteral("color")).toString());
}

TEST(UT_MigrationHtmlConverter, WarnsTextAlignOnListItem)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<ul><li style=\"text-align: center\">Item</li></ul>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    ASSERT_EQ(1, result.warnings.size());
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("downgraded-text-align")));

    const QJsonObject list = docContentOf(result).at(0).toObject();
    const QJsonObject item = nodeContentOf(list).at(0).toObject();
    const QJsonObject paragraph = nodeContentOf(item).at(0).toObject();
    EXPECT_FALSE(attrsOf(paragraph).contains(QStringLiteral("textAlign")));
    EXPECT_EQ(QStringLiteral("Item"), textOf(nodeContentOf(paragraph).at(0).toObject()));
}

TEST(UT_MigrationHtmlConverter, DowngradesTextAlignBecauseSchemaV1HasNoAlignmentAttrs)
{
    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(
        QStringLiteral("<p style=\"text-align: center\">Centered</p>"
                       "<h2 style=\"text-align: left\">Title</h2>"));

    EXPECT_TRUE(result.ok());
    expectEnvelopeValid(result);
    ASSERT_EQ(1, result.warnings.size());
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("downgraded-text-align")));

    const QJsonArray blocks = docContentOf(result);
    ASSERT_EQ(2, blocks.size());
    EXPECT_FALSE(attrsOf(blocks.at(0).toObject()).contains(QStringLiteral("textAlign")));
    EXPECT_EQ(QStringLiteral("Centered"), textOf(nodeContentOf(blocks.at(0).toObject()).at(0).toObject()));
    EXPECT_FALSE(attrsOf(blocks.at(1).toObject()).contains(QStringLiteral("textAlign")));
    EXPECT_EQ(QStringLiteral("Title"), textOf(nodeContentOf(blocks.at(1).toObject()).at(0).toObject()));
}
