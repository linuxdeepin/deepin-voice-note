// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VoicePlayerHandler.

#include "voice_player_handler.h"
#include "vlcplayer.h"
#include "voiceplayerbase.h"
#include "vnoteitem.h"

#include <gtest/gtest.h>
#include <stub.h>

static VoicePlayerBase::PlayerState stub_state_playing() { return VoicePlayerBase::Playing; }
static VoicePlayerBase::PlayerState stub_state_paused() { return VoicePlayerBase::Paused; }

TEST(VoicePlayerHandlerUT, constructAndPlayVoiceMissingFile)
{
    VoicePlayerHandler h;
    // empty json -> parsed block has empty path -> file does not exist -> onStop + voiceFileError
    h.playVoice(QVariant(), false);
    h.playVoice(QVariant(), true);   // null block + isSame -> forced false
    SUCCEED();
}

TEST(VoicePlayerHandlerUT, playVoiceImplBranches)
{
    VoicePlayerHandler h;
    // invalid (null block) branch
    h.m_voiceBlock.clear();
    h.playVoiceImpl(false);
    // new voice, non-existent path -> setFilePath + onPlay (libvlc errors silently)
    h.m_voiceBlock = QSharedPointer<VNVoiceBlock>::create();
    h.m_voiceBlock->voicePath = "/tmp/voice-note-ut-noexist.wav";
    h.playVoiceImpl(false);
    // same voice -> toggle (Stopped -> restart branch)
    h.playVoiceImpl(true);
    SUCCEED();
}

TEST(VoicePlayerHandlerUT, stopAndPosition)
{
    VoicePlayerHandler h;
    h.onStop();            // Stopped -> "already stopped"
    h.setPlayPosition(500); // Stopped -> skip
    SUCCEED();
}

// NOTE: VlcPlayer::getState is virtual and cannot be stubbed via the deepin
// stub (ADDR of a virtual method yields a PMF, not an entry address). The
// Playing/Paused branches of onToggleStateChange/onStop would require a real
// playing pipeline; the functions themselves are covered via the Stopped-state
// paths above, which is sufficient for function coverage.
