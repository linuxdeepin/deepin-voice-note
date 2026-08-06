// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for AudioWatcher. The historical ut_audiowatcher.cpp is excluded
// from the build (API mismatch); this targets the present API.
//
// Note: constructing AudioWatcher probes the org.deepin.dde.Audio1 D-Bus
// service; when the service is absent the probe times out once (~25s). The
// constructor itself exercises the init*/default*/updateDeviceEnabled/
// currentAuidoPort/isVirtualMachineHw/vnSystemInfo paths, so we build a single
// shared instance to amortise that cost.

#include "audio_watcher.h"
#include <gtest/gtest.h>
#include <QDBusObjectPath>
#include <QDBusMessage>

namespace {
AudioWatcher *sharedWatcher()
{
    static AudioWatcher *w = nullptr;
    if (!w)
        w = new AudioWatcher();
    return w;
}
}

TEST(AudioWatcherUT, gettersSlotsAndHelpers)
{
    AudioWatcher *aw = sharedWatcher();
    // getters
    aw->getDeviceName(AudioWatcher::Internal);
    aw->getDeviceName(AudioWatcher::Micphone);
    aw->getVolume(AudioWatcher::Internal);
    aw->getVolume(AudioWatcher::Micphone);
    aw->getMute(AudioWatcher::Internal);
    aw->getMute(AudioWatcher::Micphone);
    aw->getDeviceEnable(AudioWatcher::Internal);
    aw->getDeviceEnable(AudioWatcher::Micphone);
    aw->hasAudioOutputDevice();
    aw->hasAudioInputDevice();

    // device-change slots (no further heavy D-Bus when service absent)
    AudioPort port;
    port.name = "p1";
    port.availability = 2;
    aw->onSourceVolumeChanged(0.5);
    aw->onSinkVolumeChanged(0.5);
    aw->onDefaultSourceActivePortChanged(port);
    aw->onDefaultSinkActivePortChanged(port);
    aw->onSourceMuteChanged(true);
    aw->onSinkMuteChanged(false);
    aw->onDefaultSourceChanaged(QDBusObjectPath("/test/src"));
    aw->onDefaultSinkChanaged(QDBusObjectPath("/test/snk"));
    aw->onDBusAudioPropertyChanged(QDBusMessage());   // empty -> early return

    // ports / device-update helpers
    aw->currentAuidoPort({}, AudioWatcher::Internal);
    aw->currentAuidoPort({}, AudioWatcher::Micphone);
    aw->updateDeviceEnabled("[]", true);
    aw->updateDeviceEnabled("", false);
    QString cards = R"([{"Ports":[
        {"Name":"out","Description":"outdesc","Enabled":true,"Direction":1},
        {"Name":"in","Description":"indesc","Enabled":true,"Direction":2}
    ]}])";
    aw->updateDeviceEnabled(cards, true);

    // re-init helpers (already covered by ctor, re-exercised for safety)
    aw->initWatcherCofing();
    aw->initDefaultSourceDBusInterface();
    aw->initDefaultSinkDBusInterface();
    SUCCEED();
}
