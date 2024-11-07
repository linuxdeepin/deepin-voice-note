// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vlcplayer.h"

#include <vlc/vlc.h>
#include <vlc/libvlc_media.h>

#include <QLibrary>

#include <DApplication>

typedef libvlc_event_manager_t* (*voice_libvlc_media_player_event_manager) ( libvlc_media_player_t *p_mi );
typedef void (*voice_libvlc_event_detach)( libvlc_event_manager_t *p_event_manager, libvlc_event_type_t i_event_type, libvlc_callback_t f_callback, void *p_user_data );
typedef libvlc_media_t * (*voice_libvlc_media_player_get_media)( libvlc_media_player_t *p_mi );
typedef void (*voice_libvlc_media_release)( libvlc_media_t *p_md );
typedef void (*voice_libvlc_media_player_release)( libvlc_media_player_t *p_mi );
typedef void (*voice_libvlc_release)( libvlc_instance_t *p_instance );
typedef libvlc_instance_t * (*voice_libvlc_new)( int argc , const char *const *argv );
typedef int (*voice_libvlc_event_attach)( libvlc_event_manager_t *p_event_manager, libvlc_event_type_t i_event_type, libvlc_callback_t f_callback, void *user_data );
typedef void (*voice_libvlc_set_user_agent)( libvlc_instance_t *p_instance, const char *name, const char *http );
typedef libvlc_media_t *(*voice_libvlc_media_new_path)( libvlc_instance_t *p_instance, const char *path );
typedef int (*voice_libvlc_media_player_play) ( libvlc_media_player_t *p_mi );
typedef void (*voice_libvlc_set_app_id)( libvlc_instance_t *p_instance, const char *id, const char *version, const char *icon );
typedef libvlc_media_player_t * (*voice_libvlc_media_player_new)( libvlc_instance_t *p_libvlc_instance );
typedef int (*voice_libvlc_media_player_set_role)(libvlc_media_player_t *p_mi, unsigned role);
typedef void (*voice_libvlc_media_player_set_media)( libvlc_media_player_t *p_mi, libvlc_media_t *p_md );
typedef void (*voice_libvlc_media_player_pause) ( libvlc_media_player_t *p_mi );
typedef void (*voice_libvlc_media_player_stop) ( libvlc_media_player_t *p_mi );
typedef void (*voice_libvlc_media_player_set_time)( libvlc_media_player_t *p_mi, libvlc_time_t i_time );
typedef libvlc_state_t (*voice_libvlc_media_player_get_state)( libvlc_media_player_t *p_mi );

voice_libvlc_media_player_event_manager g_voice_libvlc_media_player_event_manager = nullptr;
voice_libvlc_event_detach g_voice_libvlc_event_detach = nullptr;
voice_libvlc_media_player_get_media g_voice_libvlc_media_player_get_media = nullptr;
voice_libvlc_media_release g_voice_libvlc_media_release = nullptr;
voice_libvlc_media_player_release g_voice_libvlc_media_player_release = nullptr;
voice_libvlc_release g_voice_libvlc_release = nullptr;
voice_libvlc_new g_voice_libvlc_new = nullptr;
voice_libvlc_event_attach g_voice_libvlc_event_attach = nullptr;
voice_libvlc_set_user_agent g_voice_libvlc_set_user_agent = nullptr;
voice_libvlc_media_new_path g_voice_libvlc_media_new_path = nullptr;
voice_libvlc_media_player_play g_voice_libvlc_media_player_play = nullptr;
voice_libvlc_set_app_id g_voice_libvlc_set_app_id = nullptr;
voice_libvlc_media_player_new g_voice_libvlc_media_player_new = nullptr;
voice_libvlc_media_player_set_role g_voice_libvlc_media_player_set_role = nullptr;
voice_libvlc_media_player_set_media g_voice_libvlc_media_player_set_media = nullptr;
voice_libvlc_media_player_pause g_voice_libvlc_media_player_pause = nullptr;
voice_libvlc_media_player_stop g_voice_libvlc_media_player_stop = nullptr;
voice_libvlc_media_player_set_time g_voice_libvlc_media_player_set_time = nullptr;
voice_libvlc_media_player_get_state g_voice_libvlc_media_player_get_state = nullptr;

DWIDGET_USE_NAMESPACE
/**
 * @brief VlcPlayer::VlcPlayer
 * @param parent
 */
VlcPlayer::VlcPlayer(QObject *parent)
    : VoicePlayerBase(parent)
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
        m_vlcInst = g_voice_libvlc_new(0, nullptr);
        g_voice_libvlc_set_user_agent(m_vlcInst, DApplication::translate("AppMain", "Voice Notes").toUtf8().constData(), "");
        g_voice_libvlc_set_app_id(m_vlcInst, "", "", "deepin-voice-note");
    }
    if (m_vlcPlayer == nullptr)
    {
        m_vlcPlayer = g_voice_libvlc_media_player_new(m_vlcInst);
        g_voice_libvlc_media_player_set_role(m_vlcPlayer, libvlc_role_None);
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
        libvlc_media_t *media = g_voice_libvlc_media_player_get_media(m_vlcPlayer);
        if (media)
        {
            g_voice_libvlc_media_release(media);
        }
        g_voice_libvlc_media_player_release(m_vlcPlayer);
        m_vlcPlayer = nullptr;
    }
    if (m_vlcInst)
    {
        g_voice_libvlc_release(m_vlcInst);
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

bool VlcPlayer::initVlcPlayer()
{
    QLibrary library("libvlc.so.5");
    if (!library.load())
        return false;

    g_voice_libvlc_media_player_event_manager = (voice_libvlc_media_player_event_manager)library.resolve("libvlc_media_player_event_manager");
    g_voice_libvlc_event_detach = (voice_libvlc_event_detach)library.resolve("libvlc_event_detach");
    g_voice_libvlc_media_player_get_media = (voice_libvlc_media_player_get_media)library.resolve("libvlc_media_player_get_media");
    g_voice_libvlc_media_release = (voice_libvlc_media_release)library.resolve("libvlc_media_release");
    g_voice_libvlc_media_player_release = (voice_libvlc_media_player_release)library.resolve("libvlc_media_player_release");
    g_voice_libvlc_release = (voice_libvlc_release)library.resolve("libvlc_release");
    g_voice_libvlc_new = (voice_libvlc_new)library.resolve("libvlc_new");
    g_voice_libvlc_event_attach = (voice_libvlc_event_attach)library.resolve("libvlc_event_attach");
    g_voice_libvlc_set_user_agent = (voice_libvlc_set_user_agent)library.resolve("libvlc_set_user_agent");
    g_voice_libvlc_media_new_path = (voice_libvlc_media_new_path)library.resolve("libvlc_media_new_path");
    g_voice_libvlc_media_player_play = (voice_libvlc_media_player_play)library.resolve("libvlc_media_player_play");
    g_voice_libvlc_set_app_id = (voice_libvlc_set_app_id)library.resolve("libvlc_set_app_id");
    g_voice_libvlc_media_player_new = (voice_libvlc_media_player_new)library.resolve("libvlc_media_player_new");
    g_voice_libvlc_media_player_set_role = (voice_libvlc_media_player_set_role)library.resolve("libvlc_media_player_set_role");
    g_voice_libvlc_media_player_set_media = (voice_libvlc_media_player_set_media)library.resolve("libvlc_media_player_set_media");
    g_voice_libvlc_media_player_pause = (voice_libvlc_media_player_pause)library.resolve("libvlc_media_player_pause");
    g_voice_libvlc_media_player_stop = (voice_libvlc_media_player_stop)library.resolve("libvlc_media_player_stop");
    g_voice_libvlc_media_player_set_time = (voice_libvlc_media_player_set_time)library.resolve("libvlc_media_player_set_time");
    g_voice_libvlc_media_player_get_state = (voice_libvlc_media_player_get_state)library.resolve("libvlc_media_player_get_state");
    return true;
}

#ifndef MPV_PLAYENGINE
/**
 * @brief VlcPlayer::attachEvent
 */
void VlcPlayer::attachEvent()
{
    libvlc_event_manager_t *vlc_evt_man = g_voice_libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man)
    {
        g_voice_libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        g_voice_libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        g_voice_libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
    }
}

/**
 * @brief VlcPlayer::detachEvent
 */
void VlcPlayer::detachEvent()
{
    libvlc_event_manager_t *vlc_evt_man = g_voice_libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man)
    {
        g_voice_libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        g_voice_libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        g_voice_libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
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
    libvlc_media_t *media = g_voice_libvlc_media_new_path(m_vlcInst, path.toLatin1().constData());
    if (media)
    {
        g_voice_libvlc_media_player_set_media(m_vlcPlayer, media);
        g_voice_libvlc_media_release(media);
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
        qInfo() << "libvlc play ret:" << g_voice_libvlc_media_player_play(m_vlcPlayer);
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
        g_voice_libvlc_media_player_pause(m_vlcPlayer);
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
        g_voice_libvlc_media_player_stop(m_vlcPlayer);
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
        g_voice_libvlc_media_player_set_time(m_vlcPlayer, pos);
    }
#else
    player->seekAbsolute(pos);
#endif
}

/**
 * @brief VlcPlayer::getState
 * @return 状态
 */
VoicePlayerBase::PlayerState VlcPlayer::getState()
{
    VlcPlayer::VlcState state = None;
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        state = static_cast<VlcPlayer::VlcState>(g_voice_libvlc_media_player_get_state(m_vlcPlayer));
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
    return (VoicePlayerBase::PlayerState)state;
}

void VlcPlayer::setChangePlayFile(bool flag)
{
#ifdef MPV_PLAYENGINE
    m_isChangePlayFile = flag;
#endif
}
