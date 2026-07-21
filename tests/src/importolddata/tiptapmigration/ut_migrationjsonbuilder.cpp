// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/tiptapmigration/migrationjsonbuilder.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QTemporaryFile>

namespace {
QJsonArray arrayOf(const QJsonObject &object)
{
    return QJsonArray { object };
}

QJsonObject attrsOf(const QJsonObject &node)
{
    return node.value(QStringLiteral("attrs")).toObject();
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

    return QDir::currentPath();
}

QString writeTempEnvelope(const QJsonObject &envelope)
{
    QTemporaryFile file(QDir::tempPath() + QStringLiteral("/migration-envelope-XXXXXX.json"));
    file.setAutoRemove(false);
    EXPECT_TRUE(file.open());
    file.write(MigrationJsonBuilder::toCompactJson(envelope).toUtf8());
    const QString path = file.fileName();
    file.close();
    return path;
}
} // namespace

TEST(UT_MigrationJsonBuilder, CreatesEnvelopeWithEmptyDoc)
{
    const QJsonObject envelope = MigrationJsonBuilder::makeEnvelope();
    const QJsonObject content = envelope.value(QStringLiteral("content")).toObject();

    EXPECT_EQ(QStringLiteral("tiptap"), envelope.value(QStringLiteral("format")).toString());
    EXPECT_EQ(1, envelope.value(QStringLiteral("schemaVersion")).toInt());
    expectNodeType(content, QStringLiteral("doc"));
    ASSERT_EQ(1, content.value(QStringLiteral("content")).toArray().size());
    expectNodeType(content.value(QStringLiteral("content")).toArray().at(0).toObject(), QStringLiteral("paragraph"));
}

TEST(UT_MigrationJsonBuilder, CreatesTextWithMarks)
{
    const QJsonArray marks {
        MigrationJsonBuilder::makeMark(QStringLiteral("bold")),
        MigrationJsonBuilder::makeMark(QStringLiteral("color"), QJsonObject { { QStringLiteral("color"), QStringLiteral("#ff0000") } })
    };
    const QJsonObject text = MigrationJsonBuilder::makeText(QStringLiteral("重点内容"), marks);
    const QJsonObject paragraph = MigrationJsonBuilder::makeParagraph(arrayOf(text));

    expectNodeType(paragraph, QStringLiteral("paragraph"));
    const QJsonObject textNode = paragraph.value(QStringLiteral("content")).toArray().at(0).toObject();
    expectNodeType(textNode, QStringLiteral("text"));
    EXPECT_EQ(QStringLiteral("重点内容"), textNode.value(QStringLiteral("text")).toString());
    EXPECT_EQ(2, textNode.value(QStringLiteral("marks")).toArray().size());
}

TEST(UT_MigrationJsonBuilder, CreatesHeadingAndHardBreak)
{
    const QJsonObject heading = MigrationJsonBuilder::makeHeading(
        2, QJsonArray { MigrationJsonBuilder::makeText(QStringLiteral("标题")) });
    const QJsonObject hardBreak = MigrationJsonBuilder::makeHardBreak();

    expectNodeType(heading, QStringLiteral("heading"));
    EXPECT_EQ(2, attrsOf(heading).value(QStringLiteral("level")).toInt());
    expectNodeType(hardBreak, QStringLiteral("hardBreak"));
}

TEST(UT_MigrationJsonBuilder, CreatesImageAndVoiceBlock)
{
    const QJsonObject image = MigrationJsonBuilder::makeImage(
        QStringLiteral("images/photo.png"),
        QStringLiteral("images/photo.png"),
        QString(),
        QString());
    const QJsonObject voiceBlock = MigrationJsonBuilder::makeVoiceBlock(
        QStringLiteral("voice-uuid"),
        QStringLiteral("voicenote/20260717-100000.mp3"),
        120000,
        QStringLiteral("2026-07-17 10:00:00"),
        QStringLiteral("录音"),
        QStringLiteral("转写文本"),
        true);

    expectNodeType(image, QStringLiteral("image"));
    EXPECT_EQ(QStringLiteral("images/photo.png"), attrsOf(image).value(QStringLiteral("src")).toString());
    EXPECT_EQ(QStringLiteral("images/photo.png"), attrsOf(image).value(QStringLiteral("relPath")).toString());
    EXPECT_EQ(QString(), attrsOf(image).value(QStringLiteral("alt")).toString());
    EXPECT_TRUE(attrsOf(image).value(QStringLiteral("title")).isNull());

    expectNodeType(voiceBlock, QStringLiteral("voiceBlock"));
    EXPECT_EQ(QStringLiteral("voice-uuid"), attrsOf(voiceBlock).value(QStringLiteral("voiceId")).toString());
    EXPECT_EQ(QStringLiteral("voicenote/20260717-100000.mp3"), attrsOf(voiceBlock).value(QStringLiteral("voicePath")).toString());
    EXPECT_EQ(120000, attrsOf(voiceBlock).value(QStringLiteral("voiceSize")).toInt());
    EXPECT_TRUE(attrsOf(voiceBlock).value(QStringLiteral("translateUnfold")).toBool());
    EXPECT_FALSE(attrsOf(voiceBlock).contains(QStringLiteral("translating")));
}

TEST(UT_MigrationJsonBuilder, CreatesListsAndTaskItems)
{
    const QJsonObject listItem = MigrationJsonBuilder::makeListItem(
        arrayOf(MigrationJsonBuilder::makeParagraph(arrayOf(MigrationJsonBuilder::makeText(QStringLiteral("列表"))))));
    const QJsonObject bulletList = MigrationJsonBuilder::makeBulletList(arrayOf(listItem));
    const QJsonObject orderedList = MigrationJsonBuilder::makeOrderedList(arrayOf(listItem));
    const QJsonObject taskItem = MigrationJsonBuilder::makeTaskItem(
        false,
        arrayOf(MigrationJsonBuilder::makeParagraph(arrayOf(MigrationJsonBuilder::makeText(QStringLiteral("待办"))))));
    const QJsonObject taskList = MigrationJsonBuilder::makeTaskList(arrayOf(taskItem));

    expectNodeType(bulletList, QStringLiteral("bulletList"));
    expectNodeType(orderedList, QStringLiteral("orderedList"));
    expectNodeType(taskList, QStringLiteral("taskList"));
    EXPECT_FALSE(attrsOf(taskItem).value(QStringLiteral("checked")).toBool());
}

TEST(UT_MigrationJsonBuilder, NormalizesEmptyListItems)
{
    const QJsonObject listItem = MigrationJsonBuilder::makeListItem();
    const QJsonObject taskItem = MigrationJsonBuilder::makeTaskItem(true);

    ASSERT_EQ(1, listItem.value(QStringLiteral("content")).toArray().size());
    expectNodeType(listItem.value(QStringLiteral("content")).toArray().at(0).toObject(), QStringLiteral("paragraph"));
    ASSERT_EQ(1, taskItem.value(QStringLiteral("content")).toArray().size());
    expectNodeType(taskItem.value(QStringLiteral("content")).toArray().at(0).toObject(), QStringLiteral("paragraph"));
}

TEST(UT_MigrationJsonBuilder, OutputsCompactJson)
{
    const QString json = MigrationJsonBuilder::toCompactJson(MigrationJsonBuilder::makeEnvelope());
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);

    EXPECT_EQ(QJsonParseError::NoError, error.error);
    EXPECT_TRUE(document.isObject());
    EXPECT_FALSE(json.contains(QLatin1Char('\n')));
}

TEST(UT_MigrationJsonBuilder, GeneratedEnvelopePassesSchemaValidator)
{
    const QJsonObject paragraph = MigrationJsonBuilder::makeParagraph(
        QJsonArray { MigrationJsonBuilder::makeText(QStringLiteral("重点内容")) });
    const QJsonObject image = MigrationJsonBuilder::makeImage(
        QStringLiteral("images/photo.png"),
        QStringLiteral("images/photo.png"),
        QString(),
        QString());
    const QJsonObject voiceBlock = MigrationJsonBuilder::makeVoiceBlock(
        QStringLiteral("voice-uuid"),
        QStringLiteral("voicenote/20260717-100000.mp3"),
        120000,
        QStringLiteral("2026-07-17 10:00:00"),
        QStringLiteral("录音"),
        QStringLiteral("转写文本"),
        true);
    const QJsonObject taskItem = MigrationJsonBuilder::makeTaskItem(
        false,
        arrayOf(MigrationJsonBuilder::makeParagraph(arrayOf(MigrationJsonBuilder::makeText(QStringLiteral("待办"))))));
    const QJsonObject envelope = MigrationJsonBuilder::makeEnvelope(
        MigrationJsonBuilder::makeDoc(QJsonArray { paragraph, image, voiceBlock, MigrationJsonBuilder::makeTaskList(arrayOf(taskItem)) }));

    const QString path = writeTempEnvelope(envelope);
    QProcess validator;
    validator.setWorkingDirectory(findRepoRoot());
    validator.start(QStringLiteral("node"), { QStringLiteral("web-editor/scripts/validate-envelope.mjs"), path });
    ASSERT_TRUE(validator.waitForFinished(30000));
    EXPECT_EQ(0, validator.exitCode()) << validator.readAllStandardError().toStdString();
    QFile::remove(path);
}
