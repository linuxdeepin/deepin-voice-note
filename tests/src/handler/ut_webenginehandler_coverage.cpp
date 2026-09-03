// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Coverage-focused unit tests for web_engine_handler.cpp.
//
// Targets the 18 previously-uncovered callables:
//   - 2 constructor lambdas (playStatusChanged / recoderStateChange routing)
//   - 12 connectWebContent() lambdas (JsContent + TiptapChannelBridge + player wiring)
//   - 1 processVoiceMenuRequest() single-shot lambda
//   - 3 regular methods: processTextMenuRequest / processPictureMenuRequest / saveAsFile
//
// Strategy:
//   * Lambdas wired via QObject::connect are exercised by emitting the source
//     signal (signals are protected, but -fno-access-control is set in
//     tests/CMakeLists.txt so direct emission compiles and runs).
//   * process{Text,Picture}MenuRequest dereference a QWebEngineContextMenuRequest;
//     its accessors are out-of-line in libQt6WebEngineCore so the deepin Stub
//     intercepts them cross-translation-unit, allowing a fake pointer.
//   * saveAsFile opens a QFileDialog; ADDR(QFileDialog, getSaveFileName) is
//     stubbed to avoid modal blocking (established project pattern).
//   * VoiceToTextHandler::checkNetworkState is stubbed so the
//     voiceToTextRequested lambda cannot schedule a real ASR D-Bus task.

#include "web_engine_handler.h"
#include "voice_player_handler.h"
#include "voice_recoder_handler.h"
#include "voice_to_text_handler.h"
#include "vnoteitem.h"
#include "tiptapchannelbridge.h"
#include "jscontent.h"

#include <gtest/gtest.h>
#include <stub.h>

#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QObject>
#include <QPoint>
#include <QSignalSpy>
#include <QTimer>
#include <QWebEngineContextMenuRequest>

// ---------------------------------------------------------------------------
// Stub functions
// ---------------------------------------------------------------------------

// Forces setAudioToText into the safe no-network early-return branch so the
// voiceToTextRequested lambda never schedules a deferred startAsr().
static bool stub_checkNetworkState_false() { return false; }

// Cancels QFileDialog so saveAsFile hits the early-return "" path.
static QString stub_getSaveFileName_empty() { return QString(); }

// Returns a fixed destination path so saveAsFile executes the copy path.
static QString stub_getSaveFileName_dst()
{
    return QStringLiteral("/tmp/ut_webenginehandler_dst.txt");
}

// QWebEngineContextMenuRequest accessors: real implementations deref a null
// d-pointer when the request is faked, so redirect to no-op returns.
static QWebEngineContextMenuRequest::EditFlags stub_editFlags_all()
{
    using F = QWebEngineContextMenuRequest::EditFlag;
    return QWebEngineContextMenuRequest::EditFlags(
        F::CanCopy | F::CanCut | F::CanDelete | F::CanPaste | F::CanSelectAll);
}

static QPoint stub_position_value() { return QPoint(12, 34); }

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Pumps the event loop long enough for QTimer::singleShot(0, ...) callbacks
// (used by processVoiceMenuRequest's missing-file branch) to fire.
static void pumpEvents(int ms = 100)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

// ===========================================================================
// 1. Constructor lambdas (L137, L147)
// ===========================================================================

// L137 lambda: VoicePlayerHandler::playStatusChanged -> emit playingVoice(bool)
TEST(WebEngineHandlerCoverageUT, ConstructorLambda_PlayStateChanged)
{
    WebEngineHandler h;
    QSignalSpy spy(&h, &WebEngineHandler::playingVoice);
    ASSERT_TRUE(spy.isValid());

    // End state -> playingVoice(false)
    emit h.m_voicePlayerHandler->playStatusChanged(VoicePlayerHandler::End);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());

    // Non-End state -> playingVoice(true)
    emit h.m_voicePlayerHandler->playStatusChanged(VoicePlayerHandler::Playing);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());

    emit h.m_voicePlayerHandler->playStatusChanged(VoicePlayerHandler::Paused);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());
}

// L147 lambda: VoiceRecoderHandler::recoderStateChange -> emit callJsSetVoicePlayBtnEnable
TEST(WebEngineHandlerCoverageUT, ConstructorLambda_RecoderStateChange)
{
    WebEngineHandler h;
    QSignalSpy spy(JsContent::instance(), &JsContent::callJsSetVoicePlayBtnEnable);
    ASSERT_TRUE(spy.isValid());

    // Recording -> enable=false
    emit VoiceRecoderHandler::instance()->recoderStateChange(VoiceRecoderHandler::Recording);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());

    // Idle -> enable=true
    emit VoiceRecoderHandler::instance()->recoderStateChange(VoiceRecoderHandler::Idle);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());

    // Paused -> enable=true
    emit VoiceRecoderHandler::instance()->recoderStateChange(VoiceRecoderHandler::Paused);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());
}

// ===========================================================================
// 2. connectWebContent() lambdas (L332-L410)
// ===========================================================================

// L332 lambda: getfontinfo -> callJsSetFontList(fontList, defaultFont)
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_GetFontInfo)
{
    WebEngineHandler h;
    QSignalSpy spy(JsContent::instance(), &JsContent::callJsSetFontList);
    ASSERT_TRUE(spy.isValid());

    emit JsContent::instance()->getfontinfo();
    EXPECT_EQ(spy.count(), 1);
}

// L336 lambda: loadFinsh -> emit loadRichText() + onThemeChanged()
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_LoadFinsh)
{
    WebEngineHandler h;
    QSignalSpy spy(&h, &WebEngineHandler::loadRichText);
    ASSERT_TRUE(spy.isValid());

    emit JsContent::instance()->loadFinsh();
    EXPECT_EQ(spy.count(), 1);
}

// L348 lambda: editorReady -> sendFontList(fontList, defaultFont).
// sendFontList only emits fontListProvided when m_editorReady is true, so we
// go through notifyEditorReady() which both sets the flag and emits editorReady
// (triggering the connected L348 lambda deterministically).
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_EditorReady)
{
    WebEngineHandler h;
    QSignalSpy spy(TiptapChannelBridge::instance(), &TiptapChannelBridge::fontListProvided);
    ASSERT_TRUE(spy.isValid());

    TiptapChannelBridge::instance()->notifyEditorReady();
    EXPECT_GE(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toStringList(), h.m_fontList);
}

// L353 lambda: voicePlaybackRequested(json, isSame) -> playVoice(json, isSame)
// playVoice with empty json resolves to a missing file path -> voiceFileError.
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_VoicePlaybackRequested)
{
    WebEngineHandler h;
    emit TiptapChannelBridge::instance()->voicePlaybackRequested(
        QStringLiteral("{}"), false);
    emit TiptapChannelBridge::instance()->voicePlaybackRequested(
        QStringLiteral("{\"voicePath\":\"/tmp/ut_webenginehandler_noexist.wav\"}"), true);
    pumpEvents(50);
    SUCCEED();
}

// L358 lambda: voicePlaybackStopRequested -> m_voicePlayerHandler->onStop()
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_VoicePlaybackStopRequested)
{
    WebEngineHandler h;
    emit TiptapChannelBridge::instance()->voicePlaybackStopRequested();
    SUCCEED();
}

// L362 lambda: voicePlaybackSeekRequested(ms) -> setPlayPosition(ms)
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_VoicePlaybackSeekRequested)
{
    WebEngineHandler h;
    emit TiptapChannelBridge::instance()->voicePlaybackSeekRequested(1234);
    SUCCEED();
}

// L366 lambda: voiceToTextRequested(json) -> setAudioToText(voiceBlock).
// checkNetworkState stubbed to false so setAudioToText returns early.
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_VoiceToTextRequested)
{
    WebEngineHandler h;
    Stub stub;
    stub.set(ADDR(VoiceToTextHandler, checkNetworkState),
             stub_checkNetworkState_false);

    // NoNetwork is emitted by the handler on the early-return path.
    QSignalSpy spy(h.m_voiceToTextHandler, &VoiceToTextHandler::noNetworkConnection);
    ASSERT_TRUE(spy.isValid());

    emit TiptapChannelBridge::instance()->voiceToTextRequested(
        QStringLiteral("{\"voiceId\":\"v1\",\"voicePath\":\"v.wav\",\"voiceSize\":0}"));
    EXPECT_EQ(spy.count(), 1);
}

// L381 lambda: playStatusChanged -> emitVoicePlaybackStateChanged when voiceId set.
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_PlayStateChanged_Bridge)
{
    WebEngineHandler h;
    TiptapChannelBridge::instance()->setCurrentVoiceId(
        QStringLiteral("ut-vid-state"));

    QSignalSpy spy(TiptapChannelBridge::instance(),
                   &TiptapChannelBridge::voicePlaybackStateChanged);
    ASSERT_TRUE(spy.isValid());

    emit h.m_voicePlayerHandler->playStatusChanged(VoicePlayerHandler::Playing);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("ut-vid-state"));

    // Empty voiceId branch: lambda body runs but does not emit the bridge signal.
    TiptapChannelBridge::instance()->setCurrentVoiceId(QString());
    emit h.m_voicePlayerHandler->playStatusChanged(VoicePlayerHandler::Paused);
    EXPECT_EQ(spy.count(), 0);
}

// L388 lambda: playPositionChanged -> emitVoicePlaybackPositionChanged.
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_PlayPositionChanged)
{
    WebEngineHandler h;
    TiptapChannelBridge::instance()->setCurrentVoiceId(
        QStringLiteral("ut-vid-pos"));

    QSignalSpy spy(TiptapChannelBridge::instance(),
                   &TiptapChannelBridge::voicePlaybackPositionChanged);
    ASSERT_TRUE(spy.isValid());

    emit h.m_voicePlayerHandler->playPositionChanged(4321);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(1).toLongLong(), 4321);

    // Empty voiceId early-return branch.
    TiptapChannelBridge::instance()->setCurrentVoiceId(QString());
    emit h.m_voicePlayerHandler->playPositionChanged(9999);
    EXPECT_EQ(spy.count(), 0);
}

// L395 lambda: playDurationChanged -> emitVoicePlaybackDurationChanged.
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_PlayDurationChanged)
{
    WebEngineHandler h;
    TiptapChannelBridge::instance()->setCurrentVoiceId(
        QStringLiteral("ut-vid-dur"));

    QSignalSpy spy(TiptapChannelBridge::instance(),
                   &TiptapChannelBridge::voicePlaybackDurationChanged);
    ASSERT_TRUE(spy.isValid());

    emit h.m_voicePlayerHandler->playDurationChanged(8765);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(1).toLongLong(), 8765);

    // Empty voiceId early-return branch.
    TiptapChannelBridge::instance()->setCurrentVoiceId(QString());
    emit h.m_voicePlayerHandler->playDurationChanged(1000);
    EXPECT_EQ(spy.count(), 0);
}

// L401 lambda: voiceFileError -> emitVoiceFileError(voiceId).
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_VoiceFileError)
{
    WebEngineHandler h;
    TiptapChannelBridge::instance()->setCurrentVoiceId(
        QStringLiteral("ut-vid-err"));

    QSignalSpy spy(TiptapChannelBridge::instance(),
                   &TiptapChannelBridge::voiceFileError);
    ASSERT_TRUE(spy.isValid());

    emit h.m_voicePlayerHandler->voiceFileError();
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("ut-vid-err"));

    // Empty voiceId early-return branch.
    TiptapChannelBridge::instance()->setCurrentVoiceId(QString());
    emit h.m_voicePlayerHandler->voiceFileError();
    EXPECT_EQ(spy.count(), 0);
}

// L410 lambda: callJsSetVoiceTextByPath(voiceId, text, asrFlag).
// All three branches (Start / End+empty -> Failed / End+text -> Completed)
// require the Tiptap bridge to be enabled.
TEST(WebEngineHandlerCoverageUT, ConnectWebContent_CallJsSetVoiceTextByPath)
{
    qputenv("DVN_TIPTAP_DEBUG", "1");
    WebEngineHandler h;

    QSignalSpy startSpy(TiptapChannelBridge::instance(),
                        &TiptapChannelBridge::voiceToTextStarted);
    QSignalSpy failSpy(TiptapChannelBridge::instance(),
                       &TiptapChannelBridge::voiceToTextFailed);
    QSignalSpy doneSpy(TiptapChannelBridge::instance(),
                       &TiptapChannelBridge::voiceToTextCompleted);
    ASSERT_TRUE(startSpy.isValid() && failSpy.isValid() && doneSpy.isValid());

    // AsrFlag::Start -> voiceToTextStarted
    emit JsContent::instance()->callJsSetVoiceTextByPath(
        QStringLiteral("vid-start"), QString(), JsContent::AsrFlag::Start);
    EXPECT_EQ(startSpy.count(), 1);
    EXPECT_EQ(startSpy.takeFirst().at(0).toString(), QStringLiteral("vid-start"));

    // AsrFlag::End + empty text -> voiceToTextFailed
    emit JsContent::instance()->callJsSetVoiceTextByPath(
        QStringLiteral("vid-fail"), QString(), JsContent::AsrFlag::End);
    EXPECT_EQ(failSpy.count(), 1);
    EXPECT_EQ(failSpy.takeFirst().at(0).toString(), QStringLiteral("vid-fail"));

    // AsrFlag::End + non-empty text -> voiceToTextCompleted
    emit JsContent::instance()->callJsSetVoiceTextByPath(
        QStringLiteral("vid-done"), QStringLiteral("hello"),
        JsContent::AsrFlag::End);
    EXPECT_EQ(doneSpy.count(), 1);
    EXPECT_EQ(doneSpy.takeFirst().at(0).toString(), QStringLiteral("vid-done"));
}

// ===========================================================================
// 3. processVoiceMenuRequest() single-shot lambda (L780)
// ===========================================================================

// Valid voice metadata but file does not exist -> the L780 singleShot lambda
// fires requestMessageDialog(VoicePathNoAvail). request is never dereferenced
// (early return at L787), so nullptr is safe.
TEST(WebEngineHandlerCoverageUT, ProcessVoiceMenuRequest_MissingFileLambda)
{
    WebEngineHandler h;
    h.menuJson = QVariant(QStringLiteral(
        R"({"type":2,"voiceId":"vid","voicePath":"/tmp/ut_webenginehandler_noexist.wav","voiceSize":1000,"title":"t"})"));

    QSignalSpy spy(&h, &WebEngineHandler::requestMessageDialog);
    ASSERT_TRUE(spy.isValid());

    h.processVoiceMenuRequest(nullptr);
    pumpEvents(120);  // drain the deferred singleShot(0) lambda

    EXPECT_EQ(spy.count(), 1);
}

// ===========================================================================
// 4. processTextMenuRequest (L842)
// ===========================================================================

// All editFlags set so every enableAction branch executes. Request accessors
// are stubbed; the fake pointer is never dereferenced.
TEST(WebEngineHandlerCoverageUT, ProcessTextMenuRequest_AllEditFlags)
{
    Stub stub;
    stub.set(ADDR(QWebEngineContextMenuRequest, editFlags), stub_editFlags_all);
    stub.set(ADDR(QWebEngineContextMenuRequest, position), stub_position_value);

    WebEngineHandler h;
    QObject dummy;
    auto *request = reinterpret_cast<QWebEngineContextMenuRequest *>(&dummy);

    QSignalSpy spy(&h, &WebEngineHandler::requestShowMenu);
    ASSERT_TRUE(spy.isValid());

    h.processTextMenuRequest(request);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(),
              static_cast<int>(WebEngineHandler::TxtMenu));
}

// ===========================================================================
// 5. processPictureMenuRequest (L893)
// ===========================================================================

TEST(WebEngineHandlerCoverageUT, ProcessPictureMenuRequest_AllEditFlags)
{
    Stub stub;
    stub.set(ADDR(QWebEngineContextMenuRequest, editFlags), stub_editFlags_all);
    stub.set(ADDR(QWebEngineContextMenuRequest, position), stub_position_value);

    WebEngineHandler h;
    QObject dummy;
    auto *request = reinterpret_cast<QWebEngineContextMenuRequest *>(&dummy);

    QSignalSpy spy(&h, &WebEngineHandler::requestShowMenu);
    ASSERT_TRUE(spy.isValid());

    h.processPictureMenuRequest(request);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(),
              static_cast<int>(WebEngineHandler::PictureMenu));
}

// ===========================================================================
// 6. saveAsFile (L1011)
// ===========================================================================

// Cancelled dialog (empty result) -> early-return "".
TEST(WebEngineHandlerCoverageUT, SaveAsFile_DialogCancelled)
{
    Stub stub;
    stub.set(ADDR(QFileDialog, getSaveFileName), stub_getSaveFileName_empty);

    WebEngineHandler h;
    QString result = h.saveAsFile(
        QStringLiteral("/tmp/ut_webenginehandler_src.txt"),
        QStringLiteral("/tmp"),
        QStringLiteral("default"));
    EXPECT_TRUE(result.isEmpty());
}

// Dialog returns a real destination; source is a real file -> QFile::copy hits.
TEST(WebEngineHandlerCoverageUT, SaveAsFile_CopySuccess)
{
    const QString srcPath = QStringLiteral("/tmp/ut_webenginehandler_src.txt");
    const QString dstPath = QStringLiteral("/tmp/ut_webenginehandler_dst.txt");

    {
        QFile src(srcPath);
        ASSERT_TRUE(src.open(QIODevice::WriteOnly | QIODevice::Truncate));
        src.write("coverage-payload");
    }
    QFile::remove(dstPath);

    Stub stub;
    stub.set(ADDR(QFileDialog, getSaveFileName), stub_getSaveFileName_dst);

    WebEngineHandler h;
    QString result = h.saveAsFile(srcPath, QStringLiteral("/tmp"),
                                  QStringLiteral("default"));
    EXPECT_EQ(result, dstPath);
    EXPECT_TRUE(QFile::exists(dstPath));

    QFile::remove(srcPath);
    QFile::remove(dstPath);
}
