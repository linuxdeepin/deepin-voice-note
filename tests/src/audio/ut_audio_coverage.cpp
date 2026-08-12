// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Coverage-focused tests for previously uncovered functions in the audio
// module: AudioWatcher::defaultSourcePorts, AudioPort comparison operators,
// the free functions bufferProbe / GstBusMessageCb, GstreamRecorder::createPipe
// / startRecord, and RecordingCurves destructor / updateCurves.

#include "audio_watcher.h"
#include "gstreamrecorder.h"
#include "recording_curves.h"

#include "stub.h"

#include <gtest/gtest.h>

#include <gst/gst.h>

// Free functions defined in gstreamrecorder.cpp are not declared in the header
// (external linkage); forward declare so the test can invoke them directly.
GstPadProbeReturn bufferProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
gboolean GstBusMessageCb(GstBus *bus, GstMessage *msg, void *userdata);

namespace {
// The AudioWatcher constructor probes the org.deepin.dde.Audio1 D-Bus service
// (one ~25s timeout when the service is absent); share a single instance across
// these tests to amortise that cost.
AudioWatcher *sharedAudioWatcher()
{
    static AudioWatcher *w = nullptr;
    if (!w)
        w = new AudioWatcher();
    return w;
}

// Stub for gst_element_factory_make -> always returns nullptr so createPipe()
// exits early via the "audioSrc == nullptr" guard without touching hardware.
GstElement *stub_gst_element_factory_make_null(const gchar *, const gchar *)
{
    return nullptr;
}
} // namespace

// --- AudioWatcher::defaultSourcePorts (private; reached via -fno-access-control) ---

TEST(AudioCoverageUT, AudioWatcher_defaultSourcePorts_DBusUnavailable_ReturnsEmpty)
{
    AudioWatcher *aw = sharedAudioWatcher();
    QList<AudioPort> ports = aw->defaultSourcePorts();
    // Without the real org.deepin.dde.Audio1 service the QDBusInterface is
    // invalid, so the branch that parses ports is skipped and the list is empty.
    EXPECT_TRUE(ports.isEmpty());
    SUCCEED();
}

// --- AudioPort::operator== ---

TEST(AudioCoverageUT, AudioPort_operatorEqual_SameFields_ReturnsTrue)
{
    AudioPort a;
    a.name = "speaker";
    a.description = "Built-in Speaker";
    a.availability = 2;

    AudioPort b;
    b.name = "speaker";
    b.description = "Built-in Speaker";
    b.availability = 2;

    EXPECT_TRUE(a == b);
}

TEST(AudioCoverageUT, AudioPort_operatorEqual_DifferentName_ReturnsFalse)
{
    AudioPort a;
    a.name = "speaker";
    a.description = "desc";
    a.availability = 2;

    AudioPort b;
    b.name = "mic";
    b.description = "desc";
    b.availability = 2;

    EXPECT_FALSE(a == b);
}

TEST(AudioCoverageUT, AudioPort_operatorEqual_DifferentAvailability_ReturnsFalse)
{
    AudioPort a;
    a.name = "x";
    a.description = "d";
    a.availability = 1;

    AudioPort b;
    b.name = "x";
    b.description = "d";
    b.availability = 2;

    EXPECT_FALSE(a == b);
}

// --- AudioPort::operator!= ---

TEST(AudioCoverageUT, AudioPort_operatorNotEqual_SameFields_ReturnsFalse)
{
    AudioPort a;
    a.name = "headset";
    a.description = "Headset";
    a.availability = 0;

    AudioPort b;
    b.name = "headset";
    b.description = "Headset";
    b.availability = 0;

    EXPECT_FALSE(a != b);
}

TEST(AudioCoverageUT, AudioPort_operatorNotEqual_DifferentDescription_ReturnsTrue)
{
    AudioPort a;
    a.name = "headset";
    a.description = "Headset A";
    a.availability = 0;

    AudioPort b;
    b.name = "headset";
    b.description = "Headset B";
    b.availability = 0;

    EXPECT_TRUE(a != b);
}

TEST(AudioCoverageUT, AudioPort_operatorNotEqual_AllDifferent_ReturnsTrue)
{
    AudioPort a;
    a.name = "a";
    a.description = "da";
    a.availability = 1;

    AudioPort b;
    b.name = "b";
    b.description = "db";
    b.availability = 2;

    EXPECT_TRUE(a != b);
}

// --- bufferProbe (free function) ---

TEST(AudioCoverageUT, bufferProbe_NullBufferInfo_ReturnsOk)
{
    GstreamRecorder recorder;
    // Zero-initialised info: type has no BUFFER flag, so
    // gst_pad_probe_info_get_buffer returns NULL -> bufferProbe returns OK
    // without invoking doBufferProbe.
    GstPadProbeInfo info;
    memset(&info, 0, sizeof(info));

    GstPadProbeReturn ret = bufferProbe(nullptr, &info, &recorder);
    EXPECT_EQ(ret, GST_PAD_PROBE_OK);
}

TEST(AudioCoverageUT, bufferProbe_RealBuffer_CallsDoBufferProbeAndReturnsOk)
{
    GstreamRecorder recorder;
    recorder.initFormat(); // give m_format a valid value for QAudioBuffer

    GstBuffer *buffer = gst_buffer_new();
    ASSERT_NE(buffer, nullptr);

    GstPadProbeInfo info;
    memset(&info, 0, sizeof(info));
    info.type = GST_PAD_PROBE_TYPE_BUFFER; // make get_buffer return info.data
    info.data = buffer;

    GstPadProbeReturn ret = bufferProbe(nullptr, &info, &recorder);
    EXPECT_EQ(ret, GST_PAD_PROBE_OK);

    gst_buffer_unref(buffer);
}

// --- GstBusMessageCb (free function) ---

TEST(AudioCoverageUT, GstBusMessageCb_NullMessage_ReturnsTrue)
{
    GstreamRecorder recorder;
    // doBusMessage(nullptr) takes the early-return path and returns true.
    gboolean ret = GstBusMessageCb(nullptr, nullptr, &recorder);
    EXPECT_TRUE(ret);
}

// --- GstreamRecorder::createPipe (private; reached via -fno-access-control) ---

TEST(AudioCoverageUT, GstreamRecorder_createPipe_FactoryStubbed_ReturnsFalse)
{
    GstreamRecorder recorder;
    recorder.setOutputFile("/tmp/voice-note-ut-createpipe.mp3");

    Stub stub;
    stub.set(gst_element_factory_make, stub_gst_element_factory_make_null);

    // With gst_element_factory_make stubbed to return nullptr, the first
    // element creation fails and createPipe() returns false without touching
    // audio hardware.
    bool ok = recorder.createPipe();
    EXPECT_FALSE(ok);
    // Pipeline must remain null after the failure so cleanup is safe.
    EXPECT_EQ(recorder.m_pipeline, nullptr);
}

// --- GstreamRecorder::startRecord ---

TEST(AudioCoverageUT, GstreamRecorder_startRecord_PipelineCreationFails_ReturnsFalse)
{
    GstreamRecorder recorder;
    recorder.setOutputFile("/tmp/voice-note-ut-startrecord.mp3");

    Stub stub;
    stub.set(gst_element_factory_make, stub_gst_element_factory_make_null);

    // startRecord() attempts createPipe(); when it fails (factory stubbed),
    // it returns false before touching the pipeline state.
    bool ok = recorder.startRecord();
    EXPECT_FALSE(ok);
}

// --- RecordingCurves::~RecordingCurves (deleting destructor D0) ---

TEST(AudioCoverageUT, RecordingCurves_destructor_HeapAlloc_DeleteDoesNotCrash)
{
    RecordingCurves *curves = new RecordingCurves();
    ASSERT_NE(curves, nullptr);
    // Heap allocation + delete invokes the D0 deleting destructor.
    delete curves;
    SUCCEED();
}

// --- RecordingCurves::updateCurves (private slot; via -fno-access-control) ---

TEST(AudioCoverageUT, RecordingCurves_updateCurves_IncrementsPhase_NoCrash)
{
    RecordingCurves curves;
    // updateCurves() is a private slot; -fno-access-control allows direct call.
    // Capture phase before via the private member to validate side effect.
    double phaseBefore = curves.m_phase;
    curves.updateCurves();
    // updateCurves advances m_phase by M_PI each call.
    EXPECT_NE(curves.m_phase, phaseBefore);
}

TEST(AudioCoverageUT, RecordingCurves_updateCurves_ManyCalls_PhaseResetsWithinBounds)
{
    RecordingCurves curves;
    // Exercise the wrap-around branch (m_phase > 170*M_PI -> reset to 0).
    for (int i = 0; i < 200; ++i) {
        curves.updateCurves();
    }
    // After many iterations phase must stay bounded by the reset guard.
    EXPECT_LE(curves.m_phase, 170 * M_PI);
    SUCCEED();
}
