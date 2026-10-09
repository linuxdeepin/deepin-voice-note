// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VoiceToTextHandler.

#include "voice_to_text_handler.h"
#include "vnoteitem.h"
#include "vnotea2tmanager.h"
#include "uosaicapabilitymanager.h"
#include "tiptapchannelbridge.h"

#include <gtest/gtest.h>
#include <QSharedPointer>
#include <QEventLoop>
#include <QTimer>
#include <stub.h>

static bool stub_net_false() { return false; }
static bool stub_net_true() { return true; }
static bool stub_tiptap_true() { return true; }
static bool stub_tiptap_false() { return false; }
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

    // success, same note, Tiptap disabled (Summernote path)
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, tiptapEnabled), stub_tiptap_false);
        h.onA2TSuccess("hello");
    }
    // success, same note, Tiptap enabled (Tiptap path)
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, tiptapEnabled), stub_tiptap_true);
        h.onA2TSuccess("hello");
    }
    // success, switched note (originalNoteId != currentNoteId), Tiptap enabled
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, tiptapEnabled), stub_tiptap_true);
        h.m_originalNoteId = 999;
        h.onA2TSuccess("hello");
    }
    // success, switched note, Tiptap disabled
    {
        Stub s;
        s.set(ADDR(TiptapChannelBridge, tiptapEnabled), stub_tiptap_false);
        h.m_originalNoteId = 999;
        h.onA2TSuccess("hello");
    }
    SUCCEED();
}

// BUG 337823: 网络正常情况下语音转文字误报"网络状态差"（14e92850 改用
// NetworkManager D-Bus 探测）。回归：checkNetworkState 通过时 setAudioToText
// 必须继续发起转写，而不是被误判拦截。
// PMS: https://pms.uniontech.com/bug-view-337823.html  commit: 14e92850
static bool g_bug337823_asrStarted = false;
static void stub_startAsr_capture(const QString &, qint64) { g_bug337823_asrStarted = true; }
static bool stub_cap_ok(UosAiCapabilityManager::Status) { return false; }

TEST(VoiceToTextHandlerUT, BUG337823_goodNetworkProceedsToAsr)
{
    VoiceToTextHandler h;
    g_bug337823_asrStarted = false;
    Stub stub;
    stub.set(ADDR(VoiceToTextHandler, checkNetworkState), stub_net_true);
    stub.set(ADDR(VNoteA2TManager, startAsr), stub_startAsr_capture);
    // 单元环境无 UOS AI：屏蔽能力检查，聚焦网络误判回归点
    stub.set(ADDR(UosAiCapabilityManager, isUpdateRequired), stub_cap_ok);
    stub.set(ADDR(UosAiCapabilityManager, checkCapability), stub_cap_ok);

    QSharedPointer<VNVoiceBlock> blk(new VNVoiceBlock);
    blk->voicePath = QStringLiteral("/tmp/ut-337823.wav");
    blk->voiceSize = 5000;   // 在长度限制内
    h.setAudioToText(blk);

    // onA2TStart 经 QTimer::singleShot(0) 延迟发起转写，泵一下事件
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(g_bug337823_asrStarted);   // 网络正常 → 不被拦截，转写已发起
}
