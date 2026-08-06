// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VoiceRecoderHandler. Stubs GstreamRecorder::startRecord to
// avoid real audio capture.

#include "voice_recoder_handler.h"
#include "gstreamrecorder.h"
#include <gtest/gtest.h>
#include <stub.h>

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
