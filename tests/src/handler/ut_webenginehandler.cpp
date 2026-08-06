// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for WebEngineHandler. Avoids paths that open modal dialogs
// (QFileDialog in saveMP3/saveAsFile) and synchronous JS that would block
// the event loop (callJsSynchronous with a target set). TTS-related menu
// actions are skipped to avoid real speech side effects.

#include "web_engine_handler.h"
#include "actionmanager.h"
#include "vnoteitem.h"

#include <gtest/gtest.h>
#include <QObject>
#include <QDropEvent>
#include <QMimeData>
#include <QEvent>
#include <QPointF>

TEST(WebEngineHandlerUT, lifecycleAndTarget)
{
    WebEngineHandler h;
    // callJsSynchronous must run while target is null (otherwise blocks)
    EXPECT_TRUE(h.callJsSynchronous("noop").isNull());
    h.onCallJsResult(QVariant("result"));

    QObject o1, o2;
    EXPECT_EQ(nullptr, h.target());
    h.setTarget(&o1);     // install event filter
    h.setTarget(&o1);     // same -> no change
    h.setTarget(&o2);     // change -> remove from o1, install on o2
    h.setTarget(nullptr); // remove from o2
    SUCCEED();
}

TEST(WebEngineHandlerUT, contextMenuAndVoiceInsert)
{
    WebEngineHandler h;
    // MaxMenu -> default branch (no request deref)
    h.onSaveMenuParam(WebEngineHandler::MaxMenu, QVariant());
    h.onContextMenuRequested(nullptr);
    // VoiceMenu with empty json -> parse fails -> early return (no request deref)
    h.onSaveMenuParam(WebEngineHandler::VoiceMenu, QVariant());
    h.onContextMenuRequested(nullptr);
    h.onInsertVoiceItem("/tmp/voice-ut.wav", 1000);
    SUCCEED();
}

TEST(WebEngineHandlerUT, themeAndMenuParam)
{
    WebEngineHandler h;
    h.onThemeChanged();
    h.onSaveMenuParam(WebEngineHandler::PictureMenu, QVariant());
    SUCCEED();
}

TEST(WebEngineHandlerUT, menuClicked)
{
    WebEngineHandler h;
    h.m_voiceBlock.clear();   // avoid QFileDialog in saveMP3
    h.onMenuClicked(ActionManager::VoiceAsSave);   // saveMP3 -> null -> SaveFailed
    h.onMenuClicked(ActionManager::VoiceToText);  // setAudioToText(null)
    h.onMenuClicked(ActionManager::VoiceDelete);
    h.onMenuClicked(ActionManager::PictureDelete);
    h.onMenuClicked(ActionManager::TxtDelete);
    h.onMenuClicked(ActionManager::VoiceSelectAll);
    h.onMenuClicked(ActionManager::VoiceCopy);
    h.onMenuClicked(ActionManager::VoiceCut);
    // VoicePaste -> onPaste(isVoicePaste()) -> onPaste(false) -> clipboard null deref in offscreen env: skip
    h.onMenuClicked(ActionManager::PictureView);  // normalizePicturePath("") -> viewPicture("")
    h.onMenuClicked(ActionManager::PictureSaveAs); // savePictureAs -> SaveFailed
    h.onMenuClicked(ActionManager::Invalid);       // default
    SUCCEED();
}

TEST(WebEngineHandlerUT, pasteBranches)
{
    WebEngineHandler h;
    h.onPaste(true);    // triggerWebAction Paste (safe; returns before clipboard)
    EXPECT_FALSE(h.isVoicePaste());   // target null -> {} -> false
    // onPaste(false) is NOT exercised: QApplication::clipboard()->mimeData()
    // returns null in the offscreen test environment and onPaste() does not
    // null-check it (src/handler/web_engine_handler.cpp:~697) -> segfault.
    // Reported as a source defect.
    SUCCEED();
}

TEST(WebEngineHandlerUT, eventFilterDrag)
{
    WebEngineHandler h;
    QObject target;
    h.setTarget(&target);
    QEvent leave(QEvent::DragLeave);
    EXPECT_FALSE(h.eventFilter(&target, &leave));
    QMimeData md;   // no urls
    QDropEvent enter(QPointF(0, 0), Qt::CopyAction, &md, Qt::NoButton, Qt::NoModifier);
    EXPECT_FALSE(h.eventFilter(&target, &enter));
    // event for a non-target object -> passes through
    QObject other;
    EXPECT_FALSE(h.eventFilter(&other, &leave));
    SUCCEED();
}
