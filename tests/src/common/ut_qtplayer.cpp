// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for QtPlayer (Qt6 QMediaPlayer backend).

#include "qtplayer.h"
#include <gtest/gtest.h>

TEST(QtPlayerUT, lifecycleAndControls)
{
    QtPlayer player(nullptr);
    player.setFilePath("/tmp/voice-note-ut-nonexistent.wav");
    player.setPosition(100);
    player.play();
    player.pause();
    player.stop();
    // initial state is Stopped
    EXPECT_EQ(VoicePlayerBase::Stopped, player.getState());
    SUCCEED();
}

TEST(QtPlayerUT, getStatePlayingLike)
{
    QtPlayer player(nullptr);
    player.setFilePath("/tmp/voice-note-ut-nonexistent.wav");
    player.play();
    player.getState();
    player.pause();
    player.getState();
    SUCCEED();
}
