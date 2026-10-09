// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for WebRichTextManager.

#include "webrichetextmanager.h"
#include "vnoteitem.h"
#include "tiptapchannelbridge.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTimer>

TEST(WebRichTextManagerUT, lifecycle)
{
    WebRichTextManager w;
    w.initConnect();
    w.initUpdateTimer();
    EXPECT_FALSE(w.hasPendingTextChange());
    EXPECT_EQ(-1, w.pendingTextChangeNoteId());
    EXPECT_EQ(-1, w.currentNoteId());
    w.requestUpdateNoteNow();
    w.updateNote();
    SUCCEED();
}

TEST(WebRichTextManagerUT, initDataNull)
{
    WebRichTextManager w;
    w.m_textChange = true;
    w.m_textChangeNoteId = 42;
    w.m_updateInProgress = true;
    w.m_updateRequestNoteId = 42;
    w.m_updateRequestSerial = 7;

    w.initData(nullptr, "");      // null -> clearJSContent path

    EXPECT_EQ(nullptr, w.m_noteData);
    EXPECT_FALSE(w.hasPendingTextChange());
    EXPECT_EQ(-1, w.pendingTextChangeNoteId());
    EXPECT_FALSE(w.m_updateInProgress);
    EXPECT_EQ(-1, w.m_updateRequestNoteId);
    EXPECT_EQ(0U, w.m_updateRequestSerial);
}


TEST(WebRichTextManagerUT, clearJSContentLoadsEmptyTiptapEnvelope)
{
    const QByteArray oldDisable = qgetenv("DVN_TIPTAP_DISABLE");
    const QByteArray oldLegacy = qgetenv("DVN_SUMMERNOTE_LEGACY");
    const QByteArray oldDebug = qgetenv("DVN_TIPTAP_DEBUG");
    qunsetenv("DVN_TIPTAP_DISABLE");
    qunsetenv("DVN_SUMMERNOTE_LEGACY");
    qunsetenv("DVN_TIPTAP_DEBUG");

    TiptapChannelBridge *bridge = TiptapChannelBridge::instance();
    bridge->notifyEditorReady();
    QSignalSpy spy(bridge, &TiptapChannelBridge::loadEnvelopeRequested);

    WebRichTextManager w;
    w.clearJSContent();

    ASSERT_GE(spy.count(), 1);
    const QString envelopeJson = spy.takeLast().at(0).toString();
    const QJsonObject envelope = QJsonDocument::fromJson(envelopeJson.toUtf8()).object();
    EXPECT_EQ(QStringLiteral("tiptap"), envelope.value(QStringLiteral("format")).toString());
    EXPECT_EQ(1, envelope.value(QStringLiteral("schemaVersion")).toInt());
    const QJsonObject content = envelope.value(QStringLiteral("content")).toObject();
    EXPECT_EQ(QStringLiteral("doc"), content.value(QStringLiteral("type")).toString());
    const QJsonArray nodes = content.value(QStringLiteral("content")).toArray();
    ASSERT_EQ(1, nodes.size());
    EXPECT_EQ(QStringLiteral("paragraph"), nodes.at(0).toObject().value(QStringLiteral("type")).toString());

    if (oldDisable.isEmpty())
        qunsetenv("DVN_TIPTAP_DISABLE");
    else
        qputenv("DVN_TIPTAP_DISABLE", oldDisable);
    if (oldLegacy.isEmpty())
        qunsetenv("DVN_SUMMERNOTE_LEGACY");
    else
        qputenv("DVN_SUMMERNOTE_LEGACY", oldLegacy);
    if (oldDebug.isEmpty())
        qunsetenv("DVN_TIPTAP_DEBUG");
    else
        qputenv("DVN_TIPTAP_DEBUG", oldDebug);
}

TEST(WebRichTextManagerUT, initDataWithNote)
{
    WebRichTextManager w;
    VNoteItem note;
    note.noteId = 42;
    w.initData(&note, "");
    EXPECT_EQ(42, w.currentNoteId());
    // clearJSContent spins a 100ms event loop -> fires the deferred setData
    w.clearJSContent();
    SUCCEED();
}

TEST(WebRichTextManagerUT, callbacks)
{
    WebRichTextManager w;
    w.onLoadFinsh();                 // m_noteData null -> skip
    w.onSetDataFinsh();              // m_setFocus false -> return
    w.onUpdateNoteWithResult(nullptr, "result");   // null path
    VNoteItem note;
    note.noteId = 7;
    w.onUpdateNoteWithResult(&note, "<p>hi</p>");
    EXPECT_EQ("<p>hi</p>", note.htmlCode);
    SUCCEED();
}

TEST(WebRichTextManagerUT, insertVoiceItem)
{
    WebRichTextManager w;
    w.insertVoiceItem("/tmp/voice-ut.wav", 1000);
    SUCCEED();
}

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================

// BUG 335623: 初始化界面（没有任何记事本）按 Ctrl+B 闪退。修复（a9eeef4c）：
// ① setData(nullptr) 早退；② initData 延迟回调解引用前检查 m_noteData 非空
// （src/common/webrichetextmanager.cpp:64-68, 104-107）。
// PMS: https://pms.uniontech.com/bug-view-335623.html  commit: a9eeef4c, befc63df
// PMS: https://pms.uniontech.com/bug-view-335263.html  commit: a9eeef4c, e3c51a74
TEST(WebRichTextManagerUT, BUG335623_nullGuardsOnEmptyNotebook)
{
    WebRichTextManager w;

    // 无记事本场景下空数据入口：必须早退且不崩溃，且不改变状态
    w.setData(nullptr, QStringLiteral(""));
    EXPECT_EQ(nullptr, w.m_noteData);

    // initData(有效笔记) 后，笔记在 50ms 延迟回调前被清空（如 Ctrl+B 触发
    // 的清理路径）→ 延迟 setData 必须跳过，不得解引用空指针
    VNoteItem note;
    note.noteId = 42;
    w.initData(&note, QStringLiteral(""));
    w.m_noteData = nullptr;
    {
        QEventLoop loop;
        QTimer::singleShot(150, &loop, &QEventLoop::quit);
        loop.exec();
    }
    EXPECT_EQ(nullptr, w.m_noteData);
    SUCCEED();
}

TEST(WebRichTextManagerUT, insertVoiceItemUsesTiptapBridgeWhenTiptapEnabled)
{
    const QByteArray oldDebug = qgetenv("DVN_TIPTAP_DEBUG");
    qputenv("DVN_TIPTAP_DEBUG", "1");

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString voicePath = QDir(appData).filePath(QStringLiteral("voicenote/voice-tiptap-ut.wav"));

    QSignalSpy spy(TiptapChannelBridge::instance(), &TiptapChannelBridge::insertVoiceBlock);
    WebRichTextManager w;
    w.insertVoiceItem(voicePath, 2345);

    ASSERT_EQ(1, spy.count());
    const QString voiceInfoJson = spy.takeFirst().at(0).toString();
    const QJsonObject voiceInfo = QJsonDocument::fromJson(voiceInfoJson.toUtf8()).object();
    EXPECT_EQ(2, voiceInfo.value(QStringLiteral("type")).toInt());
    EXPECT_FALSE(voiceInfo.value(QStringLiteral("voiceId")).toString().isEmpty());
    EXPECT_EQ(QStringLiteral("voicenote/voice-tiptap-ut.wav"), voiceInfo.value(QStringLiteral("voicePath")).toString());
    EXPECT_EQ(2345, voiceInfo.value(QStringLiteral("voiceSize")).toInt());
    EXPECT_FALSE(voiceInfo.value(QStringLiteral("title")).toString().isEmpty());

    if (oldDebug.isEmpty()) {
        qunsetenv("DVN_TIPTAP_DEBUG");
    } else {
        qputenv("DVN_TIPTAP_DEBUG", oldDebug);
    }
}
