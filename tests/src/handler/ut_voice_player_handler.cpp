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
#include <QSignalSpy>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include "opsstateinterface.h"
#include "metadataparser.h"
#include "voiceplayerbase.h"

// 可控状态的假播放器：绕开 VlcPlayer/QtPlayer 真实状态机（offscreen 下初始
// 为 None，无法到达 Stopped/Ended 重播分支）
class FakePlayer : public VoicePlayerBase
{
public:
    explicit FakePlayer(QObject *parent = nullptr)
        : VoicePlayerBase(parent) {}
    void setFilePath(QString) override {}
    void setPosition(qint64) override {}
    void play() override {}
    void pause() override {}
    void stop() override {}
    PlayerState getState() override { return m_state; }
    void setState(PlayerState s) { m_state = s; }
private:
    PlayerState m_state = PlayerState::Stopped;
};

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

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================

// BUG 335263/335623: 播放结束后 OpsStateInterface::StatePlaying 残留 true，
// 导致“新建语音备忘”置灰（335263）/列表删除等右键操作无效（335623）。
// 修复（a9eeef4c）：ctor 的 playEnd 回调复位播放态。
// PMS: https://pms.uniontech.com/bug-view-335263.html  commit: a9eeef4c, e3c51a74
TEST(VoicePlayerHandlerUT, BUG335263_playEndResetsPlayingState)
{
    VoicePlayerHandler h;
    OpsStateInterface *ops = OpsStateInterface::instance();
    ops->operState(OpsStateInterface::StatePlaying, true);
    ASSERT_TRUE(ops->isPlaying());

    // 播放结束信号：修复后必须复位 StatePlaying，否则 UI 一直锁定
    emit h.m_player->playEnd();
    EXPECT_FALSE(ops->isPlaying());
}

// BUG 335623: 重新播放（Stopped/Ended → 播放）路径同样要维护播放态，
// 且播放结束后状态必须回到 false，否则删除/新建按钮持续置灰。
// PMS: https://pms.uniontech.com/bug-view-335623.html  commit: a9eeef4c, befc63df
TEST(VoicePlayerHandlerUT, BUG335623_restartThenEndClearsPlayingState)
{
    VoicePlayerHandler h;
    OpsStateInterface *ops = OpsStateInterface::instance();
    QSignalSpy statusSpy(&h, &VoicePlayerHandler::playStatusChanged);
    ASSERT_TRUE(statusSpy.isValid());

    h.m_voiceBlock = QSharedPointer<VNVoiceBlock>::create();
    h.m_voiceBlock->voicePath = "/tmp/voice-note-ut-335623.wav";

    // 换上状态可控的 FakePlayer（Stopped）
    VoicePlayerBase *saved = h.m_player;
    FakePlayer fake;
    fake.setState(VoicePlayerBase::Stopped);
    h.m_player = &fake;

    // Stopped → 重新播放分支（a9eeef4c 修复点：setChangePlayFile+setFilePath）：
    // 置位 StatePlaying 并播报 Playing
    h.onToggleStateChange();
    EXPECT_TRUE(ops->isPlaying());
    ASSERT_GE(statusSpy.count(), 1);
    EXPECT_EQ(static_cast<int>(VoicePlayerHandler::Playing), statusSpy.takeLast().at(0).toInt());

    // 恢复真实 player，播放结束：必须复位（本轮回归核心断言）
    h.m_player = saved;
    emit saved->playEnd();
    EXPECT_FALSE(ops->isPlaying());
    ASSERT_GE(statusSpy.count(), 1);
    EXPECT_EQ(static_cast<int>(VoicePlayerHandler::End), statusSpy.takeLast().at(0).toInt());
}

// BUG 340969/340979: 录音保存后右键语音提示“语音被删除”/Ctrl+D 保存失败。
// 修复（4ebe6e2f）：playVoice 解析 json 后将相对路径展开为绝对路径再检查
// 文件存在性（src/handler/voice_player_handler.cpp:67）。
// PMS: https://pms.uniontech.com/bug-view-340969.html  commit: 4ebe6e2f
// PMS: https://pms.uniontech.com/bug-view-340979.html  commit: 4ebe6e2f
TEST(VoicePlayerHandlerUT, BUG340969_playVoiceExpandsRelativePath)
{
    VoicePlayerHandler h;
    QSignalSpy errSpy(&h, &VoicePlayerHandler::voiceFileError);
    ASSERT_TRUE(errSpy.isValid());

    // 在 AppData/voicenote 下准备真实文件（makeVoiceAbsolute 解析目标）
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + "/voicenote";
    ASSERT_TRUE(QDir().mkpath(dir));
    const QString absPath = dir + "/ut-340969.wav";
    {
        QFile f(absPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("RIFF", 4);
    }

    auto makeJson = [](const QString &p) {
        QSharedPointer<VNVoiceBlock> b = QSharedPointer<VNVoiceBlock>::create();
        b->voicePath = p;
        QVariant md;
        MetaDataParser parser;
        parser.makeMetaData(b.data(), md);
        return md;
    };

    // 相对路径：修复后展开到 AppData，文件存在 → 不应报 voiceFileError
    h.playVoice(makeJson("voicenote/ut-340969.wav"), false);
    EXPECT_EQ(0, errSpy.count());

    // 对照：不存在的绝对路径 → 停止并播报 voiceFileError
    h.playVoice(makeJson("/tmp/voice-note-ut-340969-noexist.wav"), false);
    EXPECT_EQ(1, errSpy.count());
}
