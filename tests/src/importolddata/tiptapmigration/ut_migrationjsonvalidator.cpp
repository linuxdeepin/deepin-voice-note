// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/tiptapmigration/migrationjsonbuilder.h"
#include "importolddata/tiptapmigration/migrationjsonvalidator.h"

#include "gtest/gtest.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace {

QJsonArray arrayOf(const QJsonObject &object)
{
    return QJsonArray { object };
}

bool hasErrorCode(const MigrationJsonValidationResult &result, const QString &code)
{
    for (const MigrationJsonValidationError &error : result.errors) {
        if (error.code == code) {
            return true;
        }
    }
    return false;
}

bool hasError(const MigrationJsonValidationResult &result, const QString &path, const QString &code)
{
    for (const MigrationJsonValidationError &error : result.errors) {
        if (error.path == path && error.code == code) {
            return true;
        }
    }
    return false;
}

QJsonObject envelopeWithSingleNode(const QJsonObject &node)
{
    return MigrationJsonBuilder::makeEnvelope(MigrationJsonBuilder::makeDoc(arrayOf(node)));
}

QJsonObject imageNode(const QString &src)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("src"), src);

    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("image"));
    node.insert(QStringLiteral("attrs"), attrs);
    return node;
}

} // namespace

TEST(UT_MigrationJsonValidator, AcceptsBuilderGeneratedEnvelope)
{
    const QJsonObject paragraph = MigrationJsonBuilder::makeParagraph(
        arrayOf(MigrationJsonBuilder::makeText(QStringLiteral("重点内容"),
                                               QJsonArray { MigrationJsonBuilder::makeMark(QStringLiteral("bold")) })));
    const QJsonObject image = MigrationJsonBuilder::makeImage(QStringLiteral("images/photo.png"),
                                                              QStringLiteral("images/photo.png"));
    const QJsonObject voiceBlock = MigrationJsonBuilder::makeVoiceBlock(
        QStringLiteral("voice-uuid"),
        QStringLiteral("voicenote/20260717-100000.mp3"),
        120000,
        QStringLiteral("2026-07-17 10:00:00"),
        QStringLiteral("录音"),
        QStringLiteral("转写文本"));
    const QJsonObject taskItem = MigrationJsonBuilder::makeTaskItem(
        false,
        arrayOf(MigrationJsonBuilder::makeParagraph(arrayOf(MigrationJsonBuilder::makeText(QStringLiteral("待办"))))));
    const QJsonObject envelope = MigrationJsonBuilder::makeEnvelope(
        MigrationJsonBuilder::makeDoc(QJsonArray { paragraph, image, voiceBlock, MigrationJsonBuilder::makeTaskList(arrayOf(taskItem)) }));

    const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(envelope);

    EXPECT_TRUE(result.ok()) << result.errors.value(0).code.toStdString();
}

TEST(UT_MigrationJsonValidator, RejectsInvalidEnvelopeFields)
{
    QJsonObject envelope = MigrationJsonBuilder::makeEnvelope();
    envelope.insert(QStringLiteral("format"), QStringLiteral("tiptap-json"));
    envelope.insert(QStringLiteral("schemaVersion"), 2);
    envelope.insert(QStringLiteral("content"), QStringLiteral("not-doc"));

    const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(envelope);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(hasError(result, QStringLiteral("format"), QStringLiteral("invalid-format")));
    EXPECT_TRUE(hasError(result, QStringLiteral("schemaVersion"), QStringLiteral("unsupported-schema-version")));
    EXPECT_TRUE(hasError(result, QStringLiteral("content"), QStringLiteral("invalid-content")));
}

TEST(UT_MigrationJsonValidator, RejectsInvalidRootAndEmptyDocContent)
{
    const MigrationJsonValidationResult invalidRoot = MigrationJsonValidator::validateEnvelope(
        MigrationJsonBuilder::makeEnvelope(MigrationJsonBuilder::makeParagraph()));

    QJsonObject emptyDoc;
    emptyDoc.insert(QStringLiteral("type"), QStringLiteral("doc"));
    emptyDoc.insert(QStringLiteral("content"), QJsonArray());
    const MigrationJsonValidationResult emptyDocResult = MigrationJsonValidator::validateEnvelope(
        MigrationJsonBuilder::makeEnvelope(emptyDoc));

    EXPECT_FALSE(invalidRoot.ok());
    EXPECT_TRUE(hasError(invalidRoot, QStringLiteral("content.type"), QStringLiteral("invalid-root")));
    EXPECT_FALSE(emptyDocResult.ok());
    EXPECT_TRUE(hasError(emptyDocResult, QStringLiteral("content.content"), QStringLiteral("empty-doc-content")));
}

TEST(UT_MigrationJsonValidator, RejectsUnsupportedNodesAndMarks)
{
    QJsonObject unknownMark;
    unknownMark.insert(QStringLiteral("type"), QStringLiteral("blink"));

    QJsonObject text = MigrationJsonBuilder::makeText(QStringLiteral("文本"), QJsonArray { unknownMark });
    QJsonObject unknownNode;
    unknownNode.insert(QStringLiteral("type"), QStringLiteral("unknownNode"));

    const QJsonObject envelope = MigrationJsonBuilder::makeEnvelope(
        MigrationJsonBuilder::makeDoc(QJsonArray { text, unknownNode }));
    const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(envelope);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[0].marks[0].type"), QStringLiteral("unsupported-mark")));
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[1].type"), QStringLiteral("unsupported-node")));
}

TEST(UT_MigrationJsonValidator, RejectsInvalidVoiceBlockPersistence)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("voiceId"), QStringLiteral("  "));
    attrs.insert(QStringLiteral("voicePath"), QStringLiteral(""));
    attrs.insert(QStringLiteral("voiceSize"), -1);
    attrs.insert(QStringLiteral("translating"), false);

    QJsonObject voiceBlock;
    voiceBlock.insert(QStringLiteral("type"), QStringLiteral("voiceBlock"));
    voiceBlock.insert(QStringLiteral("attrs"), attrs);

    const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(envelopeWithSingleNode(voiceBlock));

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[0].attrs.voiceId"), QStringLiteral("invalid-voice-id")));
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[0].attrs.voicePath"), QStringLiteral("invalid-voice-path")));
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[0].attrs.voiceSize"), QStringLiteral("invalid-voice-size")));
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[0].attrs.translating"), QStringLiteral("runtime-state-persisted")));
}

TEST(UT_MigrationJsonValidator, RejectsUnsafeImageSrcs)
{
    const QStringList unsafeSrcs = {
        QStringLiteral(""),
        QStringLiteral("javascript:alert(1)"),
        QStringLiteral("java\tscript:alert(1)"),
        QStringLiteral("data \n:text/html,<script>alert(1)</script>"),
        QStringLiteral("vbscript\r:msgbox(1)"),
        QStringLiteral("file:///etc/passwd"),
        QStringLiteral("mailto:test@example.com"),
        QStringLiteral("//example.com/protocol-relative.png"),
        QStringLiteral("\\\\example.com\\share.png"),
    };

    for (const QString &src : unsafeSrcs) {
        const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(envelopeWithSingleNode(imageNode(src)));
        EXPECT_FALSE(result.ok()) << src.toStdString();
        EXPECT_TRUE(hasErrorCode(result, src.isEmpty() ? QStringLiteral("invalid-image-src") : QStringLiteral("unsafe-image-src")))
            << src.toStdString();
    }
}

TEST(UT_MigrationJsonValidator, AcceptsSafeImageSrcs)
{
    const QStringList safeSrcs = {
        QStringLiteral("https://example.com/photo.png"),
        QStringLiteral("http://example.com/photo.png"),
        QStringLiteral(" HTTP ://example.com/photo.png"),
        QStringLiteral("images/photo.png"),
        QStringLiteral("./images/photo.png"),
    };

    for (const QString &src : safeSrcs) {
        const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(envelopeWithSingleNode(imageNode(src)));
        EXPECT_TRUE(result.ok()) << src.toStdString();
    }
}

TEST(UT_MigrationJsonValidator, RejectsInvalidNodeShapes)
{
    QJsonObject text;
    text.insert(QStringLiteral("type"), QStringLiteral("text"));

    QJsonObject heading;
    heading.insert(QStringLiteral("type"), QStringLiteral("heading"));
    heading.insert(QStringLiteral("attrs"), QJsonObject { { QStringLiteral("level"), 7 } });

    QJsonObject paragraph;
    paragraph.insert(QStringLiteral("type"), QStringLiteral("paragraph"));
    paragraph.insert(QStringLiteral("content"), QStringLiteral("not-array"));

    const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(
        MigrationJsonBuilder::makeEnvelope(MigrationJsonBuilder::makeDoc(QJsonArray { text, heading, paragraph })));

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[0].text"), QStringLiteral("invalid-text")));
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[1].attrs.level"), QStringLiteral("invalid-heading-level")));
    EXPECT_TRUE(hasError(result, QStringLiteral("content.content[2].content"), QStringLiteral("invalid-content-array")));
}

TEST(UT_MigrationJsonValidator, RejectsDocumentsBeyondMaximumNodeDepth)
{
    QJsonObject node = MigrationJsonBuilder::makeParagraph();
    for (int index = 0; index < 120; ++index) {
        QJsonObject wrapper;
        wrapper.insert(QStringLiteral("type"), QStringLiteral("blockquote"));
        wrapper.insert(QStringLiteral("content"), QJsonArray { node });
        node = wrapper;
    }

    const MigrationJsonValidationResult result = MigrationJsonValidator::validateEnvelope(envelopeWithSingleNode(node));

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(hasErrorCode(result, QStringLiteral("max-node-depth-exceeded")));
}
