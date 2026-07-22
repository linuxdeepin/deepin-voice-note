// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/tiptapmigration/migrationjsonbuilder.h"
#include "importolddata/tiptapmigration/migrationjsonvalidator.h"
#include "importolddata/tiptapmigration/migrationnotedataconverter.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryFile>

namespace {

bool hasWarningCode(const MigrationNoteDataConversionResult &result, const QString &code)
{
    for (const MigrationNoteDataConversionIssue &warning : result.warnings) {
        if (warning.code == code) {
            return true;
        }
    }
    return false;
}

bool hasErrorCode(const MigrationNoteDataConversionResult &result, const QString &code)
{
    for (const MigrationNoteDataConversionIssue &error : result.errors) {
        if (error.code == code) {
            return true;
        }
    }
    return false;
}

QJsonArray docContent(const MigrationNoteDataConversionResult &result)
{
    return result.envelope.value(QStringLiteral("content"))
        .toObject()
        .value(QStringLiteral("content"))
        .toArray();
}

QJsonArray paragraphContent(const QJsonObject &paragraph)
{
    return paragraph.value(QStringLiteral("content")).toArray();
}

void expectNodeType(const QJsonObject &node, const QString &type)
{
    EXPECT_EQ(type, node.value(QStringLiteral("type")).toString());
}

QString findRepoRoot()
{
    QDir dir(QDir::currentPath());
    for (int depth = 0; depth < 8; ++depth) {
        if (QFile::exists(dir.filePath(QStringLiteral("web-editor/scripts/validate-envelope.mjs")))) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }

    return QString();
}

} // namespace

TEST(UT_MigrationNoteDataConverter, ConvertsUnicodeAndSpecialCharacters)
{
    const QString metadata = QStringLiteral(
        R"({"dataCount":1,"noteDatas":[{"text":"Hello 中文🙂 <>&","type":1}],"voiceMaxId":0})");

    const MigrationNoteDataConversionResult result = MigrationNoteDataConverter::convertTextBlocks(metadata);

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.warnings.isEmpty());
    const QJsonArray content = docContent(result);
    ASSERT_EQ(1, content.size());
    expectNodeType(content.at(0).toObject(), QStringLiteral("paragraph"));
    const QJsonArray paragraph = paragraphContent(content.at(0).toObject());
    ASSERT_EQ(1, paragraph.size());
    expectNodeType(paragraph.at(0).toObject(), QStringLiteral("text"));
    EXPECT_EQ(QStringLiteral("Hello 中文🙂 <>&"), paragraph.at(0).toObject().value(QStringLiteral("text")).toString());
}

TEST(UT_MigrationNoteDataConverter, ConvertsNewlinesToHardBreaksInsideParagraph)
{
    QJsonObject textBlock;
    textBlock.insert(QStringLiteral("type"), 1);
    textBlock.insert(QStringLiteral("text"), QStringLiteral("A\nB\r\nC\rD"));
    const QJsonObject metadata { { QStringLiteral("noteDatas"), QJsonArray { textBlock } } };

    const MigrationNoteDataConversionResult result = MigrationNoteDataConverter::convertTextBlocks(metadata);

    EXPECT_TRUE(result.ok());
    const QJsonArray content = docContent(result);
    ASSERT_EQ(1, content.size());
    const QJsonArray paragraph = paragraphContent(content.at(0).toObject());
    ASSERT_EQ(7, paragraph.size());
    EXPECT_EQ(QStringLiteral("A"), paragraph.at(0).toObject().value(QStringLiteral("text")).toString());
    expectNodeType(paragraph.at(1).toObject(), QStringLiteral("hardBreak"));
    EXPECT_EQ(QStringLiteral("B"), paragraph.at(2).toObject().value(QStringLiteral("text")).toString());
    expectNodeType(paragraph.at(3).toObject(), QStringLiteral("hardBreak"));
    EXPECT_EQ(QStringLiteral("C"), paragraph.at(4).toObject().value(QStringLiteral("text")).toString());
    expectNodeType(paragraph.at(5).toObject(), QStringLiteral("hardBreak"));
    EXPECT_EQ(QStringLiteral("D"), paragraph.at(6).toObject().value(QStringLiteral("text")).toString());
}

TEST(UT_MigrationNoteDataConverter, TrimsTrailingNewlineWithoutDroppingInnerBlankLines)
{
    QJsonObject textBlock;
    textBlock.insert(QStringLiteral("type"), 1);
    textBlock.insert(QStringLiteral("text"), QStringLiteral("A\n\nB\n"));
    const QJsonObject metadata { { QStringLiteral("noteDatas"), QJsonArray { textBlock } } };

    const MigrationNoteDataConversionResult result = MigrationNoteDataConverter::convertTextBlocks(metadata);

    EXPECT_TRUE(result.ok());
    const QJsonArray content = docContent(result);
    ASSERT_EQ(1, content.size());
    const QJsonArray paragraph = paragraphContent(content.at(0).toObject());
    ASSERT_EQ(4, paragraph.size());
    EXPECT_EQ(QStringLiteral("A"), paragraph.at(0).toObject().value(QStringLiteral("text")).toString());
    expectNodeType(paragraph.at(1).toObject(), QStringLiteral("hardBreak"));
    expectNodeType(paragraph.at(2).toObject(), QStringLiteral("hardBreak"));
    EXPECT_EQ(QStringLiteral("B"), paragraph.at(3).toObject().value(QStringLiteral("text")).toString());
}

TEST(UT_MigrationNoteDataConverter, HandlesEmptyTextAsEmptyParagraph)
{
    const QString metadata = QStringLiteral(R"({"noteDatas":[{"text":"","type":1}]})");

    const MigrationNoteDataConversionResult result = MigrationNoteDataConverter::convertTextBlocks(metadata);

    EXPECT_TRUE(result.ok());
    const QJsonArray content = docContent(result);
    ASSERT_EQ(1, content.size());
    expectNodeType(content.at(0).toObject(), QStringLiteral("paragraph"));
    EXPECT_TRUE(content.at(0).toObject().value(QStringLiteral("content")).isUndefined());
}

TEST(UT_MigrationNoteDataConverter, HandlesEmptyNoteDatasAsEmptyParagraph)
{
    const QString metadata = QStringLiteral(R"({"noteDatas":[]})");

    const MigrationNoteDataConversionResult result = MigrationNoteDataConverter::convertTextBlocks(metadata);

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.warnings.isEmpty());
    const QJsonArray content = docContent(result);
    ASSERT_EQ(1, content.size());
    expectNodeType(content.at(0).toObject(), QStringLiteral("paragraph"));
    EXPECT_TRUE(content.at(0).toObject().value(QStringLiteral("content")).isUndefined());
}

TEST(UT_MigrationNoteDataConverter, SkipsNonTextBlocksWithWarning)
{
    const QString metadata = QStringLiteral(
        R"({"noteDatas":[{"text":"first","type":1},{"text":"","title":"voice","type":2,"voicePath":"voicenote/a.mp3"},{"text":"second","type":1}]})");

    const MigrationNoteDataConversionResult result = MigrationNoteDataConverter::convertTextBlocks(metadata);

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(hasWarningCode(result, QStringLiteral("skipped-non-text-block")));
    const QJsonArray content = docContent(result);
    ASSERT_EQ(2, content.size());
    EXPECT_EQ(QStringLiteral("first"), paragraphContent(content.at(0).toObject()).at(0).toObject().value(QStringLiteral("text")).toString());
    EXPECT_EQ(QStringLiteral("second"), paragraphContent(content.at(1).toObject()).at(0).toObject().value(QStringLiteral("text")).toString());
}

TEST(UT_MigrationNoteDataConverter, RejectsInvalidInputShape)
{
    const MigrationNoteDataConversionResult invalidJson = MigrationNoteDataConverter::convertTextBlocks(QStringLiteral("{"));
    EXPECT_FALSE(invalidJson.ok());
    EXPECT_TRUE(hasErrorCode(invalidJson, QStringLiteral("invalid-json")));

    const MigrationNoteDataConversionResult missingNoteDatas = MigrationNoteDataConverter::convertTextBlocks(QJsonObject());
    EXPECT_FALSE(missingNoteDatas.ok());
    EXPECT_TRUE(hasErrorCode(missingNoteDatas, QStringLiteral("invalid-note-datas")));
}

TEST(UT_MigrationNoteDataConverter, ComparesGoldenJsonAndPassesValidators)
{
    const QString metadata = QStringLiteral(R"({"noteDatas":[{"text":"第一行\n第二行","type":1},{"text":"","type":1}]})");
    const QString golden = QStringLiteral(
        R"({"content":{"content":[{"content":[{"text":"第一行","type":"text"},{"type":"hardBreak"},{"text":"第二行","type":"text"}],"type":"paragraph"},{"type":"paragraph"}],"type":"doc"},"format":"tiptap","schemaVersion":1})");

    const MigrationNoteDataConversionResult result = MigrationNoteDataConverter::convertTextBlocks(metadata);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(golden, MigrationJsonBuilder::toCompactJson(result.envelope));
    const MigrationJsonValidationResult validation = MigrationJsonValidator::validateEnvelope(result.envelope);
    EXPECT_TRUE(validation.ok()) << validation.errors.value(0).code.toStdString();

    const QString repoRoot = findRepoRoot();
    if (repoRoot.isEmpty()) {
        GTEST_SKIP() << "web-editor/scripts/validate-envelope.mjs not found";
    }

    QTemporaryFile envelopeFile(QDir::tempPath() + QStringLiteral("/note-data-text-envelope-XXXXXX.json"));
    ASSERT_TRUE(envelopeFile.open());
    ASSERT_NE(-1, envelopeFile.write(MigrationJsonBuilder::toCompactJson(result.envelope).toUtf8()));
    ASSERT_TRUE(envelopeFile.flush());

    QProcess validator;
    validator.setWorkingDirectory(repoRoot);
    validator.start(QStringLiteral("node"), { QStringLiteral("web-editor/scripts/validate-envelope.mjs"), envelopeFile.fileName() });
    ASSERT_TRUE(validator.waitForFinished(30000));
    EXPECT_EQ(0, validator.exitCode()) << validator.readAllStandardError().toStdString();
}
