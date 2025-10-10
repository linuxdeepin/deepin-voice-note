// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice_player_handler.h"

#include <QFile>
#include <QDir>
#include <QLibraryInfo>
#include <QLibrary>
#include <QDebug>

#include "vnoteitem.h"
#include "common/vlcplayer.h"
#include "common/qtplayer.h"
#include "common/metadataparser.h"
#include "common/jscontent.h"
#include "common/utils.h"
#include "opsstateinterface.h"

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
{
    initPlayer();
    // 绑定 Js 前端的控制
    connect(JsContent::instance(), &JsContent::playVoice, this, &VoicePlayerHandler::playVoice);
    connect(JsContent::instance(), &JsContent::playVoiceStop, this, &VoicePlayerHandler::onStop);
    connect(JsContent::instance(), &JsContent::playVoiceProgressChange, this, &VoicePlayerHandler::setPlayPosition);
    connect(this, &VoicePlayerHandler::playStatusChanged, JsContent::instance(), &JsContent::callJsSetPlayStatus);
    connect(this, &VoicePlayerHandler::playPositionChanged, JsContent::instance(), &JsContent::callJsVoicePlayProgressChanged);

    connect(m_player, &VoicePlayerBase::playEnd, this, [this]() {
        OpsStateInterface::instance()->operState(OpsStateInterface::StatePlaying, false);
        Q_EMIT playStatusChanged(End);
    });
    connect(m_player, &VoicePlayerBase::durationChanged, this, &VoicePlayerHandler::playDurationChanged);
    connect(m_player, &VoicePlayerBase::positionChanged, this, &VoicePlayerHandler::playPositionChanged);
}

void VoicePlayerHandler::playVoice(const QVariant &json, bool bIsSame)
{
    qDebug() << "Playing voice, isSame:" << bIsSame;
    // 当前播放的语音为空时，此时bIsSame应该为false
    if (m_voiceBlock.isNull() && bIsSame) {
        qWarning() << "Play voice parameter error: voice block is null but isSame is true";
        bIsSame = false;
    }

    if (!bIsSame) {
        qDebug() << "Creating new voice block from JSON data";
        m_voiceBlock = QSharedPointer<VNVoiceBlock>::create();
        MetaDataParser parser;
        parser.parse(json, m_voiceBlock.get());
    }

    if (!QFile::exists(m_voiceBlock->voicePath)) {
        qWarning() << "Voice file does not exist:" << m_voiceBlock->voicePath;
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
        qInfo() << "Continuing/pausing current voice playback";
        // 切换 暂停/恢复播放 状态
        onToggleStateChange();
    } else if (m_voiceBlock) {  // 与上一次语音不相同，重新播放语音
        qInfo() << "Playing new voice file:" << m_voiceBlock->voicePath;
        m_player->setChangePlayFile(true);
        m_player->setFilePath(m_voiceBlock->voicePath);

        onPlay();
    } else {
        qWarning() << "Play voice parameters are invalid";
    }
}

void VoicePlayerHandler::initPlayer()
{
    qDebug() << "Initializing voice player";
    bool found = false;
    // Using Qt player in linglong environment
    if (!Utils::inLinglongEnv()) {
        QString strlib = "libvlc.so";
        QDir dir;
        QString path  = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
        dir.setPath(path);
        QStringList list = dir.entryList(QStringList() << (strlib + "*"), QDir::NoDotAndDotDot | QDir::Files); //filter name with strlib

        QLibrary library("libvlc.so.5");
        found = list.contains(strlib) || library.load();
        qDebug() << "VLC library search result:" << found;
    }

    if (found) {
        qDebug() << "Creating VLC player instance";
        m_player = new VlcPlayer(this);
        bool successed = static_cast<VlcPlayer*>(m_player)->initVlcPlayer();
        if (!successed) {
            qWarning() << "VLC player initialization failed, falling back to QT player";
            delete m_player;
            m_player = nullptr;

            m_player = new QtPlayer(this);
        } else {
            qInfo() << "VLC player initialized successfully";
        }
    } else {
        qInfo() << "VLC player not found, using QT player";
        m_player = new QtPlayer(this);
    }
}

void VoicePlayerHandler::onPlay()
{
    if (VoicePlayerBase::Playing != m_player->getState()) {
        qDebug() << "Starting voice playback";
        m_player->play();
        OpsStateInterface::instance()->operState(OpsStateInterface::StatePlaying, true);
        Q_EMIT playStatusChanged(Playing);
        qInfo() << "Voice playback started";
    } else {
        qDebug() << "Voice is already playing";
    }
}

void VoicePlayerHandler::onStop()
{
    const VoicePlayerBase::PlayerState state = m_player->getState();
    if (VoicePlayerBase::Playing == state || VoicePlayerBase::Paused == state) {
        qDebug() << "Stopping voice playback";
        m_player->stop();
        OpsStateInterface::instance()->operState(OpsStateInterface::StatePlaying, false);
        Q_EMIT playStatusChanged(End);
        qInfo() << "Voice playback stopped";
    } else {
        qDebug() << "Voice is already stopped";
    }
}

void VoicePlayerHandler::onToggleStateChange()
{
    const VoicePlayerBase::PlayerState state = m_player->getState();
    qDebug() << "Toggling voice playback state, current state:" << state;
    
    switch (state) {
        case VoicePlayerBase::Playing:
            qDebug() << "Pausing voice playback";
            m_player->pause();
            Q_EMIT playStatusChanged(Paused);
            qInfo() << "Voice playback paused";
            break;
        case VoicePlayerBase::Paused:
            qDebug() << "Resuming voice playback";
            m_player->play();
            Q_EMIT playStatusChanged(Playing);
            qInfo() << "Voice playback resumed";
            break;
        case VoicePlayerBase::Stopped:
            Q_FALLTHROUGH();
        case VoicePlayerBase::Ended:
            qDebug() << "Restarting voice playback";
            // 重新播放时需要设置文件
            m_player->setChangePlayFile(true);
            m_player->setFilePath(m_voiceBlock->voicePath);
            m_player->play();
            OpsStateInterface::instance()->operState(OpsStateInterface::StatePlaying, true);
            Q_EMIT playStatusChanged(Playing);
            qInfo() << "Voice playback restarted";
            break;
        default:
            qWarning() << "Unknown player state:" << state;
            break;
    }
}

void VoicePlayerHandler::setPlayPosition(qint64 ms)
{
    const VoicePlayerBase::PlayerState state = m_player->getState();
    if (VoicePlayerBase::Playing == state || VoicePlayerBase::Paused == state) {
        qDebug() << "Setting play position to:" << ms << "ms";
        m_player->setPosition(ms);
    }
}
