// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vlcplayer.h"
#include "ddlog.h"

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
    qCDebug(voiceNoteLog) << "Initializing VlcPlayer";
#ifdef MPV_PLAYENGINE
    init();
#endif
}

/**
 * @brief VlcPlayer::~VlcPlayer
 */
VlcPlayer::~VlcPlayer()
{
    qCDebug(voiceNoteLog) << "Destroying VlcPlayer";
    deinit();
}

/**
 * @brief VlcPlayer::init
 */
void VlcPlayer::init()
{
#ifndef MPV_PLAYENGINE
    qCInfo(voiceNoteLog) << "Initializing VLC player engine";
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
    qCInfo(voiceNoteLog) << "Initializing MPV player engine";
    setlocale(LC_NUMERIC, "C");
    player = new dmr::PlayerEngine(nullptr);
    player->hide();
    player->getplaylist()->setPlayMode(PlaylistModel::PlayMode::SinglePlay);
    connect(player, &dmr::PlayerEngine::elapsedChanged, this, &VlcPlayer::onGetCurrentPosition);
    connect(player, &dmr::PlayerEngine::stateChanged, this, &VlcPlayer::onGetState);
    connect(player, &dmr::PlayerEngine::fileLoaded, this, &VlcPlayer::onGetduration);
    qCDebug(voiceNoteLog) << "MPV player initialized successfully";
#endif
}

/**
 * @brief VlcPlayer::deinit
 */
void VlcPlayer::deinit()
{
    qCDebug(voiceNoteLog) << "Deinitializing player";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        qCDebug(voiceNoteLog) << "Detaching VLC events";
        detachEvent();
        libvlc_media_t *media = g_voice_libvlc_media_player_get_media(m_vlcPlayer);
        if (media)
        {
            g_voice_libvlc_media_release(media);
            qCDebug(voiceNoteLog) << "Released VLC media";
        }
        g_voice_libvlc_media_player_release(m_vlcPlayer);
        m_vlcPlayer = nullptr;
        qCDebug(voiceNoteLog) << "Released VLC player";
    }
    if (m_vlcInst)
    {
        g_voice_libvlc_release(m_vlcInst);
        m_vlcInst = nullptr;
        qCDebug(voiceNoteLog) << "Released VLC instance";
    }
#else
    if (player != nullptr)
    {
        qCDebug(voiceNoteLog) << "Cleaning up MPV player";
        player->hide();
        disconnect(player, &dmr::PlayerEngine::elapsedChanged, this, &VlcPlayer::onGetCurrentPosition);
        disconnect(player, &dmr::PlayerEngine::stateChanged, this, &VlcPlayer::onGetState);
        disconnect(player, &dmr::PlayerEngine::fileLoaded, this, &VlcPlayer::onGetduration);
        player->deleteLater();
        qCDebug(voiceNoteLog) << "MPV player cleanup completed";
    }
#endif
}

bool VlcPlayer::initVlcPlayer()
{
    qCDebug(voiceNoteLog) << "Initializing VLC player library";
    QLibrary library("libvlc.so.5");
    if (!library.load()) {
        qCWarning(voiceNoteLog) << "Failed to load libvlc.so.5";
        return false;
    }
    qCDebug(voiceNoteLog) << "VLC library loaded successfully";

    // Resolve all function pointers
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

    qCDebug(voiceNoteLog) << "All VLC function pointers resolved successfully";
    return true;
}

#ifndef MPV_PLAYENGINE
/**
 * @brief VlcPlayer::attachEvent
 */
void VlcPlayer::attachEvent()
{
    qCDebug(voiceNoteLog) << "Attaching VLC events";
    libvlc_event_manager_t *vlc_evt_man = g_voice_libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man)
    {
        g_voice_libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        g_voice_libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        g_voice_libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
        qCDebug(voiceNoteLog) << "VLC events attached successfully";
    } else {
        qCWarning(voiceNoteLog) << "Failed to get VLC event manager";
    }
}

/**
 * @brief VlcPlayer::detachEvent
 */
void VlcPlayer::detachEvent()
{
    qCDebug(voiceNoteLog) << "Detaching VLC events";
    libvlc_event_manager_t *vlc_evt_man = g_voice_libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man)
    {
        g_voice_libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        g_voice_libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        g_voice_libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
        qCDebug(voiceNoteLog) << "VLC events detached successfully";
    } else {
        qCWarning(voiceNoteLog) << "Failed to get VLC event manager for detach";
    }
}
#endif

/**
 * @brief VlcPlayer::setFilePath
 * @param path 文件路径
 */
void VlcPlayer::setFilePath(QString path)
{
    qCDebug(voiceNoteLog) << "Setting media file path:" << path;
#ifndef MPV_PLAYENGINE
    init();
    libvlc_media_t *media = g_voice_libvlc_media_new_path(m_vlcInst, path.toLatin1().constData());
    if (media)
    {
        g_voice_libvlc_media_player_set_media(m_vlcPlayer, media);
        g_voice_libvlc_media_release(media);
        qCDebug(voiceNoteLog) << "Media file set successfully";
    } else {
        qCWarning(voiceNoteLog) << "Failed to create media from path:" << path;
    }
#else
    if (player->isPlayableFile(path))
    {
        videoUrl = QUrl::fromLocalFile(path);
        qCDebug(voiceNoteLog) << "MPV playable file set:" << path;
    } else {
        qCWarning(voiceNoteLog) << "File is not playable by MPV:" << path;
    }
#endif
}

/**
 * @brief VlcPlayer::play
 */
void VlcPlayer::play()
{
    qCInfo(voiceNoteLog) << "Starting playback";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        int ret = g_voice_libvlc_media_player_play(m_vlcPlayer);
        qCDebug(voiceNoteLog) << "VLC play result:" << ret;
    } else {
        qCWarning(voiceNoteLog) << "VLC player not initialized";
    }
#else
    qCDebug(voiceNoteLog) << "MPV player state:" << player->state();
    if (player->state() == PlayerEngine::CoreState::Paused && !m_isChangePlayFile)
    {
        qCDebug(voiceNoteLog) << "Resuming paused playback:" << videoUrl;
        player->pauseResume();
    }
    else
    {
        qCDebug(voiceNoteLog) << "Starting new playback:" << videoUrl;
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
    qCInfo(voiceNoteLog) << "Pausing playback";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        g_voice_libvlc_media_player_pause(m_vlcPlayer);
        qCDebug(voiceNoteLog) << "VLC playback paused";
    } else {
        qCWarning(voiceNoteLog) << "VLC player not initialized";
    }
#else
    player->pauseResume();
    qCDebug(voiceNoteLog) << "MPV playback paused";
#endif
}

/**
 * @brief VlcPlayer::stop
 */
void VlcPlayer::stop()
{
    qCInfo(voiceNoteLog) << "Stopping playback";
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        g_voice_libvlc_media_player_stop(m_vlcPlayer);
        qCDebug(voiceNoteLog) << "VLC playback stopped";
    } else {
        qCWarning(voiceNoteLog) << "VLC player not initialized";
    }
#else
    player->stop();
    qCDebug(voiceNoteLog) << "MPV playback stopped";
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
    qCDebug(voiceNoteLog) << "Player state changed:" << player->state();
    if (player->state() == PlayerEngine::CoreState::Idle && !m_isChangePlayFile)
    {
        qCInfo(voiceNoteLog) << "Playback completed";
        emit playEnd();
    }
    else if (player->state() == PlayerEngine::CoreState::Playing && m_isChangePlayFile)
    {
        m_isChangePlayFile = false;
        qCDebug(voiceNoteLog) << "Playback resumed after file change";
    }
}
void VlcPlayer::onGetduration()
{
    qCDebug(voiceNoteLog) << "Duration changed:" << player->duration();
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
        qCInfo(voiceNoteLog) << "Playback completed";
        emit userData->playEnd();
        break;
    case libvlc_MediaPlayerTimeChanged:
        qCDebug(voiceNoteLog) << "Position changed:" << event->u.media_player_time_changed.new_time;
        emit userData->positionChanged(event->u.media_player_time_changed.new_time);
        break;
    case libvlc_MediaPlayerLengthChanged:
        qCDebug(voiceNoteLog) << "Duration changed:" << event->u.media_duration_changed.new_duration;
        emit userData->durationChanged(event->u.media_duration_changed.new_duration);
        break;
    default:
        qCDebug(voiceNoteLog) << "Unhandled VLC event type:" << event->type;
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
    qCDebug(voiceNoteLog) << "Setting position:" << pos;
#ifndef MPV_PLAYENGINE
    if (m_vlcPlayer)
    {
        g_voice_libvlc_media_player_set_time(m_vlcPlayer, pos);
        qCDebug(voiceNoteLog) << "VLC position set successfully";
    } else {
        qCWarning(voiceNoteLog) << "VLC player not initialized";
    }
#else
    player->seekAbsolute(pos);
    qCDebug(voiceNoteLog) << "MPV position set successfully";
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
        qCDebug(voiceNoteLog) << "VLC player state:" << state;
    } else {
        qCWarning(voiceNoteLog) << "VLC player not initialized";
    }
#else
    qCDebug(voiceNoteLog) << "MPV player state:" << player->state();
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
