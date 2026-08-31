// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/tiptapdocumentexporter.h"
#include "common/vnoteitem.h"
#include "task/exportnoteworker.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace {

QJsonObject textNode(const QString &text, const QJsonArray &marks = QJsonArray())
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("text"));
    node.insert(QStringLiteral("text"), text);
    if (!marks.isEmpty()) {
        node.insert(QStringLiteral("marks"), marks);
    }
    return node;
}

QJsonObject mark(const QString &type, const QJsonObject &attrs = QJsonObject())
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), type);
    if (!attrs.isEmpty()) {
        node.insert(QStringLiteral("attrs"), attrs);
    }
    return node;
}

QJsonObject paragraph(const QJsonArray &content)
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("paragraph"));
    node.insert(QStringLiteral("content"), content);
    return node;
}

QJsonObject heading(int level, const QString &text)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("level"), level);
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("heading"));
    node.insert(QStringLiteral("attrs"), attrs);
    node.insert(QStringLiteral("content"), QJsonArray { textNode(text) });
    return node;
}

QJsonObject blockNode(const QString &type, const QJsonArray &content = QJsonArray(), const QJsonObject &attrs = QJsonObject())
{
    QJsonObject node;
    node.insert(QStringLiteral("type"), type);
    if (!attrs.isEmpty()) {
        node.insert(QStringLiteral("attrs"), attrs);
    }
    if (!content.isEmpty()) {
        node.insert(QStringLiteral("content"), content);
    }
    return node;
}

QJsonObject listItem(const QJsonArray &content)
{
    return blockNode(QStringLiteral("listItem"), content);
}

QJsonObject taskItem(bool checked, const QString &text)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("checked"), checked);
    return blockNode(QStringLiteral("taskItem"), QJsonArray { paragraph(QJsonArray { textNode(text) }) }, attrs);
}

QJsonObject nestedBulletList()
{
    return blockNode(QStringLiteral("bulletList"), QJsonArray {
        listItem(QJsonArray {
            paragraph(QJsonArray { textNode(QStringLiteral("父项")) }),
            blockNode(QStringLiteral("bulletList"), QJsonArray {
                listItem(QJsonArray { paragraph(QJsonArray { textNode(QStringLiteral("子项")) }) }),
            }),
        }),
    });
}

QJsonObject taskList()
{
    return blockNode(QStringLiteral("taskList"), QJsonArray {
        taskItem(true, QStringLiteral("任务完成")),
    });
}

QJsonObject imageNode(const QString &relPath, const QString &alt, const QString &title)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("src"), relPath);
    attrs.insert(QStringLiteral("relPath"), relPath);
    attrs.insert(QStringLiteral("alt"), alt);
    attrs.insert(QStringLiteral("title"), title);
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("image"));
    node.insert(QStringLiteral("attrs"), attrs);
    return node;
}

QJsonObject voiceNode()
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("voiceId"), QStringLiteral("voice-export-1"));
    attrs.insert(QStringLiteral("voicePath"), QStringLiteral("voicenote/export.mp3"));
    attrs.insert(QStringLiteral("voiceSize"), 1234);
    attrs.insert(QStringLiteral("createTime"), QStringLiteral("2026-08-29 10:00:00"));
    attrs.insert(QStringLiteral("title"), QStringLiteral("会议录音"));
    attrs.insert(QStringLiteral("text"), QStringLiteral("语音转写结果\n第二行"));
    attrs.insert(QStringLiteral("translateUnfold"), false);
    attrs.insert(QStringLiteral("playing"), true);
    attrs.insert(QStringLiteral("progress"), 99);
    attrs.insert(QStringLiteral("translating"), true);
    QJsonObject node;
    node.insert(QStringLiteral("type"), QStringLiteral("voiceBlock"));
    node.insert(QStringLiteral("attrs"), attrs);
    return node;
}

QString sampleEnvelope()
{
    QJsonObject colorAttrs;
    colorAttrs.insert(QStringLiteral("color"), QStringLiteral("#123456"));

    QJsonObject doc;
    doc.insert(QStringLiteral("type"), QStringLiteral("doc"));
    doc.insert(QStringLiteral("content"), QJsonArray {
                   heading(2, QStringLiteral("导出标题")),
                   paragraph(QJsonArray { textNode(QStringLiteral("正文")),
                                          textNode(QStringLiteral("加粗"), QJsonArray { mark(QStringLiteral("bold")) }),
                                          textNode(QStringLiteral("彩色"), QJsonArray { mark(QStringLiteral("color"), colorAttrs) }) }),
                   nestedBulletList(),
                   taskList(),
                   paragraph(QJsonArray { imageNode(QStringLiteral("images/export-test.png"), QStringLiteral("截图Alt"), QStringLiteral("截图标题")) }),
                   voiceNode(),
               });

    QJsonObject envelope;
    envelope.insert(QStringLiteral("format"), QStringLiteral("tiptap"));
    envelope.insert(QStringLiteral("schemaVersion"), 1);
    envelope.insert(QStringLiteral("content"), doc);
    return QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
}

void prepareImageFixture()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + QStringLiteral("/images");
    QDir().mkpath(dirPath);
    QImage image(2, 2, QImage::Format_ARGB32);
    image.fill(Qt::red);
    image.save(dirPath + QStringLiteral("/export-test.png"), "PNG");
}

void prepareVoiceFixture()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + QStringLiteral("/voicenote");
    QDir().mkpath(dirPath);
    QFile file(dirPath + QStringLiteral("/export.mp3"));
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write("voice-data"), qint64(10));
}

QList<VNoteItem *> tiptapNoteList(VNoteItem *note)
{
    note->noteTitle = QStringLiteral("tiptap-note");
    note->metaDataRef() = sampleEnvelope();
    return QList<VNoteItem *> { note };
}

} // namespace

TEST(UT_TiptapDocumentExporter, plainTextExportsStructuredNodes)
{
    const QString text = TiptapDocumentExporter::toPlainText(sampleEnvelope());

    EXPECT_TRUE(text.contains(QStringLiteral("导出标题")));
    EXPECT_TRUE(text.contains(QStringLiteral("正文加粗彩色")));
    EXPECT_TRUE(text.contains(QStringLiteral("- 父项\n  - 子项")));
    EXPECT_TRUE(text.contains(QStringLiteral("[x] 任务完成")));
    EXPECT_TRUE(text.contains(QStringLiteral("[图片: 截图标题]")));
    EXPECT_TRUE(text.contains(QStringLiteral("语音转写结果\n第二行")));
    EXPECT_FALSE(text.contains(QStringLiteral("[语音:")));
    EXPECT_FALSE(text.contains(QStringLiteral("会议录音")));
    EXPECT_FALSE(text.contains(QStringLiteral("playing")));
    EXPECT_FALSE(text.contains(QStringLiteral("translateUnfold")));
    EXPECT_FALSE(text.contains(QStringLiteral("translating")));
}

TEST(UT_TiptapDocumentExporter, htmlExportsImagesTranscriptAndMarks)
{
    prepareImageFixture();

    const QString html = TiptapDocumentExporter::toHtml(sampleEnvelope());

    EXPECT_TRUE(html.contains(QStringLiteral("<h2>导出标题</h2>")));
    EXPECT_TRUE(html.contains(QStringLiteral("<strong>加粗</strong>")));
    EXPECT_TRUE(html.contains(QStringLiteral("style=\"color:#123456\"")));
    EXPECT_TRUE(html.contains(QStringLiteral("<ul><li><p>父项</p><ul><li><p>子项</p></li></ul></li></ul>")));
    EXPECT_TRUE(html.contains(QStringLiteral("data-type=\"taskItem\" data-checked=\"true\"")));
    EXPECT_TRUE(html.contains(QStringLiteral("<img")));
    EXPECT_TRUE(html.contains(QStringLiteral("src=\"data:image/png;base64,")));
    EXPECT_TRUE(html.contains(QStringLiteral("alt=\"截图Alt\"")));
    EXPECT_TRUE(html.contains(QStringLiteral("class=\"voiceBox\"")));
    EXPECT_TRUE(html.contains(QStringLiteral("语音转写结果<br>第二行")));
    EXPECT_FALSE(html.contains(QStringLiteral("playing")));
    EXPECT_FALSE(html.contains(QStringLiteral("progress")));
    EXPECT_FALSE(html.contains(QStringLiteral("translateUnfold")));
    EXPECT_FALSE(html.contains(QStringLiteral("translating")));
}

TEST(UT_TiptapDocumentExporter, voiceJsonsExportPersistedFieldsOnly)
{
    const QStringList voices = TiptapDocumentExporter::voiceJsons(sampleEnvelope());
    ASSERT_EQ(voices.size(), 1);

    const QJsonObject voice = QJsonDocument::fromJson(voices.first().toUtf8()).object();
    EXPECT_EQ(voice.value(QStringLiteral("type")).toInt(), 2);
    EXPECT_EQ(voice.value(QStringLiteral("voiceId")).toString(), QStringLiteral("voice-export-1"));
    EXPECT_EQ(voice.value(QStringLiteral("voicePath")).toString(), QStringLiteral("voicenote/export.mp3"));
    EXPECT_EQ(voice.value(QStringLiteral("title")).toString(), QStringLiteral("会议录音"));
    EXPECT_EQ(voice.value(QStringLiteral("text")).toString(), QStringLiteral("语音转写结果\n第二行"));
    EXPECT_TRUE(voice.value(QStringLiteral("state")).toBool());
    EXPECT_FALSE(voice.contains(QStringLiteral("playing")));
    EXPECT_FALSE(voice.contains(QStringLiteral("progress")));
    EXPECT_FALSE(voice.contains(QStringLiteral("translateUnfold")));
    EXPECT_FALSE(voice.contains(QStringLiteral("translating")));
}

TEST(UT_TiptapDocumentExporter, vnoteItemUsesTiptapExportSemantics)
{
    VNoteItem note;
    note.metaDataRef() = sampleEnvelope();

    EXPECT_TRUE(note.haveText());
    EXPECT_TRUE(note.haveVoice());
    EXPECT_EQ(note.voiceCount(), 1);
    EXPECT_EQ(note.getVoiceJsons().size(), 1);
    EXPECT_TRUE(note.getFullHtml().contains(QStringLiteral("语音转写结果<br>第二行")));
}


TEST(UT_TiptapDocumentExporter, exportWorkerWritesTiptapPlainText)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    VNoteItem note;
    ExportNoteWorker worker(dir.path(), ExportNoteWorker::ExportText, tiptapNoteList(&note));

    EXPECT_EQ(worker.exportText(), ExportNoteWorker::ExportOK);

    QFile file(dir.filePath(QStringLiteral("tiptap-note.txt")));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(file.readAll());
    EXPECT_TRUE(text.contains(QStringLiteral("正文加粗彩色")));
    EXPECT_TRUE(text.contains(QStringLiteral("[图片: 截图标题]")));
    EXPECT_TRUE(text.contains(QStringLiteral("语音转写结果")));
    EXPECT_FALSE(text.contains(QStringLiteral("[语音:")));
    EXPECT_FALSE(text.contains(QStringLiteral("会议录音")));
    EXPECT_FALSE(text.contains(QStringLiteral("playing")));
}

TEST(UT_TiptapDocumentExporter, exportWorkerWritesTiptapStandaloneHtml)
{
    prepareImageFixture();
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    VNoteItem note;
    ExportNoteWorker worker(dir.path(), ExportNoteWorker::ExportHtml, tiptapNoteList(&note));

    EXPECT_EQ(worker.exportAsHtml(), ExportNoteWorker::ExportOK);

    QFile file(dir.filePath(QStringLiteral("tiptap-note.html")));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString html = QString::fromUtf8(file.readAll());
    EXPECT_TRUE(html.startsWith(QStringLiteral("<!DOCTYPE html>")));
    EXPECT_TRUE(html.contains(QStringLiteral("src=\"data:image/png;base64,")));
    EXPECT_TRUE(html.contains(QStringLiteral("class=\"voiceBox\"")));
    EXPECT_TRUE(html.contains(QStringLiteral("语音转写结果<br>第二行")));
    EXPECT_FALSE(html.contains(QStringLiteral("progress")));
}

TEST(UT_TiptapDocumentExporter, exportWorkerCopiesTiptapVoiceFile)
{
    prepareVoiceFixture();
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    VNoteItem note;
    ExportNoteWorker worker(dir.path(), ExportNoteWorker::ExportVoice, tiptapNoteList(&note));

    EXPECT_EQ(worker.exportAllVoice(), ExportNoteWorker::ExportOK);

    QFile file(dir.filePath(QStringLiteral("会议录音.mp3")));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), QByteArray("voice-data"));
}

TEST(UT_TiptapDocumentExporter, htmlDropsUnsafeImageSrcFallbacks)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("src"), QStringLiteral("java\nscript:alert(1)"));
    attrs.insert(QStringLiteral("relPath"), QStringLiteral("../secret.png"));
    attrs.insert(QStringLiteral("alt"), QStringLiteral("bad"));

    QJsonObject image;
    image.insert(QStringLiteral("type"), QStringLiteral("image"));
    image.insert(QStringLiteral("attrs"), attrs);

    QJsonObject doc;
    doc.insert(QStringLiteral("type"), QStringLiteral("doc"));
    doc.insert(QStringLiteral("content"), QJsonArray { paragraph(QJsonArray { image }) });

    QJsonObject envelope;
    envelope.insert(QStringLiteral("format"), QStringLiteral("tiptap"));
    envelope.insert(QStringLiteral("schemaVersion"), 1);
    envelope.insert(QStringLiteral("content"), doc);

    const QString html = TiptapDocumentExporter::toHtml(QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)));

    EXPECT_FALSE(html.contains(QStringLiteral("javascript"), Qt::CaseInsensitive));
    EXPECT_FALSE(html.contains(QStringLiteral("secret.png")));
    EXPECT_FALSE(html.contains(QStringLiteral("<img")));
}
