// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vlcplayer.h"

#include <vlc/vlc.h>
#include <vlc/libvlc_media.h>

#include <DApplication>

DWIDGET_USE_NAMESPACE
/**
 * @brief VlcPlayer::VlcPlayer
 * @param parent
 */
VlcPlayer::VlcPlayer(QObject *parent)
    : QObject(parent)
{
#ifdef MPV_PLAYENGINE
    init();
#endif
}

/**
 * @brief VlcPlayer::~VlcPlayer
 */
VlcPlayer::~VlcPlayer()
{
    deinit();
}

/**
 * @brief VlcPlayer::init
 */
void VlcPlayer::init()
{
#ifndef MPV_PLAYENGINE
    qInfo() << "The current play engine is vlc";
    if (m_vlcInst == nullptr)
    {
        m_vlcInst = libvlc_new(0, nullptr);
        libvlc_set_user_agent(m_vlcInst, DApplication::translate("AppMain", "Voice Notes").toUtf8().constData(), "");
        libvlc_set_app_id(m_vlcInst, "", "", "deepin-voice-note");
    }
    if (m_vlcPlayer == nullptr)
    {
        m_vlcPlayer = libvlc_media_player_new(m_vlcInst);
        libvlc_media_player_set_role(m_vlcPlayer, libvlc_role_None);
        attachEvent();
    }
#else
    qInfo() << "The current play engine is mpv";
    setlocale(LC_NUMERIC, "C");
    player = new dmr::PlayerEngine(nullptr);
    player->hide();
    player->getplaylist()->setPlayMode(PlaylistModel::PlayMode::SinglePlay);
    connect(player, &dmr::PlayerEngine::elapsedChanged, this, &VlcPlayer::onGetCurrentPosition);
    connect(player, &dmr::PlayerEngine::stateChanged, this, &VlcPlayer::onGetState);
    connect(player, &dmr::PlayerEngine::fileLoaded, this, &VlcPlayer::onGetduration);
#endif
}

/**
 * @brief VlcPlayer::deinit
 */
void VlcPlayer::deinit()
{
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        detachEvent();
        libvlc_media_t *media = libvlc_media_player_get_media(m_vlcPlayer);
        if (media)
        {
            libvlc_media_release(media);
        }
        libvlc_media_player_release(m_vlcPlayer);
        m_vlcPlayer = nullptr;
    }
    if (m_vlcInst)
    {
        libvlc_release(m_vlcInst);
        m_vlcInst = nullptr;
    }
#else
    if (player != nullptr)
    {
        player->hide();
        disconnect(player, &dmr::PlayerEngine::elapsedChanged, this, &VlcPlayer::onGetCurrentPosition);
        disconnect(player, &dmr::PlayerEngine::stateChanged, this, &VlcPlayer::onGetState);
        disconnect(player, &dmr::PlayerEngine::fileLoaded, this, &VlcPlayer::onGetduration);
        player->deleteLater();
    }
#endif
}

#ifndef MPV_PLAYENGINE
/**
 * @brief VlcPlayer::attachEvent
 */
void VlcPlayer::attachEvent()
{
    libvlc_event_manager_t *vlc_evt_man = libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man)
    {
        libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
    }
}

/**
 * @brief VlcPlayer::detachEvent
 */
void VlcPlayer::detachEvent()
{
    libvlc_event_manager_t *vlc_evt_man = libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man)
    {
        libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
    }
}
#endif

/**
 * @brief VlcPlayer::setFilePath
 * @param path 文件路径
 */
void VlcPlayer::setFilePath(QString path)
{
    qDebug() << "Current playback file: " << path;
#ifndef MPV_PLAYENGINE
    init();
    libvlc_media_t *media = libvlc_media_new_path(m_vlcInst, path.toLatin1().constData());
    if (media)
    {
        libvlc_media_player_set_media(m_vlcPlayer, media);
        libvlc_media_release(media);
    }
#else
    if (player->isPlayableFile(path))
    {
        videoUrl = QUrl::fromLocalFile(path);
    }
#endif
}

/**
 * @brief VlcPlayer::play
 */
void VlcPlayer::play()
{
    qInfo() << "Start playing audio";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        qInfo() << "libvlc play ret:" << libvlc_media_player_play(m_vlcPlayer);
    }
#else
    qInfo() << "Current play engine status: " << player->state();
    if (player->state() == PlayerEngine::CoreState::Paused && !m_isChangePlayFile)
    {
        qInfo() << "Pause start: " << videoUrl;
        player->pauseResume();
    }
    else
    {
        qInfo() << "Play new audio: " << videoUrl;
        player->addPlayFile(videoUrl);
        player->playByName(videoUrl);
    }
#endif
}

/**
 * @brief VlcPlayer::pause
 */
void VlcPlayer::pause()
{
    qInfo() << "Pause play";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        libvlc_media_player_pause(m_vlcPlayer);
    }
#else
    player->pauseResume();
#endif
}

/**
 * @brief VlcPlayer::stop
 */
void VlcPlayer::stop()
{
    qInfo() << "Stop playing";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        libvlc_media_player_stop(m_vlcPlayer);
    }
#else
    player->stop();
#endif
}
#ifdef MPV_PLAYENGINE
/**
 * @brief VlcPlayer::onGetCurrentPosition
 */
void VlcPlayer::onGetCurrentPosition()
{
    //    qInfo() << "The current play position has changed!" << player->engine().elapsed();
    emit positionChanged(player->elapsed());
}

/**
 * @brief VlcPlayer::onGetState
 */
void VlcPlayer::onGetState()
{
    qInfo() << "Change the current audio playback status!(status: " << player->state() << ")";
    if (player->state() == PlayerEngine::CoreState::Idle && !m_isChangePlayFile)
    {
        qInfo() << "The current audio playback is complete! (status: " << player->state() << ")";
        emit playEnd();
    }
    else if (player->state() == PlayerEngine::CoreState::Playing && m_isChangePlayFile)
    {
        m_isChangePlayFile = false;
    }
}
void VlcPlayer::onGetduration()
{
    qInfo() << "Total audio duration changed! (duration: " << player->duration() << ")";
    emit durationChanged(player->duration());
}
#endif

#ifndef MPV_PLAYENGINE

/**
 * @brief VlcPlayer::handleEvent
 * @param event
 * @param data
 */
void VlcPlayer::handleEvent(const libvlc_event_t *event, void *data)
{
    VlcPlayer *userData = static_cast<VlcPlayer *>(data);
    switch (event->type)
    {
    case libvlc_MediaPlayerEndReached:
        qInfo() << "The current audio playback is complete!";
        emit userData->playEnd();
        break;
    case libvlc_MediaPlayerTimeChanged:
        qInfo() << "The current play position has changed!";
        emit userData->positionChanged(event->u.media_player_time_changed.new_time);
        break;
    case libvlc_MediaPlayerLengthChanged:
        qInfo() << "Total audio duration changed!";
        emit userData->durationChanged(event->u.media_duration_changed.new_duration);
        break;
    default:
        break;
    }
}
#endif

/**
 * @brief VlcPlayer::setPosition
 * @param pos
 */
void VlcPlayer::setPosition(qint64 pos)
{
    qInfo() << "Set the current audio playback position.(pos: " << pos << ")";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        libvlc_media_player_set_time(m_vlcPlayer, pos);
    }
#else
    player->seekAbsolute(pos);
#endif
}

/**
 * @brief VlcPlayer::getState
 * @return 状态
 */
VlcPlayer::VlcState VlcPlayer::getState()
{
    VlcPlayer::VlcState state = None;
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        state = static_cast<VlcPlayer::VlcState>(libvlc_media_player_get_state(m_vlcPlayer));
    }
    qInfo() << "Gets the current audio status.(status: " << state << ")";
#else
    qInfo() << "Gets the current audio status.(status: " << player->state() << ")";
    if (player->state() == PlayerEngine::CoreState::Idle)
    {
        state = VlcPlayer::VlcState::Stopped;
    }
    else if (player->state() == PlayerEngine::CoreState::Playing)
    {
        state = VlcPlayer::VlcState::Playing;
    }
    else
    {
        state = VlcPlayer::VlcState::Paused;
    }
#endif
    return state;
}

void VlcPlayer::setChangePlayFile(bool flag)
{
#ifdef MPV_PLAYENGINE
    m_isChangePlayFile = flag;
#endif
}
