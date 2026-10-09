// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VoiceRecoderHandler. Stubs GstreamRecorder::startRecord to
// avoid real audio capture.

#include "voice_recoder_handler.h"
#include "gstreamrecorder.h"
#include "opsstateinterface.h"
#include "audio_watcher.h"
#include <gtest/gtest.h>
#include <stub.h>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>

static bool stub_startRecord_false() { return false; }

TEST(VoiceRecoderHandlerUT, instanceAndState)
{
    auto *r = VoiceRecoderHandler::instance();
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(VoiceRecoderHandler::Idle, r->getRecoderType());
    r->setAudioDevice("alsa_input.test");
    r->hasAudioOutputDevice();
    r->hasAudioInputDevice();
    SUCCEED();
}

TEST(VoiceRecoderHandlerUT, startStopPause)
{
    auto *r = VoiceRecoderHandler::instance();
    Stub stub;
    stub.set(ADDR(GstreamRecorder, startRecord), stub_startRecord_false);

    // start -> volume check / confirm path (startRecord stubbed false -> Idle)
    r->startRecoder();
    r->confirmStartRecoder();

    // simulate an active recording, then stop -> finished branch
    r->m_type = VoiceRecoderHandler::Recording;
    r->stopRecoder();

    // pause then resume
    r->m_type = VoiceRecoderHandler::Recording;
    r->pauseRecoder();        // -> Paused
    r->pauseRecoder();        // -> resume (startRecord stubbed false)
    SUCCEED();
}

TEST(VoiceRecoderHandlerUT, deviceAndMode)
{
    auto *r = VoiceRecoderHandler::instance();
    r->m_type = VoiceRecoderHandler::Idle;
    r->m_currentMode = 1;
    r->changeMode(1);            // onAudioDeviceChange same mode
    r->changeMode(2);            // onAudioDeviceChange different mode
    r->onDeviceEnableChanged(1, true);
    r->onDeviceEnableChanged(2, true);
    r->onReduceNoiseChanged(true);
    r->checkVolume();
    r->tryGetMicNameFromPactl();
    r->getDefaultMicDeviceName();
    SUCCEED();
}

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================

static bool stub_audioWatcher_enable_false(AudioWatcher::AudioMode) { return false; }
static bool stub_audioWatcher_enable_true(AudioWatcher::AudioMode) { return true; }
static QString stub_audioWatcher_name_valid(AudioWatcher::AudioMode) { return "plughw:CARD=ut"; }
static QString stub_audioWatcher_name_empty(AudioWatcher::AudioMode) { return QString(); }

static void pumpEvents(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

// BUG 335263: 录音过程中记事本/笔记操作未置灰。修复（e3c51a74）：
// startRecoder 对播放中/转写中/无可用设备等异常分支补齐拦截与置灰播报
// （updateRecordBtnState(false)+recoderStateChange(Idle)）。
// PMS: https://pms.uniontech.com/bug-view-335263.html  commit: e3c51a74, a9eeef4c
TEST(VoiceRecoderHandlerUT, BUG335263_startRecoderGuardsKeepUiConsistent)
{
    auto *r = VoiceRecoderHandler::instance();
    QSignalSpy btnSpy(r, &VoiceRecoderHandler::updateRecordBtnState);
    QSignalSpy stateSpy(r, &VoiceRecoderHandler::recoderStateChange);
    Stub stub;
    stub.set(ADDR(GstreamRecorder, startRecord), stub_startRecord_false);

    // 已在录音 → 拒绝重复启动
    r->m_type = VoiceRecoderHandler::Recording;
    r->startRecoder();
    EXPECT_EQ(VoiceRecoderHandler::Recording, r->getRecoderType());
    r->m_type = VoiceRecoderHandler::Idle;

    // 播放中 → 录音/播放互斥，拒绝启动
    OpsStateInterface::instance()->operState(OpsStateInterface::StatePlaying, true);
    r->startRecoder();
    EXPECT_EQ(VoiceRecoderHandler::Idle, r->getRecoderType());
    OpsStateInterface::instance()->operState(OpsStateInterface::StatePlaying, false);

    // 无可用输入设备 → 置灰按钮并广播 Idle
    Stub stub2;
    stub2.set(ADDR(AudioWatcher, getDeviceEnable), stub_audioWatcher_enable_false);
    stub2.set(ADDR(AudioWatcher, getDeviceName), stub_audioWatcher_name_empty);
    r->startRecoder();
    EXPECT_EQ(1, btnSpy.count());
    EXPECT_FALSE(btnSpy.first().at(0).toBool());
    EXPECT_EQ(1, stateSpy.count());
    EXPECT_EQ(VoiceRecoderHandler::Idle, r->getRecoderType());
}

// BUG 376597: OPS 大屏插入耳机后软件有概率识别不到录音设备（提示“未检测到
// 录音设备”）。修复（0c04d11c）：设备使能缓存未刷新时，只要设备名可解析出
// 有效值，isRecordDeviceEnabled 不得提前判不可用（回退判定）。
// PMS: https://pms.uniontech.com/bug-view-376597.html  commit: 0c04d11c
TEST(VoiceRecoderHandlerUT, BUG376597_recognizeDeviceByNameFallback)
{
    auto *r = VoiceRecoderHandler::instance();
    ASSERT_NE(nullptr, r);

    {
        Stub stub;
        stub.set(ADDR(AudioWatcher, getDeviceEnable), stub_audioWatcher_enable_false);
        stub.set(ADDR(AudioWatcher, getDeviceName), stub_audioWatcher_name_valid);
        // 使能缓存未刷新，但设备名有效 → 仍可识别，不应误报无设备
        EXPECT_TRUE(r->isRecordDeviceEnabled());
    }
    {
        Stub stub;
        stub.set(ADDR(AudioWatcher, getDeviceEnable), stub_audioWatcher_enable_false);
        stub.set(ADDR(AudioWatcher, getDeviceName), stub_audioWatcher_name_empty);
        // 设备名也解析不出 → 维持不可用判定
        EXPECT_FALSE(r->isRecordDeviceEnabled());
    }
    {
        Stub stub;
        stub.set(ADDR(AudioWatcher, getDeviceEnable), stub_audioWatcher_enable_true);
        EXPECT_TRUE(r->isRecordDeviceEnabled());
    }
}

// BUG 309299: 控制中心开启/关闭噪音抑制开关后录音无效果。修复（9b3dad02）：
// onReduceNoiseChanged 立即停止当前录音管线下次按新模式启动，并延迟刷新
// 录音按钮状态。
// PMS: https://pms.uniontech.com/bug-view-309299.html  commit: 9b3dad02
TEST(VoiceRecoderHandlerUT, BUG309299_noiseToggleStopsActiveRecoder)
{
    auto *r = VoiceRecoderHandler::instance();
    QSignalSpy stateSpy(r, &VoiceRecoderHandler::recoderStateChange);
    QSignalSpy waveSpy(r, &VoiceRecoderHandler::updateWave);
    QSignalSpy btnSpy(r, &VoiceRecoderHandler::updateRecordBtnState);
    ASSERT_TRUE(stateSpy.isValid() && waveSpy.isValid() && btnSpy.isValid());

    Stub stub;
    stub.set(ADDR(AudioWatcher, getDeviceEnable), stub_audioWatcher_enable_true);

    // 录音中切换噪音抑制：立即停止并广播 Idle，波形归零
    r->m_type = VoiceRecoderHandler::Recording;
    r->onReduceNoiseChanged(true);
    EXPECT_EQ(VoiceRecoderHandler::Idle, r->getRecoderType());
    EXPECT_GE(stateSpy.count(), 1);
    EXPECT_GE(waveSpy.count(), 1);

    // 200ms 后刷新录音按钮状态
    pumpEvents(400);
    EXPECT_GE(btnSpy.count(), 1);
    EXPECT_TRUE(btnSpy.last().at(0).toBool());
}
