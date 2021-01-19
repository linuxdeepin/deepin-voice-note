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

#include <DLog>
/**
 * @brief AudioWatcher::AudioWatcher
 * @param parent 父类
 */
AudioWatcher::AudioWatcher(QObject *parent)
    : QObject(parent)
{
    initWatcherCofing();
    initDeviceWacther();
    initConnections();
}

/**
 * @brief AudioWatcher::initDeviceWacther
 */
void AudioWatcher::initDeviceWacther()
{
    m_pAudioInterface.reset(new com::deepin::daemon::Audio(
        m_serviceName,
        "/com/deepin/daemon/Audio",
        QDBusConnection::sessionBus(), this));

    m_pDefaultSink.reset(new com::deepin::daemon::audio::Sink(
        m_serviceName,
        m_pAudioInterface->defaultSink().path(),
        QDBusConnection::sessionBus(), this));

    m_pDefaultSource.reset(new com::deepin::daemon::audio::Source(
        m_serviceName,
        m_pAudioInterface->defaultSource().path(),
        QDBusConnection::sessionBus(), this));

    m_inAudioPortVolume = m_pDefaultSource->volume();
    m_inAudioPort = m_pDefaultSource->activePort();
    m_inAudioMute = m_pDefaultSource->mute();
    m_outAudioPortVolume = m_pDefaultSink->volume();
    m_outAudioPort = m_pDefaultSink->activePort();
    m_outAudioMute = m_pDefaultSink->mute();

    qInfo() << "current input active port name:" << m_inAudioPort.name
            << "\ncurrent input active port availability:" << m_inAudioPort.availability
            << "\ncurrent input device name:" << m_pDefaultSource->name()
            << "\ncurrent output active port name:" << m_outAudioPort.name
            << "\ncurrent output active port availability:" << m_outAudioPort.availability
            << "\ncurrent output device name:" << m_pDefaultSink->name();
}

/**
 * @brief AudioWatcher::initConnections
 */
void AudioWatcher::initConnections()
{
    connect(m_pAudioInterface.get(), SIGNAL(DefaultSourceChanged(const QDBusObjectPath &)),
            this, SLOT(onDefaultSourceChanaged(const QDBusObjectPath &)));

    connect(m_pAudioInterface.get(), SIGNAL(DefaultSinkChanged(const QDBusObjectPath &)),
            this, SLOT(onDefaultSinkChanaged(const QDBusObjectPath &)));

    connect(m_pDefaultSource.get(), SIGNAL(VolumeChanged(double)), this, SLOT(onSourceVolumeChanged(double)));
    connect(m_pDefaultSource.get(), SIGNAL(MuteChanged(bool)), this, SLOT(onSourceMuteChanged(bool)));
    connect(m_pDefaultSource.get(), SIGNAL(ActivePortChanged(AudioPort)),
            this, SLOT(onDefaultSourceActivePortChanged(AudioPort)));

    connect(m_pDefaultSink.get(), SIGNAL(VolumeChanged(double)), this, SLOT(onSinkVolumeChanged(double)));
    connect(m_pDefaultSink.get(), SIGNAL(MuteChanged(bool)), this, SLOT(onSinkMuteChanged(bool)));
    connect(m_pDefaultSink.get(), SIGNAL(ActivePortChanged(AudioPort)),
            this, SLOT(onDefaultSinkActivePortChanged(AudioPort)));
}

/**
 * @brief AudioWatcher::onSourceVolumeChanged
 * @param value 当前音量值
 */
void AudioWatcher::onSourceVolumeChanged(double value)
{
    m_inAudioPortVolume = value;
    AudioPort activePort = m_pDefaultSource->activePort();
    if (m_inAudioPort.name == activePort.name) {
        emit sigVolumeChange(Micphone);
    } else {
        qInfo() << "last input active port:" << m_inAudioPort.name;
        qInfo() << "current input active port:" << activePort.name;
        m_inAudioPort = activePort;
    }
}

/**
 * @brief AudioWatcher::onSinkVolumeChanged
 * @param value 当前音量值
 */
void AudioWatcher::onSinkVolumeChanged(double value)
{
    m_outAudioPortVolume = value;
    AudioPort activePort = m_pDefaultSink->activePort();
    if (m_outAudioPort.name == activePort.name) {
        emit sigVolumeChange(Internal);
    } else {
        qInfo() << "last output active port:" << m_outAudioPort.name;
        qInfo() << "current output active port:" << activePort.name;
        m_outAudioPort = activePort;
    }
}

/**
 * @brief AudioWatcher::onDefaultSourceChanaged
 * @param value 设备dbus路径
 */
void AudioWatcher::onDefaultSourceChanaged(const QDBusObjectPath &value)
{
    qInfo() << "Input device change from:"
            << "\nactive port:" << m_inAudioPort.name
            << "\ndevice name:" << m_pDefaultSource->name();

    m_pDefaultSource.reset(new com::deepin::daemon::audio::Source(
        m_serviceName,
        value.path(),
        QDBusConnection::sessionBus(), this));
    connect(m_pDefaultSource.get(), SIGNAL(VolumeChanged(double)), this, SLOT(onSourceVolumeChanged(double)));
    connect(m_pDefaultSource.get(), SIGNAL(MuteChanged(bool)), this, SLOT(onSourceMuteChanged(bool)));
    connect(m_pDefaultSource.get(), SIGNAL(ActivePortChanged(AudioPort)),
            this, SLOT(onDefaultSourceActivePortChanged(AudioPort)));
    m_inAudioPortVolume = m_pDefaultSource->volume();
    m_inAudioPort = m_pDefaultSource->activePort();
    m_inAudioMute = m_pDefaultSource->mute();

    qInfo() << "\nTo:"
            << "\nactive port:" << m_inAudioPort.name << ";" << m_inAudioPort.availability
            << "\ndevice name:" << m_pDefaultSource->name();

    emit sigDeviceChange(Micphone);
}

/**
 * @brief AudioWatcher::onDefaultSinkChanaged
 * @param value 设备dbus路径
 */
void AudioWatcher::onDefaultSinkChanaged(const QDBusObjectPath &value)
{
    qInfo() << "Output device change from:"
            << "\nactive port:" << m_outAudioPort.name << ";" << m_outAudioPort.availability
            << "\ndevice name:" << m_pDefaultSink->name();

    m_pDefaultSink.reset(new com::deepin::daemon::audio::Sink(
        m_serviceName,
        value.path(),
        QDBusConnection::sessionBus(), this));
    connect(m_pDefaultSink.get(), SIGNAL(VolumeChanged(double)), this, SLOT(onSinkVolumeChanged(double)));
    connect(m_pDefaultSink.get(), SIGNAL(MuteChanged(bool)), this, SLOT(onSinkMuteChanged(bool)));
    connect(m_pDefaultSink.get(), SIGNAL(ActivePortChanged(AudioPort)),
            this, SLOT(onDefaultSinkActivePortChanged(AudioPort)));
    m_outAudioPortVolume = m_pDefaultSink->volume();
    m_outAudioPort = m_pDefaultSink->activePort();
    m_outAudioMute = m_pDefaultSink->mute();

    qInfo() << "\nTo:"
            << "\nactive port:" << m_outAudioPort.name << ";" << m_outAudioPort.availability
            << "\ndevice name:" << m_pDefaultSink->name();

    emit sigDeviceChange(Internal);
}

/**
 * @brief AudioWatcher::onDefaultSinkActivePortChanged
 * @param value 当前活动端口
 */
void AudioWatcher::onDefaultSinkActivePortChanged(AudioPort value)
{
    qInfo() << "output active port change from:"
            << "\nPort Name:" << m_outAudioPort.name
            << "\nPort Availability:" << m_outAudioPort.availability
            << "\nto:"
            << "\nPort Name:" << value.name
            << "\nPort Availability:" << value.availability;
    m_outAudioPort = value;
    emit sigDeviceChange(Internal);
}

/**
 * @brief AudioWatcher::onDefaultSourceActivePortChanged
 * @param value 当前活动端口
 */
void AudioWatcher::onDefaultSourceActivePortChanged(AudioPort value)
{
    qInfo() << "input active port change from:"
            << "\nPort Name:" << m_inAudioPort.name
            << "\nPort Availability:" << m_inAudioPort.availability
            << "\nto:"
            << "\nPort Name:" << value.name
            << "\nPort Availability:" << value.availability;
    m_inAudioPort = value;
    emit sigDeviceChange(Micphone);
}

/**
 * @brief AudioWatcher::onSinkMuteChanged
 * @param value 当前静音状态
 */
void AudioWatcher::onSinkMuteChanged(bool value)
{
    m_outAudioMute = value;
    emit sigMuteChanged(Internal);
}

/**
 * @brief AudioWatcher::onSourceMuteChanged
 * @param value 当前静音状态
 */
void AudioWatcher::onSourceMuteChanged(bool value)
{
    m_inAudioMute = value;
    emit sigMuteChanged(Micphone);
}

/**
 * @brief AudioWatcher::getDeviceName
 * @param mode
 * @return 设备名称
 */
QString AudioWatcher::getDeviceName(AudioMode mode)
{
    QString device = "";
    if (mode == Internal) {
        if (m_outAudioPort.availability != 1 || !m_fNeedDeviceChecker) {
            device = m_pDefaultSink->name();
            if (!device.isEmpty() && !device.endsWith(".monitor")) {
                device += ".monitor";
            }
        }
    } else {
        if (m_inAudioPort.availability != 1 || !m_fNeedDeviceChecker) {
            device = m_pDefaultSource->name();
            if (device.endsWith(".monitor") && m_fNeedDeviceChecker) {
                device.clear();
            }
        }
    }
    return device;
}

/**
 * @brief AudioWatcher::getVolume
 * @param mode
 * @return 设备音量
 */
double AudioWatcher::getVolume(AudioMode mode)
{
    return mode != Internal ? m_inAudioPortVolume : m_outAudioPortVolume;
}

/**
 * @brief AudioWatcher::getMute
 * @param mode
 * @return 设备静音状态
 */
bool AudioWatcher::getMute(AudioMode mode)
{
    return mode != Internal ? m_inAudioMute : m_outAudioMute;
}

/**
 * @brief AudioWatcher::initWatcherCofing
 */
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

    QString configFileBasePath = QString(DEEPIN_VOICE_NOTE) + QString("/")
                                 + QString(DEEPIN_VOICE_NOTE) + QString(".conf");

    for (auto it : watcherConfigFilePaths) {
        QString configFileName(it + configFileBasePath);

        QFileInfo watcherConfig(configFileName);

        if (watcherConfig.exists()) {
            QSettings watcherSettings(configFileName, QSettings::Format::IniFormat);

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
