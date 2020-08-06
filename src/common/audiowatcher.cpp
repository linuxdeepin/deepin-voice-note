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

#include "audiowatcher.h"
#include "globaldef.h"

AudioWatcher::AudioWatcher(QObject *parent)
    : QObject(parent)
{
    initWatcherCofing();
    initDeviceWacther();
    initConnections();
}


void AudioWatcher::initDeviceWacther()
{
    m_pAudioInterface.reset( new com::deepin::daemon::Audio(
                                 m_serviceName,
                                 "/com/deepin/daemon/Audio",
                                 QDBusConnection::sessionBus(),this)
                             );

    m_pDefaultSink.reset(new com::deepin::daemon::audio::Sink (
                             m_serviceName,
                             m_pAudioInterface->defaultSink().path(),
                             QDBusConnection::sessionBus(),this)
                         );

    m_pDefaultSource.reset(new com::deepin::daemon::audio::Source (
                               m_serviceName,
                               m_pAudioInterface->defaultSource().path(),
                               QDBusConnection::sessionBus(),this)
                           );

    m_inAudioPortVolume = m_pDefaultSource->volume();
    m_inAudioPort = m_pDefaultSource->activePort();
    m_inAudioMute = m_pDefaultSource->mute();
    m_outAudioPortVolume = m_pDefaultSink->volume();
    m_outAudioPort = m_pDefaultSink->activePort();
    m_outAudioMute = m_pDefaultSink->mute();
}

void AudioWatcher::initConnections()
{
    connect(m_pAudioInterface.get(), SIGNAL(DefaultSourceChanged(const QDBusObjectPath &)),
            this, SLOT(onDefaultSourceChanaged(const QDBusObjectPath &)));

    connect(m_pAudioInterface.get(), SIGNAL(DefaultSinkChanged(const QDBusObjectPath &)),
            this, SLOT(onDefaultSinkChanaged(const QDBusObjectPath &)));

    connect(m_pDefaultSource.get(), SIGNAL(VolumeChanged(double)),this, SLOT(onSourceVolumeChanged(double)));
    connect(m_pDefaultSource.get(), SIGNAL(MuteChanged(bool)),this, SLOT(onSourceMuteChanged(bool)));

    connect(m_pDefaultSink.get(), SIGNAL(VolumeChanged(double)),this, SLOT(onSinkVolumeChanged(double)));
    connect(m_pDefaultSink.get(), SIGNAL(MuteChanged(bool)),this, SLOT(onSinkMuteChanged(bool)));
}

void AudioWatcher::onSourceVolumeChanged(double value)
{
    m_inAudioPortVolume = value;
    AudioPort activePort = m_pDefaultSource->activePort();
    if(m_inAudioPort.name == activePort.name){
        emit sigVolumeChange(Micphone);
    }else {
        qDebug() << "last input active port:" << m_inAudioPort.name;
        qDebug() << "current input active port:" << activePort.name;
        m_inAudioPort = activePort;
    }
}

void AudioWatcher::onSinkVolumeChanged(double value)
{
    m_outAudioPortVolume = value;
    AudioPort activePort = m_pDefaultSink->activePort();
    if(m_outAudioPort.name == activePort.name){
        emit sigVolumeChange(Internal);
    }else {
        qDebug() << "last output active port:" << m_outAudioPort.name;
        qDebug() << "current output active port:" << activePort.name;
        m_outAudioPort = activePort;
    }
}


void AudioWatcher::onDefaultSourceChanaged(const QDBusObjectPath &value)
{
    qDebug() << "last input active port:" << m_inAudioPort.name;
    qDebug() << "last input:" << m_pDefaultSource->name();

    m_pDefaultSource.reset(new com::deepin::daemon::audio::Source (
                               m_serviceName,
                               value.path(),
                               QDBusConnection::sessionBus(),this)
                           );
    connect(m_pDefaultSource.get(), SIGNAL(VolumeChanged(double)),this, SLOT(onSourceVolumeChanged(double)));
    connect(m_pDefaultSource.get(), SIGNAL(MuteChanged(bool)),this, SLOT(onSourceMuteChanged(bool)));
    m_inAudioPortVolume = m_pDefaultSource->volume();
    m_inAudioPort = m_pDefaultSource->activePort();
    m_inAudioMute = m_pDefaultSource->mute();
    emit sigDeviceChange(Micphone);

    qDebug() << "current input active port:" << m_inAudioPort.name;
    qDebug() << "current input:" << m_pDefaultSource->name();
}

void AudioWatcher::onDefaultSinkChanaged(const QDBusObjectPath &value)
{
    qDebug() << "last output active port:" << m_outAudioPort.name;
    qDebug() << "last output:" << m_pDefaultSink->name();

    m_pDefaultSink.reset(new com::deepin::daemon::audio::Sink (
                             m_serviceName,
                             value.path(),
                             QDBusConnection::sessionBus(),this)
                         );
    connect(m_pDefaultSink.get(), SIGNAL(VolumeChanged(double)),this, SLOT(onSinkVolumeChanged(double)));
    connect(m_pDefaultSink.get(), SIGNAL(MuteChanged(bool)),this, SLOT(onSinkMuteChanged(bool)));
    m_outAudioPortVolume = m_pDefaultSink->volume();
    m_outAudioPort = m_pDefaultSink->activePort();
    m_outAudioMute = m_pDefaultSink->mute();
    emit sigDeviceChange(Internal);

    qDebug() << "current output active port:" << m_outAudioPort.name;
    qDebug() << "current output:" << m_pDefaultSink->name();
}

void AudioWatcher::onSinkMuteChanged(bool value)
{
    m_outAudioMute = value;
    emit sigMuteChanged(Internal);
}

void AudioWatcher::onSourceMuteChanged(bool  value)
{
    m_inAudioMute = value;
    emit sigMuteChanged(Micphone);
}

QString AudioWatcher::getDeviceName(AudioMode mode)
{
    QString device = "";
    if(mode == Internal){
        device = m_pDefaultSink->name();
        if(!device.isEmpty()){
            device += ".monitor";
        }
    }else {
        device = m_pDefaultSource->name();
        if(device.endsWith(".monitor") && m_fNeedDeviceChecker){
            device.clear();
        }
    }
    return  device;
}

double AudioWatcher::getVolume(AudioMode mode)
{
    return  mode != Internal ? m_inAudioPortVolume : m_outAudioPortVolume;
}

bool AudioWatcher::getMute(AudioMode mode)
{
    return  mode != Internal ? m_inAudioMute : m_outAudioMute;
}

void AudioWatcher::initWatcherCofing()
{
    //TODO:
    //    Both App & Backend may be integrate the
    //config file,and /etc's priority is higher than
    //app's configuration.
    QStringList watcherConfigFilePaths = {
        "/etc/",
        "/usr/share/",
    };

    QString configFileBasePath = QString(DEEPIN_VOICE_NOTE)+QString("/")
            + QString(DEEPIN_VOICE_NOTE)+QString(".conf");

    for (auto it : watcherConfigFilePaths) {
        QString configFileName(it+configFileBasePath);

        QFileInfo watcherConfig(configFileName);

        if (watcherConfig.exists()) {
            QSettings  watcherSettings(configFileName, QSettings::Format::IniFormat);

            //Default need device watcher
            QVariant varValue = watcherSettings.value("Audio/CheckInputDevice");

            if (!varValue.isNull()) {
                m_fNeedDeviceChecker = varValue.toBool();
                qInfo() << "Device watcher config:Path=" << configFileName
                        << " CheckInputDevice->" << m_fNeedDeviceChecker;
                break;
            } else {
                qInfo() << "Device watcher config:Path=" << configFileName
                        << " [Audio/CheckInputDevice] doesn't exist.";
            }
        } else {
            qInfo() << "Device watcher config don't exist:" << configFileName;
        }
    }

    qInfo() << "Device watcher config:CheckInputDevice->" << m_fNeedDeviceChecker;
}
