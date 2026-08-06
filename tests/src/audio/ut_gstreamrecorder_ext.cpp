// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for GstreamRecorder (null-pipeline / safe branches; avoids real
// audio capture). The historical ut_gstreamrecorder.cpp is excluded (API
// mismatch); this targets the present API.

#include "gstreamrecorder.h"
#include <gtest/gtest.h>

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
