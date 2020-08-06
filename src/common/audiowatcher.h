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
    enum AudioMode{Internal,Micphone};
    Q_ENUM(AudioMode)
    explicit AudioWatcher(QObject *parent = nullptr);
    QString getDeviceName(AudioMode mode);
    double getVolume(AudioMode mode);
    bool getMute(AudioMode mode);
signals:
    void sigVolumeChange(AudioMode mode);
    void sigDeviceChange(AudioMode mode);
    void sigMuteChanged(AudioMode mode);
protected slots:
    void onDefaultSourceChanaged(const QDBusObjectPath &value);
    void onDefaultSinkChanaged(const QDBusObjectPath &value);
    void onSourceVolumeChanged(double d);
    void onSinkVolumeChanged(double d);
    void onSourceMuteChanged(bool  value);
    void onSinkMuteChanged(bool  value);

private:
    void initWatcherCofing();
    void initDeviceWacther();
    void initConnections();
private:
    const QString m_serviceName {"com.deepin.daemon.Audio"};
    QScopedPointer<com::deepin::daemon::Audio> m_pAudioInterface;
    QScopedPointer<com::deepin::daemon::audio::Source> m_pDefaultSource;
    QScopedPointer<com::deepin::daemon::audio::Sink> m_pDefaultSink;
    AudioPort   m_outAudioPort;
    AudioPort   m_inAudioPort;
    double m_outAudioPortVolume = 0.0;
    double m_inAudioPortVolume = 0.0;
    bool m_fNeedDeviceChecker {true};
    bool m_inAudioMute {false};
    bool m_outAudioMute {false};
};

#endif // AudioWatcher_H
