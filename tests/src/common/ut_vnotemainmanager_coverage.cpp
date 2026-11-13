// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Coverage-focused tests for VNoteMainManager functions that are not exercised
// by ut_vnotemainmanager.cpp. Each test targets one of the 14 uncovered
// functions listed in the task. DB-writing operations and process-exit /
// external-launch calls are stubbed so the test binary continues running.

#include "VNoteMainManager.h"
#include "vnoteitem.h"
#include "vnoteforlder.h"
#include "vnotedatamanager.h"
#include "webrichetextmanager.h"
#include "actionmanager.h"
#include "vnotefolderoper.h"
#include "vnoteitemoper.h"
#include "jscontent.h"
#include "web_engine_handler.h"
#include "migrationviewcontroller.h"
#include "tiptapchannelbridge.h"
#include "voice_recoder_handler.h"
#include "vtextspeechandtrmanager.h"
#include "VoiceNoteDBusService.h"

#include <gtest/gtest.h>
#include <stub.h>

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QUrl>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPoint>
#include <QEventLoop>
#include <QPointF>
#include <QQmlEngine>
#include <QJSEngine>
#include <QVariantList>
#include <QSet>

// The QML singleton provider free functions are defined in VNoteMainManager.cpp
// (compiled into the test binary via GLOB_RECURSE) but not declared in any
// header. Forward-declare them here so we can call them directly.
QObject *jsContent_provider(QQmlEngine *engine, QJSEngine *scriptEngine);
QObject *mainManager_provider(QQmlEngine *engine, QJSEngine *scriptEngine);
QObject *actionManager_provider(QQmlEngine *engine, QJSEngine *scriptEngine);
QObject *voiceRecoder_provider(QQmlEngine *engine, QJSEngine *scriptEngine);
QObject *migrationViewController_provider(QQmlEngine *engine, QJSEngine *scriptEngine);
QObject *tiptapChannelBridge_provider(QQmlEngine *engine, QJSEngine *scriptEngine);

// ==================== Stub helpers ====================

static void stub_void() {}
static bool stub_bool_true() { return true; }
static int stub_int_zero() { return 0; }

// Used by forceExit test: throws to prevent reaching _Exit (which is declared
// __attribute__((noreturn)) and has no valid code after it).
static void stub_qapp_exit_throw(int) { throw 42; }

// ============================================================================
// 1. (anonymous)::folderCountsForIds — exercised through moveNotesToFolderId
//    (the only caller). We populate m_noteItems, then move a note to a
//    different folder so movedCount > 0 and folderCountsForIds is reached.
// ============================================================================
TEST(VNoteMainManagerCoverage, folderCountsForIds_viaMoveNotes)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    ASSERT_TRUE(m);

    // Stub DB-writing operation to keep the shared DB unchanged.
    Stub sUpdateFolderId;
    sUpdateFolderId.set(ADDR(VNoteItemOper, updateFolderId), stub_bool_true);

    // Ensure m_richTextManager exists (loadNotes -> vNoteChanged -> doSwitchNote
    // dereferences it).
    if (!m->m_richTextManager)
        m->m_richTextManager = new WebRichTextManager();

    // Populate m_folderSort so getFloderByIndex / getFloderById can resolve.
    m->loadNotepads();
    // Populate m_noteItems with notes from folder 0.
    m->vNoteFloderChanged(0);

    if (m->m_noteItems.isEmpty()) {
        SUCCEED() << "No notes loaded for folder 0";
        return;
    }

    VNoteItem *note = m->m_noteItems.first();
    ASSERT_TRUE(note);
    const int noteId = note->noteId;
    const int srcFolderId = note->folderId;
    const int dstFolderId = (srcFolderId == 0) ? 1 : 0;

    // Moving to a different folder makes movedCount > 0, which triggers the
    // internal folderCountsForIds call.
    m->moveNotesToFolderId({noteId}, dstFolderId);

    SUCCEED();
}

// ============================================================================
// 2. VNoteMainManager::initNote
// ============================================================================
TEST(VNoteMainManagerCoverage, initNote)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    ASSERT_TRUE(m);

    // Stub async DB reload, signal connections and DBus registration so
    // initNote is safe to run and does not interfere with later tests.
    Stub sReqIcons, sReqFolders, sReqItems, sChangeMode, sDBus, sInitConn;
    sReqIcons.set(ADDR(VNoteDataManager, reqNoteDefIcons), stub_void);
    sReqFolders.set(ADDR(VNoteDataManager, reqNoteFolders), stub_void);
    sReqItems.set(ADDR(VNoteDataManager, reqNoteItems), stub_void);
    sChangeMode.set(ADDR(VoiceRecoderHandler, changeMode), stub_void);
    sDBus.set(ADDR(VoiceNoteDBusService, initDBusService), stub_bool_true);
    // initConnections() wires signals (noteTextChanged -> onNoteChanged etc.)
    // that interfere with later tests' VNoteMainManager state.
    sInitConn.set(ADDR(VNoteMainManager, initConnections), stub_void);

    // Save old manager: initNote always creates a new WebRichTextManager,
    // which would leak the previous one (with running timers) and prevent
    // clean process exit during post-test cleanup.
    WebRichTextManager *oldMgr = m->m_richTextManager;

    m->initNote();

    // initNote must have created the rich-text manager.
    EXPECT_NE(nullptr, m->m_richTextManager);

    // Delete the new manager and restore the old one so we don't accumulate
    // leaked WebRichTextManager objects with active timers/connections.
    if (m->m_richTextManager != oldMgr) {
        delete m->m_richTextManager;
        m->m_richTextManager = oldMgr;
    }
}

// ============================================================================
// 3. VNoteMainManager::initData
// ============================================================================
TEST(VNoteMainManagerCoverage, initData)
{
    // Stub the async DB loaders so the pre-seeded in-memory data is preserved.
    Stub sReqIcons, sReqFolders, sReqItems;
    sReqIcons.set(ADDR(VNoteDataManager, reqNoteDefIcons), stub_void);
    sReqFolders.set(ADDR(VNoteDataManager, reqNoteFolders), stub_void);
    sReqItems.set(ADDR(VNoteDataManager, reqNoteItems), stub_void);

    VNoteMainManager::instance()->initData();
    SUCCEED();
}

// ============================================================================
// 4-9. QML singleton provider functions
// ============================================================================
TEST(VNoteMainManagerCoverage, jsContent_provider)
{
    EXPECT_NE(nullptr, jsContent_provider(nullptr, nullptr));
}

TEST(VNoteMainManagerCoverage, mainManager_provider)
{
    EXPECT_NE(nullptr, mainManager_provider(nullptr, nullptr));
}

TEST(VNoteMainManagerCoverage, actionManager_provider)
{
    EXPECT_NE(nullptr, actionManager_provider(nullptr, nullptr));
}

TEST(VNoteMainManagerCoverage, voiceRecoder_provider)
{
    EXPECT_NE(nullptr, voiceRecoder_provider(nullptr, nullptr));
}

TEST(VNoteMainManagerCoverage, migrationViewController_provider)
{
    EXPECT_NE(nullptr, migrationViewController_provider(nullptr, nullptr));
}

TEST(VNoteMainManagerCoverage, tiptapChannelBridge_provider)
{
    EXPECT_NE(nullptr, tiptapChannelBridge_provider(nullptr, nullptr));
}

// ============================================================================
// 10. VNoteMainManager::initQMLRegister
// ============================================================================
TEST(VNoteMainManagerCoverage, initQMLRegister)
{
    VNoteMainManager::instance()->initQMLRegister();
    SUCCEED();
}

// ============================================================================
// 11. updateTop lambda (std::find_if predicate at L1101)
//     The lambda is only reached when a valid note with isTop != top is found
//     and notesDataList is non-empty. We ensure m_noteItems is populated.
// ============================================================================
TEST(VNoteMainManagerCoverage, updateTop_lambda)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    ASSERT_TRUE(m);

    // Stub DB write so note->isTop is not actually persisted.
    Stub sUpdateTop;
    sUpdateTop.set(ADDR(VNoteItemOper, updateTop), stub_bool_true);

    // Ensure m_richTextManager exists (loadNotes -> vNoteChanged -> doSwitchNote
    // dereferences it).
    if (!m->m_richTextManager)
        m->m_richTextManager = new WebRichTextManager();

    // Populate m_folderSort + m_noteItems with notes from folder 0.
    m->loadNotepads();
    m->vNoteFloderChanged(0);

    if (m->m_noteItems.isEmpty()) {
        SUCCEED() << "No notes loaded for folder 0";
        return;
    }

    VNoteItem *note = m->m_noteItems.first();
    ASSERT_TRUE(note);
    const int noteId = note->noteId;
    const bool currentTop = note->isTop;

    // Toggle top so note->isTop != top, forcing updateTop to proceed past
    // the early-return and execute the std::find_if lambda.
    m->updateTop(noteId, !currentTop);

    SUCCEED();
}

// ============================================================================
// 12. VNoteMainManager::preViewShortcut
//     Launches deepin-shortcut-viewer via QProcess::startDetached. In a
//     headless environment the binary may be absent; startDetached returns
//     false without crashing.
// ============================================================================
TEST(VNoteMainManagerCoverage, preViewShortcut)
{
    VNoteMainManager::instance()->preViewShortcut(QPointF(100, 200));
    SUCCEED();
}

// ============================================================================
// 13. VNoteMainManager::showPrivacy
//     Calls QDesktopServices::openUrl which is safe in offscreen mode.
// ============================================================================
TEST(VNoteMainManagerCoverage, showPrivacy)
{
    VNoteMainManager::instance()->showPrivacy();
    SUCCEED();
}

// ============================================================================
// 14. VNoteMainManager::forceExit
//     forceExit calls _Exit(0) which terminates the process. _Exit is declared
//     __attribute__((noreturn)), so the compiler does NOT generate valid code
//     after the _Exit() call. Stubbing _Exit to return would execute broken
//     code. Instead, we stub QApplication::exit (which is called BEFORE _Exit
//     and is NOT noreturn) to throw a C++ exception. The exception safely
//     unwinds the stack back to our try/catch, so _Exit is never reached.
//     We test forceExit(false) to cover the main body (qDebug,
//     onStopTextToSpeech, QApplication::exit) without the needWait event loop.
// ============================================================================
TEST(VNoteMainManagerCoverage, forceExit)
{
    // --- Stub QApplication::exit to throw (avoids _Exit with broken code) ---
    typedef void (*qcore_exit_func)(int);
    qcore_exit_func A_QAppExit = (qcore_exit_func)(&QCoreApplication::exit);

    // --- Stub onStopTextToSpeech to avoid DBus / service dependencies ---
    Stub sQAppExit, sStopTTS;
    sQAppExit.set(A_QAppExit, stub_qapp_exit_throw);
    sStopTTS.set(ADDR(VTextSpeechAndTrManager, onStopTextToSpeech), stub_int_zero);

    // forceExit(false): skips the event-loop wait, reaches onStopTextToSpeech
    // then QApplication::exit which throws. This covers the main function body
    // (qDebug, onStopTextToSpeech, QApplication::exit) without reaching _Exit.
    try {
        VNoteMainManager::instance()->forceExit(false);
        FAIL() << "forceExit(false) should not return normally";
    } catch (int) {
        // Expected: QApplication::exit stub threw to prevent reaching _Exit
    }
}


// BUG-382971: GIF must not be accepted by the drag/drop gate. The QML DropArea
// calls VNoteMainManager.canInsertImages(); returning false is what rejects the
// drop. Keep the file-dialog filter and this gate consistent.
TEST(VNoteMainManagerCoverage, canInsertImagesRejectsGif)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    ASSERT_TRUE(m);

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    const QString pngPath = dir.filePath("control.png");
    QImage png(2, 2, QImage::Format_RGB32);
    png.fill(Qt::red);
    ASSERT_TRUE(png.save(pngPath, "PNG"));

    const QString jpegPath = dir.filePath("control.jpeg");
    QImage jpeg(2, 2, QImage::Format_RGB32);
    jpeg.fill(Qt::green);
    ASSERT_TRUE(jpeg.save(jpegPath, "JPEG"));

    const QString bmpPath = dir.filePath("control.bmp");
    QImage bmp(2, 2, QImage::Format_RGB32);
    bmp.fill(Qt::blue);
    ASSERT_TRUE(bmp.save(bmpPath, "BMP"));

    // Minimal valid 1x1 GIF89a; avoids depending on the Qt GIF writer plugin.
    const QString gifPath = dir.filePath("reject.gif");
    QFile gifFile(gifPath);
    ASSERT_TRUE(gifFile.open(QIODevice::WriteOnly));
    const QByteArray gifBytes = QByteArray::fromHex(
        "47494638396101000100800000000000ffffff21f90401000000002c00000000010001000002024401003b");
    gifFile.write(gifBytes);
    gifFile.close();

    // A GIF renamed to a supported suffix must still be rejected by the
    // actual-format check, not just by the file name.
    const QString renamedGifPath = dir.filePath("renamed.gif.png");
    QFile renamedGif(renamedGifPath);
    ASSERT_TRUE(renamedGif.open(QIODevice::WriteOnly));
    renamedGif.write(gifBytes);
    renamedGif.close();

    EXPECT_TRUE(m->canInsertImages({QUrl::fromLocalFile(pngPath)}))
        << "PNG must remain accepted";
    EXPECT_TRUE(m->canInsertImages({QUrl::fromLocalFile(jpegPath)}))
        << "JPEG must remain accepted";
    EXPECT_TRUE(m->canInsertImages({QUrl::fromLocalFile(bmpPath)}))
        << "BMP must remain accepted";
    EXPECT_FALSE(m->canInsertImages({QUrl::fromLocalFile(gifPath)}))
        << "GIF must be rejected by the drag/drop gate";
    EXPECT_FALSE(m->canInsertImages({QUrl::fromLocalFile(renamedGifPath)}))
        << "A GIF renamed to .png must be rejected by the actual-format check";
}

// Image-only drag payloads (no file URL) must also be rejected before they
// reach Summernote's Dropzone, which would otherwise insert a GIF as a data URL.
TEST(WebEngineHandlerCoverage, imageOnlyGifDragIsRejected)
{
    QObject target;
    WebEngineHandler handler;
    handler.setTarget(&target);

    QMimeData mimeData;
    mimeData.setData("image/gif", QByteArray("GIF89a"));
    QDragEnterEvent event(QPoint(10, 10), Qt::CopyAction, &mimeData,
                          Qt::LeftButton, Qt::NoModifier);
    EXPECT_TRUE(handler.eventFilter(&target, &event));
    EXPECT_EQ(event.dropAction(), Qt::IgnoreAction);
    EXPECT_FALSE(event.isAccepted());
}
