// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VoiceToTextHandler.

#include "voice_to_text_handler.h"
#include "vnoteitem.h"
#include "vnotea2tmanager.h"
#include "tiptapchannelbridge.h"

#include <gtest/gtest.h>
#include <QSharedPointer>
#include <stub.h>

static bool stub_net_false() { return false; }
static bool stub_net_true() { return true; }
static bool stub_debug_true() { return true; }
static bool stub_debug_false() { return false; }
static void stub_startAsr(const QString &, qint64) { /* no-op: avoid real ASR */ }

TEST(VoiceToTextHandlerUT, setAudio_nullBlock)
{
    VoiceToTextHandler h;
    h.setAudioToText(QSharedPointer<VNVoiceBlock>(nullptr));
    SUCCEED();
}

TEST(VoiceToTextHandlerUT, setAudio_noNetwork)
{
    VoiceToTextHandler h;
    Stub stub;
    stub.set(ADDR(VoiceToTextHandler, checkNetworkState), stub_net_false);
    QSharedPointer<VNVoiceBlock> blk(new VNVoiceBlock);
    h.setAudioToText(blk);
    SUCCEED();
}

TEST(VoiceToTextHandlerUT, setAudio_lengthLimit)
{
    VoiceToTextHandler h;
    Stub stub;
    stub.set(ADDR(VoiceToTextHandler, checkNetworkState), stub_net_true);
    QSharedPointer<VNVoiceBlock> blk(new VNVoiceBlock);
    blk->voiceSize = 99999999;   // exceeds MAX_A2T_AUDIO_LEN_MS
    h.setAudioToText(blk);
    SUCCEED();
}

TEST(VoiceToTextHandlerUT, checkNetworkStateDirect)
{
    VoiceToTextHandler h;
    h.checkNetworkState();   // exercises NetworkManager D-Bus probe
    SUCCEED();
}

TEST(VoiceToTextHandlerUT, a2tCallbacks)
{
    VoiceToTextHandler h;
    Stub stub;
    stub.set(ADDR(VNoteA2TManager, startAsr), stub_startAsr);

    QSharedPointer<VNVoiceBlock> blk(new VNVoiceBlock);
    blk->voicePath = "/tmp/x.wav";
    blk->voiceSize = 1000;
    h.m_voiceBlock = blk;
    h.m_originalVoiceId = "v1";
    h.m_originalNoteId = -1;   // == currentNoteId default -> same-note branch

    h.onA2TStart();
    h.onA2TError(1);

    // success, same note, debug false (Summernote path)
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, debugEnabled), stub_debug_false);
        h.onA2TSuccess("hello");
    }
    // success, same note, debug true (Tiptap path)
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, debugEnabled), stub_debug_true);
        h.onA2TSuccess("hello");
    }
    // success, switched note (originalNoteId != currentNoteId), debug true
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, debugEnabled), stub_debug_true);
        h.m_originalNoteId = 999;
        h.onA2TSuccess("hello");
    }
    // success, switched note, debug false
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, debugEnabled), stub_debug_false);
        h.m_originalNoteId = 999;
        h.onA2TSuccess("hello");
    }
    SUCCEED();
}
