// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for WebRichTextManager.

#include "webrichetextmanager.h"
#include "vnoteitem.h"
#include "tiptapchannelbridge.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>

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
    w.initData(nullptr, "");      // null -> clearJSContent path
    EXPECT_EQ(nullptr, w.m_noteData);
    SUCCEED();
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

TEST(WebRichTextManagerUT, insertVoiceItemUsesTiptapBridgeWhenDebugEnabled)
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
