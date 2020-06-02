/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
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
#ifndef VNOTEAUDIODEVICEWATCHER_H
#define VNOTEAUDIODEVICEWATCHER_H

#include <com_deepin_daemon_audio.h>
#include <com_deepin_daemon_audio_source.h>

#include <QThread>

class VNoteAudioDeviceWatcher : public QThread
{
    Q_OBJECT
public:
    explicit VNoteAudioDeviceWatcher(QObject *parent = nullptr);
    virtual ~VNoteAudioDeviceWatcher() override;

    void initDeviceWatcher();
    void initWatcherCofing();
    void exitWatcher();

    enum MicrophoneState {
        NotAvailable,
        VolumeTooLow,   // volume lower than 20%
        Normal,         // volume more than 20%
    };
signals:
    void microphoneAvailableState(int isAvailable);
    void inputSourceChanged(const QString &name);
public slots:
    void onDefaultSourceChanaged(const QDBusObjectPath & value);
    void onCardsChanged(const QString & value);
protected:
    virtual void run() override;

    void initAudioMeter();
    void initConnections();
    void initAvailInputPorts(const QString& cards);
    bool isMicrophoneAvail(const QString& activePort) const;

private:
    const QString m_serviceName {"com.deepin.daemon.Audio"};

    QScopedPointer<com::deepin::daemon::Audio> m_audioInterface;
    QScopedPointer<com::deepin::daemon::audio::Source> m_defaultSource;

    //Reference the state to AudioPort
    enum PortState {
        Unkown = 0,
        UnAvailable,
        Available,
    };

    typedef QString PortID;

    struct Port {
        PortID  portId;
        QString portName;
        QString cardName;
        int     available {UnAvailable};
        int     cardId;
        bool    isActive {false};

        bool isInputPort() const;
        bool isLoopback() const;
    };

    friend QDebug& operator << (QDebug& out, const Port &port);

    MicrophoneState m_microphoneState {NotAvailable};
    volatile bool m_quitWatcher {false};

    //All available input ports.except loopback port.
    QMap<PortID, Port> m_availableInputPorts;

    bool m_fNeedDeviceChecker {true};
};

#endif // VNOTEAUDIODEVICEWATCHER_H
