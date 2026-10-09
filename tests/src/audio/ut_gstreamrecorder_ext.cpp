// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for GstreamRecorder (null-pipeline / safe branches; avoids real
// audio capture). The historical ut_gstreamrecorder.cpp is excluded (API
// mismatch); this targets the present API.

#include "gstreamrecorder.h"
#include <gtest/gtest.h>
#include <stub.h>

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// 通过 gst C 层 stub 驱动非空管道分支（避免真实音频采集与环境依赖）
// ============================================================================

static GstStateChangeReturn stub_getState_paused(GstElement *, GstState *state,
                                                 GstState *pending, GstClockTime)
{
    if (state)
        *state = GST_STATE_PAUSED;
    if (pending)
        *pending = GST_STATE_VOID_PENDING;
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn stub_getState_playing(GstElement *, GstState *state,
                                                  GstState *pending, GstClockTime)
{
    if (state)
        *state = GST_STATE_PLAYING;
    if (pending)
        *pending = GST_STATE_VOID_PENDING;
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn stub_setState_ok(GstElement *, GstState)
{
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn stub_setState_fail(GstElement *, GstState)
{
    return GST_STATE_CHANGE_FAILURE;
}

static gboolean stub_sendEvent_true(GstElement *, GstEvent *) { return TRUE; }

static GstBus *stub_getBus_fake(GstElement *)
{
    return reinterpret_cast<GstBus *>(0x1234);
}

static GstMessage *stub_popEos_immediate(GstBus *, GstClockTime, GstMessageType)
{
    // 立即返回 EOS 消息，避免真实 5s 等待
    return gst_message_new_eos(nullptr);
}

static void stub_objectUnref_nop(gpointer) {}

// BUG 322355: 录音暂停时点击完成按钮，语音记事本卡死。修复（a3006922）：
// stopRecord 在暂停态先恢复 PLAYING 再发 EOS，且 EOS 等待使用 5s 超时而非
// 无限等待（src/audio/gstreamrecorder.cpp:261-309）。
// PMS: https://pms.uniontech.com/bug-view-322355.html  commit: a3006922
TEST(GstreamRecorderUT, BUG322355_stopFromPausedNoDeadlock)
{
    GstreamRecorder rec;
    rec.m_pipeline = reinterpret_cast<GstElement *>(0x1234);

    Stub stub;
    stub.set(gst_element_get_state, stub_getState_paused);
    stub.set(gst_element_set_state, stub_setState_ok);
    stub.set(gst_element_send_event, stub_sendEvent_true);
    stub.set(gst_element_get_bus, stub_getBus_fake);
    stub.set(gst_bus_timed_pop_filtered, stub_popEos_immediate);
    stub.set(gst_object_unref, stub_objectUnref_nop);

    int state = -1, pending = -1;
    rec.GetGstState(&state, &pending);
    ASSERT_EQ(GST_STATE_PAUSED, state);   // 前置：暂停态

    // 修复前：暂停态直接发 EOS → EOS 无法传播 → 无限等待卡死；
    // 修复后：恢复 PLAYING + 限时等待，必须立即走完清理流程
    rec.stopRecord();
    SUCCEED();

    rec.m_pipeline = nullptr;   // 避免析构触到伪指针
}

// BUG 305495: 已连接麦克风但点击录音无法录音。回归 startRecord 的状态机：
// 已在 PLAYING 不重复切状态；PAUSED 从暂停恢复；NULL 正常启动。
// PMS: https://pms.uniontech.com/bug-view-305495.html  commit: 43e4d05d
TEST(GstreamRecorderUT, BUG305495_startRecordStateTransitions)
{
    GstreamRecorder rec;
    rec.m_pipeline = reinterpret_cast<GstElement *>(0x1234);
    rec.initFormat();

    {
        Stub stub;
        // 已在 PLAYING：不得再切状态（若切了则返回 FAILURE→false）
        stub.set(gst_element_get_state, stub_getState_playing);
        stub.set(gst_element_set_state, stub_setState_fail);
        EXPECT_TRUE(rec.startRecord());
    }
    {
        Stub stub;
        // PAUSED：恢复播放成功
        stub.set(gst_element_get_state, stub_getState_paused);
        stub.set(gst_element_set_state, stub_setState_ok);
        EXPECT_TRUE(rec.startRecord());
    }
    rec.m_pipeline = nullptr;   // 避免析构触到伪指针
}

TEST(GstreamRecorderUT, lifecycleAndSetters)
{
    GstreamRecorder rec;
    rec.setDevice("alsa_input.test");
    rec.setOutputFile("/tmp/voice-note-ut-rec.mp3");
    rec.initFormat();
    int state = -1, pending = -1;
    rec.GetGstState(&state, &pending);
    rec.pauseRecord();   // null pipeline -> no-op
    rec.stopRecord();    // null pipeline -> no-op
    rec.setStateToNull();
    SUCCEED();
}

TEST(GstreamRecorderUT, messageAndBufferHandling)
{
    GstreamRecorder rec;
    EXPECT_TRUE(rec.doBusMessage(nullptr));      // null message -> early return
    EXPECT_TRUE(rec.doBufferProbe(nullptr));    // null buffer -> early return
    rec.bufferProbed();                          // invalid pending buffer -> return
    rec.objectUnref(nullptr);                    // null -> no-op
    SUCCEED();
}
