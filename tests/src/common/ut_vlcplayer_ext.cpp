// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VlcPlayer (runtime-loaded libvlc backend).
// The historical ut_vlcplayer.cpp is excluded from the build (API mismatch);
// this file targets the present VlcPlayer API.

#include "vlcplayer.h"
#include <gtest/gtest.h>

TEST(VlcPlayerUT, uninitializedBranches)
{
    // Before setFilePath/init, m_vlcPlayer is null -> "not initialized" paths.
    VlcPlayer v(nullptr);
    EXPECT_EQ(VoicePlayerBase::None, v.getState());
    v.setPosition(0);
    v.play();
    v.pause();
    v.stop();
    v.setChangePlayFile(true);
    SUCCEED();
}

TEST(VlcPlayerUT, initAndPlaybackPath)
{
    // Loads libvlc.so.5 and resolves function pointers.
    VlcPlayer v(nullptr);
    ASSERT_TRUE(v.initVlcPlayer());
    v.setFilePath("/tmp/voice-note-ut-nonexistent.wav");
    v.play();
    v.pause();
    v.stop();
    v.setPosition(500);
    v.getState();
    SUCCEED();
}
