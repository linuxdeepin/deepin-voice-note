// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Coverage tests for previously-uncovered functions across handler/common:
//   VoicePlayerHandler ctor lambda (playEnd),
//   VoiceRecoderHandler::onAudioBufferProbed + onReduceNoiseChanged lambda,
//   VoiceToTextHandler::onA2TStart lambda,
//   VNoteA2TManager::initSession lambda,
//   VNoteDataManager::reqNoteFolders / reqNoteItems,
//   VNoteBlock::~VNoteBlock (D0),
//   VNVoiceBlock::releaseSpecificData,
//   WebRichTextManager::initConnect lambda.
//
// Build enables -fno-access-control; private/protected members are accessible.

#include "voice_player_handler.h"
#include "voiceplayerbase.h"
#include "voice_recoder_handler.h"
#include "voice_to_text_handler.h"
#include "vnotea2tmanager.h"
#include "vnotedatamanager.h"
#include "vnoteitem.h"
#include "vnoteforlder.h"
#include "webrichetextmanager.h"
#include "jscontent.h"
#include "datatypedef.h"

#include <gtest/gtest.h>
#include <stub.h>

#include <QAudioBuffer>
#include <QSharedPointer>
#include <QThreadPool>
#include <QTimer>
#include <QTest>

// ---------------------------------------------------------------------------
// Stub helpers
// ---------------------------------------------------------------------------
static void stub_startAsr(const QString &, qint64) { /* no-op: avoid real ASR */ }

// Global `timer` is defined (non-static) in vnotea2tmanager.cpp; declare it
// extern so we can trigger its timeout signal.
extern QTimer *timer;

// ===========================================================================
// VoicePlayerHandler constructor lambda — voice_player_handler.cpp:43
//
// The lambda is connected to VoicePlayerBase::playEnd inside the constructor.
// We construct the handler, then emit m_player->playEnd to fire the lambda.
// ===========================================================================
TEST(HandlerCoverage, VoicePlayerHandler_ctorPlayEndLambda)
{
    VoicePlayerHandler h;
    ASSERT_NE(nullptr, h.m_player);

    // playEnd is a protected signal of VoicePlayerBase; -fno-access-control
    // lets us trigger it directly, which invokes the connected lambda.
    h.m_player->playEnd();
    qApp->processEvents();
    SUCCEED();
}

// ===========================================================================
// VoiceRecoderHandler::onAudioBufferProbed — voice_recoder_handler.cpp:286
//
// An empty QAudioBuffer is safe: frameCount() == 0 so the sample loop is
// skipped, and startTime() is -1 (≠ m_recordMsec 0) so the if-branch runs.
// ===========================================================================
TEST(HandlerCoverage, VoiceRecoderHandler_onAudioBufferProbed)
{
    auto *r = VoiceRecoderHandler::instance();
    ASSERT_NE(nullptr, r);

    // Reset state so the startTime check is entered
    r->m_recordMsec = 0;

    QAudioBuffer emptyBuffer;
    // onAudioBufferProbed is private; -fno-access-control grants access.
    r->onAudioBufferProbed(emptyBuffer);

    // m_recordMsec should now be the buffer's startTime()
    EXPECT_NE(0, r->m_recordMsec);
    SUCCEED();
}

// ===========================================================================
// VoiceRecoderHandler::onReduceNoiseChanged::{lambda#1} — L326
//
// onReduceNoiseChanged schedules a QTimer::singleShot(200,...) lambda.
// We call it then spin an event loop long enough for the timer to fire.
// ===========================================================================
TEST(HandlerCoverage, VoiceRecoderHandler_onReduceNoiseChangedLambda)
{
    auto *r = VoiceRecoderHandler::instance();
    ASSERT_NE(nullptr, r);

    r->m_type = VoiceRecoderHandler::Idle;
    r->m_currentMode = 1;

    // Fires the deferred lambda after 200 ms
    r->onReduceNoiseChanged(true);

    // Spin event loop so the 200 ms singleShot timer fires
    QTest::qWait(350);
    SUCCEED();
}

// ===========================================================================
// VoiceToTextHandler::onA2TStart::{lambda#1} — voice_to_text_handler.cpp:76
//
// onA2TStart schedules QTimer::singleShot(0, ...) which calls startAsr.
// We stub startAsr, set m_voiceBlock, call onA2TStart, then processEvents.
// ===========================================================================
TEST(HandlerCoverage, VoiceToTextHandler_onA2TStartLambda)
{
    VoiceToTextHandler h;
    Stub stub;
    stub.set(ADDR(VNoteA2TManager, startAsr), stub_startAsr);

    QSharedPointer<VNVoiceBlock> blk = QSharedPointer<VNVoiceBlock>::create();
    blk->voicePath = "/tmp/ut_a2t_start.wav";
    blk->voiceSize = 1000;
    h.m_voiceBlock = blk;
    h.m_originalVoiceId = "ut-voice-id";

    // onA2TStart is private; -fno-access-control grants access.
    h.onA2TStart();

    // Fire the singleShot(0) lambda → calls stubbed startAsr
    qApp->processEvents();
    SUCCEED();
}

// ===========================================================================
// VNoteA2TManager::initSession::{lambda#1} — vnotea2tmanager.cpp:44
//
// initSession (called from constructor) connects a lambda to the global
// `timer`'s timeout signal. We construct the manager then emit
// timer->timeout() to fire the lambda body.
// ===========================================================================
TEST(HandlerCoverage, VNoteA2TManager_initSessionLambda)
{
    // Construction triggers initSession which creates the global timer and
    // connects the lambda.
    VNoteA2TManager mgr;

    ASSERT_NE(nullptr, timer);

    // In Qt6, QTimer::timeout has a QPrivateSignal arg so we cannot emit it
    // directly from outside. Instead, arm the single-shot timer with a 0 ms
    // interval and spin the event loop so it fires → the connected lambda
    // runs (emit asrError + operState).
    timer->setSingleShot(true);
    timer->start(0);
    QTest::qWait(50);
    SUCCEED();
}

// ===========================================================================
// VNoteDataManager::reqNoteFolders — vnotedatamanager.cpp:439
// VNoteDataManager::reqNoteItems   — vnotedatamanager.cpp:459
//
// These create background workers. We use a local VNoteDataManager with
// initialised maps and wait for the thread pool to drain.
// ===========================================================================
TEST(HandlerCoverage, VNoteDataManager_reqNoteFolders)
{
    VNoteDataManager mgr;
    mgr.m_qspNoteFoldersMap.reset(new VNOTE_FOLDERS_MAP());
    mgr.m_qspAllNotesMap.reset(new VNOTE_ALL_NOTES_MAP());

    // Takes the if-branch (m_pNotesLoadThread is nullptr for a fresh mgr)
    mgr.reqNoteFolders();
    // Drain background workers so they don't outlive the local instance
    QThreadPool::globalInstance()->waitForDone(5000);
    SUCCEED();
}

TEST(HandlerCoverage, VNoteDataManager_reqNoteItems)
{
    VNoteDataManager mgr;
    mgr.m_qspNoteFoldersMap.reset(new VNOTE_FOLDERS_MAP());
    mgr.m_qspAllNotesMap.reset(new VNOTE_ALL_NOTES_MAP());

    // if-branch: m_pNotesLoadThread is nullptr
    mgr.reqNoteItems();
    // else-branch: m_pNotesLoadThread is now set → exercises the other path
    mgr.reqNoteItems();
    QThreadPool::globalInstance()->waitForDone(5000);
    SUCCEED();
}

// ===========================================================================
// VNoteBlock::~VNoteBlock (D0 deleting destructor) — vnoteitem.cpp:444
// ===========================================================================
TEST(HandlerCoverage, VNoteBlock_Destructor_D0)
{
    // VNTextBlock extends VNoteBlock; deleting via base pointer triggers
    // VNoteBlock::~VNoteBlock (D0) after the derived destructor.
    VNoteBlock *block = new VNTextBlock();
    delete block;
    SUCCEED();
}

// ===========================================================================
// VNVoiceBlock::releaseSpecificData — vnoteitem.cpp:504
//
// The voicePath is empty by default → QFileInfo::exists() returns false →
// the else-branch (log + return) is taken; no file is touched.
// ===========================================================================
TEST(HandlerCoverage, VNVoiceBlock_releaseSpecificData)
{
    VNVoiceBlock block;
    // voicePath is "" by default → file does not exist → else branch
    block.releaseSpecificData();
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Extra: exercise the file-removal branch of releaseSpecificData with a
// temporary file we create and own.
// ---------------------------------------------------------------------------
TEST(HandlerCoverage, VNVoiceBlock_releaseSpecificData_RemovesFile)
{
    VNVoiceBlock block;
    // Create a temp file so fileInfo.exists() is true.
    QString tmpPath = "/tmp/ut_vnvoice_release_test.wav";
    {
        QFile f(tmpPath);
        f.open(QIODevice::WriteOnly);
        f.write("test");
        f.close();
    }
    block.voicePath = tmpPath;
    block.releaseSpecificData();    // should remove the file
    EXPECT_FALSE(QFile::exists(tmpPath));
}

// ===========================================================================
// WebRichTextManager::initConnect::{lambda#1} — webrichetextmanager.cpp:60
//
// initConnect (called from constructor) connects a lambda to
// JsContent::textChange. We construct the manager then emit textChange.
// ===========================================================================
TEST(HandlerCoverage, WebRichTextManager_initConnectLambda)
{
    WebRichTextManager w;
    // The lambda was connected in the constructor's initConnect call.
    // textChange is a protected signal of JsContent; -fno-access-control
    // lets us trigger it, invoking the connected lambda synchronously
    // (DirectConnection — same thread).
    JsContent::instance()->textChange();
    qApp->processEvents();

    // After the lambda runs, m_textChange should be true
    EXPECT_TRUE(w.m_textChange);
    SUCCEED();
}
