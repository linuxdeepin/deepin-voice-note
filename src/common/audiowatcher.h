/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef AudioWatcher_H
#define AudioWatcher_H

#include <QObject>
#include <com_deepin_daemon_audio.h>
#include <com_deepin_daemon_audio_source.h>
#include <com_deepin_daemon_audio_sink.h>

class AudioWatcher : public QObject
{
    Q_OBJECT
public:
    enum AudioMode { Internal,
                     Micphone };
    Q_ENUM(AudioMode)
    explicit AudioWatcher(QObject *parent = nullptr);
    //获取设备名称
    QString getDeviceName(AudioMode mode);
    //获取设备音量
    double getVolume(AudioMode mode);
    //判断设备是否静音
    bool getMute(AudioMode mode);
signals:
    //音量改变信号
    void sigVolumeChange(AudioMode mode);
    //设备改变信号
    void sigDeviceChange(AudioMode mode);
    //静音状态改变信号
    void sigMuteChanged(AudioMode mode);
protected slots:
    //输入设备默认端口改变
    void onDefaultSourceActivePortChanged(AudioPort value);
    //输出设备默认端口改变
    void onDefaultSinkActivePortChanged(AudioPort value);
    //默认输入设备改变
    void onDefaultSourceChanaged(const QDBusObjectPath &value);
    //默认输出设备改变
    void onDefaultSinkChanaged(const QDBusObjectPath &value);
    //默认输入设备音量改变
    void onSourceVolumeChanged(double value);
    //默认输出设备音量改变
    void onSinkVolumeChanged(double value);
    //默认输入设备静音状态改变
    void onSourceMuteChanged(bool value);
    //默认输出设备静音状态改变
    void onSinkMuteChanged(bool value);

private:
    //从参数文件中读取设备检测开启标志，无配置文件，默认开启
    //CheckInputDevice=false,关闭检测
    void initWatcherCofing();
    //初始化获取默认输入/输出设备信息
    void initDeviceWacther();
    //连接相关槽函数
    void initConnections();

private:
    const QString m_serviceName {"com.deepin.daemon.Audio"};
    QScopedPointer<com::deepin::daemon::Audio> m_pAudioInterface;
    QScopedPointer<com::deepin::daemon::audio::Source> m_pDefaultSource;
    QScopedPointer<com::deepin::daemon::audio::Sink> m_pDefaultSink;
    AudioPort m_outAudioPort;
    AudioPort m_inAudioPort;
    double m_outAudioPortVolume = 0.0;
    double m_inAudioPortVolume = 0.0;
    bool m_fNeedDeviceChecker {true};
    bool m_inAudioMute {false};
    bool m_outAudioMute {false};
};

#endif // AudioWatcher_H
