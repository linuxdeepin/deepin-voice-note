// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice_player_handler.h"

#include <QFile>
#include <QDebug>

#include "vnoteitem.h"
#include "common/vlcplayer.h"
#include "common/metadataparser.h"
#include "common/jscontent.h"

// 进度条刻度，按秒进位
const int kTickMilliseconds = 1000;

/**
   @class VoicePlayerHandler
   @brief 语音播放处理类，同步 c++ 后端 和 html/qml 前端的播放状态和进度
    数据的传递流程为 [js: VoicePlayer] <-> [c++: JsContent] <-> [c++: VoicePlayerHandler] <-> [qml: VoicePlayer]
    处理类会绑定 JsContent 的相关信号，用于
 */
VoicePlayerHandler::VoicePlayerHandler(QObject *parent)
    : QObject { parent }
    , m_player(new VlcPlayer(this))
{
    // 绑定 Js 前端的控制
    connect(JsContent::instance(), &JsContent::playVoice, this, &VoicePlayerHandler::playVoice);
    connect(JsContent::instance(), &JsContent::playVoiceStop, this, &VoicePlayerHandler::onStop);
    connect(JsContent::instance(), &JsContent::playVoiceProgressChange, this, &VoicePlayerHandler::setPlayPosition);
    connect(this, &VoicePlayerHandler::playStatusChanged, JsContent::instance(), &JsContent::callJsSetPlayStatus);
    connect(this, &VoicePlayerHandler::playPositionChanged, JsContent::instance(), &JsContent::callJsVoicePlayProgressChanged);

    connect(m_player, &VlcPlayer::playEnd, this, [this]() { Q_EMIT playStatusChanged(End); });
    connect(m_player, &VlcPlayer::durationChanged, this, &VoicePlayerHandler::playDurationChanged);
    connect(m_player, &VlcPlayer::positionChanged, this, &VoicePlayerHandler::playPositionChanged);
}

void VoicePlayerHandler::playVoice(const QVariant &json, bool bIsSame)
{
    // 当前播放的语音为空时，此时bIsSame应该为false
    if (m_voiceBlock.isNull() && bIsSame) {
        qWarning() << "Play voice parameter error";
        bIsSame = false;
    }

    if (!bIsSame) {
        m_voiceBlock = QSharedPointer<VNVoiceBlock>::create();
        MetaDataParser parser;
        parser.parse(json, m_voiceBlock.get());
    }

    if (!QFile::exists(m_voiceBlock->voicePath)) {
        // 停止当前播放，并通知文件错误，将删除富文本中的记录
        onStop();
        Q_EMIT voiceFileError();

        return;
    }

    playVoiceImpl(bIsSame);
}

void VoicePlayerHandler::playVoiceImpl(bool bIsSame)
{
    if (bIsSame && m_voiceBlock) {
        qInfo() << "As with the last voice, continue/pause";
        // 切换 暂停/恢复播放 状态
        onToggleStateChange();
    } else if (m_voiceBlock) {  // 与上一次语音不相同，重新播放语音
        qInfo() << "Different from the last voice, play the voice again";
        m_player->setChangePlayFile(true);
        m_player->setFilePath(m_voiceBlock->voicePath);

        onPlay();
    } else {
        qInfo() << "paly voice param is error";
    }
}

void VoicePlayerHandler::onPlay()
{
    if (VlcPlayer::Playing != m_player->getState()) {
        m_player->play();
        Q_EMIT playStatusChanged(Playing);
    }
}

void VoicePlayerHandler::onStop()
{
    const VlcPlayer::VlcState state = m_player->getState();
    if (VlcPlayer::Playing == state || VlcPlayer::Paused == state) {
        m_player->stop();
        Q_EMIT playStatusChanged(End);
    }
}

void VoicePlayerHandler::onToggleStateChange()
{
    const VlcPlayer::VlcState state = m_player->getState();
    switch (state) {
        case VlcPlayer::Playing:
            m_player->pause();
            Q_EMIT playStatusChanged(Paused);
            break;
        case VlcPlayer::Paused:
            m_player->play();
            Q_EMIT playStatusChanged(Playing);
            break;
        case VlcPlayer::Stopped:
            Q_FALLTHROUGH();
        case VlcPlayer::Ended:
            // 重新播放时需要设置文件
            m_player->setChangePlayFile(true);
            m_player->setFilePath(m_voiceBlock->voicePath);
            m_player->play();
            Q_EMIT playStatusChanged(Playing);
            break;
        default:
            break;
    }
}

void VoicePlayerHandler::setPlayPosition(qint64 ms)
{
    const VlcPlayer::VlcState state = m_player->getState();
    if (VlcPlayer::Playing == state || VlcPlayer::Paused == state) {
        m_player->setPosition(ms);
    }
}
