/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "vlcpalyer.h"

#include <vlc/vlc.h>
#include <vlc/libvlc_media.h>

#include <DApplication>

DWIDGET_USE_NAMESPACE
/**
 * @brief VlcPalyer::VlcPalyer
 * @param parent
 */
VlcPalyer::VlcPalyer(QObject *parent)
    : QObject(parent)
{
    ;
}

/**
 * @brief VlcPalyer::~VlcPalyer
 */
VlcPalyer::~VlcPalyer()
{
    deinit();
}

/**
 * @brief VlcPalyer::init
 */
void VlcPalyer::init()
{
    if (m_vlcInst == nullptr) {
        m_vlcInst = libvlc_new(0, nullptr);
        libvlc_set_user_agent(m_vlcInst, DApplication::translate("AppMain", "Voice Notes").toUtf8().constData(), "");
        libvlc_set_app_id(m_vlcInst, "", "", "deepin-voice-note");
    }
    if (m_vlcPlayer == nullptr) {
        m_vlcPlayer = libvlc_media_player_new(m_vlcInst);
        libvlc_media_player_set_role(m_vlcPlayer, libvlc_role_None);
        attachEvent();
    }
}

/**
 * @brief VlcPalyer::deinit
 */
void VlcPalyer::deinit()
{
    if (m_vlcPlayer) {
        detachEvent();
        libvlc_media_t *media = libvlc_media_player_get_media(m_vlcPlayer);
        if (media) {
            libvlc_media_release(media);
        }
        libvlc_media_player_release(m_vlcPlayer);
        m_vlcPlayer = nullptr;
    }
    if (m_vlcInst) {
        libvlc_release(m_vlcInst);
        m_vlcInst = nullptr;
    }
}

/**
 * @brief VlcPalyer::attachEvent
 */
void VlcPalyer::attachEvent()
{
    libvlc_event_manager_t *vlc_evt_man = libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man) {
        libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
    }
}

/**
 * @brief VlcPalyer::detachEvent
 */
void VlcPalyer::detachEvent()
{
    libvlc_event_manager_t *vlc_evt_man = libvlc_media_player_event_manager(m_vlcPlayer);
    if (vlc_evt_man) {
        libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerEndReached, handleEvent, this);
        libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerTimeChanged, handleEvent, this);
        libvlc_event_detach(vlc_evt_man, libvlc_MediaPlayerLengthChanged, handleEvent, this);
    }
}

/**
 * @brief VlcPalyer::setFilePath
 * @param path 文件路径
 */
void VlcPalyer::setFilePath(QString path)
{
    init();
    libvlc_media_t *media = libvlc_media_new_path(m_vlcInst, path.toLatin1().constData());
    if (media) {
        libvlc_media_player_set_media(m_vlcPlayer, media);
        libvlc_media_release(media);
    }
}

/**
 * @brief VlcPalyer::play
 */
void VlcPalyer::play()
{
    if (m_vlcPlayer) {
        libvlc_media_player_play(m_vlcPlayer);
    }
}

/**
 * @brief VlcPalyer::pause
 */
void VlcPalyer::pause()
{
    if (m_vlcPlayer) {
        libvlc_media_player_pause(m_vlcPlayer);
    }
}

/**
 * @brief VlcPalyer::stop
 */
void VlcPalyer::stop()
{
    if (m_vlcPlayer) {
        libvlc_media_player_stop(m_vlcPlayer);
    }
}

/**
 * @brief VlcPalyer::handleEvent
 * @param event
 * @param data
 */
void VlcPalyer::handleEvent(const libvlc_event_t *event, void *data)
{
    VlcPalyer *userData = static_cast<VlcPalyer *>(data);
    switch (event->type) {
    case libvlc_MediaPlayerEndReached:
        emit userData->playEnd();
        break;
    case libvlc_MediaPlayerTimeChanged:
        emit userData->positionChanged(event->u.media_player_time_changed.new_time);
        break;
    case libvlc_MediaPlayerLengthChanged:
        emit userData->durationChanged(event->u.media_duration_changed.new_duration);
        break;
    default:
        break;
    }
}

/**
 * @brief VlcPalyer::setPosition
 * @param pos
 */
void VlcPalyer::setPosition(qint64 pos)
{
    if (m_vlcPlayer) {
        libvlc_media_player_set_time(m_vlcPlayer, pos);
    }
}

/**
 * @brief VlcPalyer::getState
 * @return 状态
 */
VlcPalyer::VlcState VlcPalyer::getState()
{
    VlcPalyer::VlcState state = None;
    if (m_vlcPlayer) {
        state = static_cast<VlcPalyer::VlcState>(libvlc_media_player_get_state(m_vlcPlayer));
    }
    return state;
}
