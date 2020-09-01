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
#ifndef VLCPALYER_H
#define VLCPALYER_H

#include <QObject>

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_event_t;

class VlcPalyer : public QObject
{
    Q_OBJECT
public:
    enum VlcState{
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
    explicit VlcPalyer(QObject *parent = nullptr);
    ~VlcPalyer();
    //设置播放文件
    void setFilePath(QString path);
    //跳转指定位置，单位:毫秒
    void setPosition(qint64 pos);
    //播放
    void play ();
    //暂停
    void pause();
    //停止
    void stop ();
    //获取状态
    VlcState getState();
signals:
    //总时长改变
    void durationChanged(qint64 duration);
    //播放位置改变，单位:毫秒
    void positionChanged(qint64 position);
    //播放完成
    void playEnd();
public slots:
private:
    //添加事件监控
    void attachEvent();
    //移除事件监控
    void detachEvent();
    //初始化
    void init();
    //释放资源
    void deinit();
    //事件处理
    static void handleEvent(const libvlc_event_t *event, void *data);
    libvlc_instance_t     *m_vlcInst {nullptr};
    libvlc_media_player_t *m_vlcPlayer {nullptr};
};

#endif // VLCPALYER_H
