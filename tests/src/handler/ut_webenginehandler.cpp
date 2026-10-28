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
#include "VNoteMainManager.h"

#include <gtest/gtest.h>
#include <QEvent>
#include <QObject>
#include <QPointF>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QVariantMap>

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
    // MaxMenu -> default branch (no request use)
    h.onSaveMenuParam(WebEngineHandler::MaxMenu, QVariant());
    h.onContextMenuRequested(QVariantMap());
    // VoiceMenu with empty json -> parse fails -> early return (no request use)
    h.onSaveMenuParam(WebEngineHandler::VoiceMenu, QVariant());
    h.onContextMenuRequested(QVariantMap());
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

// QtWebEngine 时代存在 WebEngineHandler::eventFilter 拦截 DragLeave 等事件的
// 逻辑；WebKitGTK 方案下拖放由 webhelper 独立窗接管（filesDropped/textDropped），
// eventFilter override 已随架构移除，事件过滤回归 QObject 基类行为。
TEST(WebEngineHandlerUT, eventFilterRemoved_fallsBackToBase)
{
    WebEngineHandler h;
    QObject target;
    QObject other;
    QEvent leave(QEvent::DragLeave);
    // 无重写：任意对象均走 QObject::eventFilter，返回 false 不拦截
    EXPECT_FALSE(h.eventFilter(&target, &leave));
    EXPECT_FALSE(h.eventFilter(&other, &leave));
    SUCCEED();
}

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================

// BUG 300535: 已接入输入/输出设备情况下拖拽操作异常。
// 修复随架构演进迁移：QtWebEngine 时代由 WebEngineHandler::eventFilter 在宿主
// 侧拦截非图片拖放（commit 1c05a9e4, 4ebe6e2f）；WebKitGTK 方案下拖放由
// webhelper 独立窗转发（WebKitWebView::filesDropped → WebEngineView.qml
// onFilesDropped），非图片过滤收敛到 VNoteMainManager::canInsertImages。
// 此处验证过滤语义不变：txt 拒绝、png 放行、混合列表整体拒绝。
// PMS: https://pms.uniontech.com/bug-view-300535.html
TEST(WebEngineHandlerUT, BUG300535_nonImageDropStillFiltered)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    ASSERT_NE(nullptr, m);

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString imgPath = dir.filePath("a.png");
    ASSERT_TRUE(QImage(2, 2, QImage::Format_RGB32).save(imgPath, "PNG"));
    const QString txtPath = dir.filePath("a.txt");
    {
        QFile f(txtPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("x", 1);
    }

    const QUrl img = QUrl::fromLocalFile(imgPath);
    const QUrl txt = QUrl::fromLocalFile(txtPath);

    // 合法图片拖入：放行
    EXPECT_TRUE(m->canInsertImages({img}));
    // 非图片文件：过滤
    EXPECT_FALSE(m->canInsertImages({txt}));
    // 图片+非图片混合列表：整体拒绝
    EXPECT_FALSE(m->canInsertImages({img, txt}));
    // 空列表：拒绝
    EXPECT_FALSE(m->canInsertImages({}));
    // 非本地 URL：拒绝
    EXPECT_FALSE(m->canInsertImages({QUrl(QStringLiteral("https://example.com/a.png"))}));
}
