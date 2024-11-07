// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VLCPLAYER_H
#define VLCPLAYER_H
// TODO(renbin): libdmr 适配 qt6
// #define MPV_PLAYENGINE  ///MPV播放引擎 通过是否定义该宏，来切换采用vlc或者libdmr来播放音频

#include <QObject>
#include "voiceplayerbase.h"
#ifdef MPV_PLAYENGINE
#include <player_widget.h>
#include <player_engine.h>
using namespace dmr;
#endif

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_event_t;

class VlcPlayer : public VoicePlayerBase
{
    Q_OBJECT
public:
    enum VlcState
    {
        None = 0,
        Opening,
        Buffering,
        Playing,
        Paused,
        Stopped,
        Ended,
        Error
    };
    Q_ENUM(VlcState)
    explicit VlcPlayer(QObject *parent = nullptr);
    ~VlcPlayer();
    // 设置播放文件
    void setFilePath(QString path);
    // 跳转指定位置，单位:毫秒
    void setPosition(qint64 pos);
    // 播放
    void play();
    // 暂停
    void pause();
    // 停止
    void stop();
    // 获取状态
    PlayerState getState();
    void setChangePlayFile(bool flag);
    bool initVlcPlayer();

public slots:
#ifdef MPV_PLAYENGINE
    void onGetCurrentPosition();
    void onGetState();
    void onGetduration();
#endif

private:
#ifndef MPV_PLAYENGINE
    // 添加事件监控
    void attachEvent();
    // 移除事件监控
    void detachEvent();
    // 事件处理
    static void handleEvent(const libvlc_event_t *event, void *data);
#endif
    // 初始化
    void init();
    // 释放资源
    void deinit();
    libvlc_instance_t *m_vlcInst{nullptr};
    libvlc_media_player_t *m_vlcPlayer{nullptr};
#ifdef MPV_PLAYENGINE
    dmr::PlayerEngine *player{nullptr};
    QUrl videoUrl;

    bool m_isChangePlayFile{false};
#endif
};

#endif // VLCPLAYER_H
