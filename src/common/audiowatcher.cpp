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
    // 初始化音频服务Dus接口
    m_audioDBusInterface = new QDBusInterface(AudioService,
                                              AudioPath,
                                              AudioInterface,
                                              QDBusConnection::sessionBus());
    //检查是否存在音频服务
    if (m_audioDBusInterface->isValid()) {
        qInfo() << "音频服务初始化成功！音频服务： " << AudioService << " 地址：" << AudioPath << " 接口：" << AudioInterface;
        m_defaultSourcePath = m_audioDBusInterface->property("DefaultSource").value<QDBusObjectPath>().path();
        initDefaultSourceDBusInterface();
        m_defaultSinkPath = m_audioDBusInterface->property("DefaultSink").value<QDBusObjectPath>().path();
        initDefaultSinkDBusInterface();
        qInfo() << "current input active port name:" << m_inAudioPort.name
                << "\ncurrent input active port availability:" << m_inAudioPort.availability
                << "\ncurrent input device name:" << defaultSourceName()
                << "\ncurrent output active port name:" << m_outAudioPort.name
                << "\ncurrent output active port availability:" << m_outAudioPort.availability
                << "\ncurrent output device name:" << defaultSinkName();
        //监听音频服务的属性改变
        QDBusConnection::sessionBus().connect(AudioService,
                                              AudioPath,
                                              PropertiesInterface,
                                              "PropertiesChanged",
                                              "sa{sv}as",
                                              this,
                                              SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                             );
    } else {
        qWarning() << "初始化失败！音频服务 (" << AudioService << ") 不存在";
    }
}

/**
 * @brief AudioWatcher::initConnections
 */
void AudioWatcher::initConnections()
{
    //监听音频服务的属性改变
    QDBusConnection::sessionBus().connect(AudioService,
                                          AudioPath,
                                          PropertiesInterface,
                                          "PropertiesChanged",
                                          "sa{sv}as",
                                          this,
                                          SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                         );
}
//初始化音频dbus服务默认输入源的接口
void AudioWatcher::initDefaultSourceDBusInterface()
{
    if (m_defaultSourceDBusInterface) {
        delete m_defaultSourceDBusInterface;
        m_defaultSourceDBusInterface = nullptr;
    }
    // 初始化Dus接口
    m_defaultSourceDBusInterface = new QDBusInterface(AudioService,
                                                      m_defaultSourcePath,
                                                      SourceInterface,
                                                      QDBusConnection::sessionBus());
    if (m_defaultSourceDBusInterface->isValid()) {
        qInfo() << "默认音频输入源初始化成功！音频服务： " << AudioService << " 默认输入源地址" << m_defaultSourcePath << " 默认输入源接口：" << SourceInterface;
        m_inAudioPortVolume = defaultSourceVolume();
        m_inAudioPort = defaultSourceActivePort();
        m_inAudioMute = defaultSourceMute();
        //监听音频服务输入源的属性改变
        QDBusConnection::sessionBus().connect(AudioService,
                                              m_defaultSourcePath,
                                              PropertiesInterface,
                                              "PropertiesChanged",
                                              "sa{sv}as",
                                              this,
                                              SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                             );
    } else {
        qWarning() << "默认音频输入源初始化失败！默认输入源地址 (" << m_defaultSourcePath << ") 不存在";
    }
}

//初始化音频dbus服务默认输出源的接口
void AudioWatcher::initDefaultSinkDBusInterface()
{
    if (m_defaultSinkDBusInterface) {
        delete m_defaultSinkDBusInterface;
        m_defaultSinkDBusInterface = nullptr;
    }
    // 初始化Dus接口
    m_defaultSinkDBusInterface = new QDBusInterface(AudioService,
                                                    m_defaultSinkPath,
                                                    SinkInterface,
                                                    QDBusConnection::sessionBus());
    if (m_defaultSinkDBusInterface->isValid()) {
        qInfo() << "默认音频输出源初始化成功！音频服务： " << AudioService << " 默认输出源地址" << m_defaultSinkPath << " 默认输出源接口：" << SinkInterface;
        m_outAudioPortVolume = defaultSinkVolume();
        m_outAudioPort = defaultSinkActivePort();
        m_outAudioMute = defaultSinkMute();

        //监听音频服务输出源的属性改变
        QDBusConnection::sessionBus().connect(AudioService,
                                              m_defaultSinkPath,
                                              PropertiesInterface,
                                              "PropertiesChanged",
                                              "sa{sv}as",
                                              this,
                                              SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                             );
    } else {
        qWarning() << "默认音频输出源初始化失败！默认输出源地址 (" << m_defaultSinkPath << ") 不存在";
    }
}

//音频dbus服务属性改变时触发(注:此槽函数链接的dbus属性改变信号发出时机，是dbus属性已经改变之后才会发出)
void AudioWatcher::onDBusAudioPropertyChanged(QDBusMessage msg)
{
    QList<QVariant> arguments = msg.arguments();
    //qInfo() << "arguments" << arguments;
    //参数固定长度
    if (3 != arguments.count()) {
        qWarning() << "参数长度不为3！ 参数: " << arguments;
        return;
    }
    QString interfaceName = msg.arguments().at(0).toString();
    if (interfaceName == AudioInterface) {
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            //qInfo() << "property: " << prop << changedProps[prop];
            if (prop == QStringLiteral("DefaultSource")) {
                //默认输入源地址改变
                const QDBusObjectPath &defaultSourcePath = qvariant_cast<QDBusObjectPath>(changedProps[prop]);
                onDefaultSourceChanaged(defaultSourcePath);
            } else if (prop == QStringLiteral("DefaultSink")) {
                //默认输出源地址改变
                const QDBusObjectPath &defaultSinkePath = qvariant_cast<QDBusObjectPath>(changedProps[prop]);
                onDefaultSinkChanaged(defaultSinkePath);
            }
        }
    } else if (interfaceName == SourceInterface) {
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            qInfo() << "property: " << prop << changedProps[prop];
            if (prop == QStringLiteral("Volume")) {
                //默认输入源音量改变
                double inAudioPortVolume = qvariant_cast<double>(changedProps[prop]);
                if (abs(m_inAudioPortVolume - inAudioPortVolume) <= 0.000001) {
                    qInfo() << "默认输入源音量改变:" <<  m_inAudioPortVolume << " To " << inAudioPortVolume;
                    onSourceVolumeChanged(inAudioPortVolume);
                }
            } else if (prop == QStringLiteral("Mute")) {
                //默认输入源静音状态
                const bool inAudioMute = qvariant_cast<bool>(changedProps[prop]);
                qInfo() << ">>>>> inAudioMute:" <<  inAudioMute << " defaultSourceMute: " << defaultSourceMute();
                if (m_inAudioMute != inAudioMute) {
                    qInfo() << "默认输入源静音状态改变:" <<  m_inAudioMute << " To " << inAudioMute;
                    onSourceMuteChanged(inAudioMute);
                }
            } else if (prop == QStringLiteral("ActivePort")) {
                qInfo() << "property: " << prop << changedProps[prop];
                //默认输入源活跃端口
                //const AudioPort inAudioPort = qvariant_cast<AudioPort>(changedProps[prop]);
                //由于执行到这里时，此dbus属性已经被改变了，可以直接通过此方式进行获取
                const AudioPort inAudioPort = defaultSourceActivePort();
                if (m_inAudioPort != inAudioPort) {
                    qInfo() << "默认输入源活跃端口改变:" <<  m_inAudioPort << " To " << inAudioPort;
                    onDefaultSourceActivePortChanged(inAudioPort);
                }
            }
        }
    } else if (interfaceName == SinkInterface) {
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            //qInfo() << "property: " << prop << changedProps[prop];
            if (prop == QStringLiteral("Volume")) {
                //默认输出源音量改变
                double outAudioPortVolume = qvariant_cast<double>(changedProps[prop]);
                if (abs(m_outAudioPortVolume - outAudioPortVolume) <= 0.000001) {
                    qInfo() << "默认输出源音量改变:" <<  m_outAudioPortVolume << " To " << outAudioPortVolume;
                    onSourceVolumeChanged(outAudioPortVolume);
                }
            } else if (prop == QStringLiteral("Mute")) {
                //默认输出源静音状态
                const bool outAudioMute = qvariant_cast<bool>(changedProps[prop]);
                if (m_outAudioMute != outAudioMute) {
                    qInfo() << "默认输出源静音状态:" <<  m_outAudioMute << " To " << outAudioMute;
                    onSourceMuteChanged(outAudioMute);
                }
            } else if (prop == QStringLiteral("ActivePort")) {
                //默认输出源活跃端口
                //const AudioPort outAudioPort = qvariant_cast<AudioPort>(changedProps[prop]);
                //由于执行到这里时，此dbus属性已经被改变了，可以直接通过此方式进行获取
                const AudioPort outAudioPort = defaultSinkActivePort();
                if (m_outAudioPort != outAudioPort) {
                    qInfo() << "默认输出源活跃端口:" <<  m_outAudioPort << " To " << outAudioPort;
                    onDefaultSinkActivePortChanged(outAudioPort);
                }
            }
        }
    }
}

/**
 * @brief AudioWatcher::onSourceVolumeChanged
 * @param value 当前音量值
 */
void AudioWatcher::onSourceVolumeChanged(double value)
{
    m_inAudioPortVolume = value;
    AudioPort activePort = defaultSourceActivePort();
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
    AudioPort activePort = defaultSinkActivePort();
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
void AudioWatcher::onDefaultSourceChanaged(const QDBusObjectPath &defaultSourcePath)
{
    if (m_defaultSourcePath != defaultSourcePath.path()) {
        qInfo() << "默认输入源地址改变:" <<  m_defaultSourcePath << " To " << defaultSourcePath.path();
        QDBusConnection::sessionBus().disconnect(AudioService,
                                                 m_defaultSourcePath,
                                                 PropertiesInterface,
                                                 "PropertiesChanged",
                                                 "sa{sv}as",
                                                 this,
                                                 SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                                );
        m_defaultSourcePath = defaultSourcePath.path();
        //重新初始化默认输入源接口
        initDefaultSourceDBusInterface();
        emit sigDeviceChange(Micphone);
    }
}

/**
 * @brief AudioWatcher::onDefaultSinkChanaged
 * @param value 设备dbus路径
 */
void AudioWatcher::onDefaultSinkChanaged(const QDBusObjectPath &defaultSinkePath)
{
    if (m_defaultSinkPath != defaultSinkePath.path()) {
        qInfo() << "默认输出源地址改变:" <<  m_defaultSinkPath << " To " << defaultSinkePath.path();
        QDBusConnection::sessionBus().disconnect(AudioService,
                                                 m_defaultSinkPath,
                                                 PropertiesInterface,
                                                 "PropertiesChanged",
                                                 "sa{sv}as",
                                                 this,
                                                 SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                                );
        m_defaultSinkPath = defaultSinkePath.path();
        //重新初始化默认输出源接口
        initDefaultSinkDBusInterface();
        emit sigDeviceChange(Internal);
    }
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
            device = defaultSinkName();
            if (!device.isEmpty() && !device.endsWith(".monitor")) {
                device += ".monitor";
            }
        }
    } else {
        if (m_inAudioPort.availability != 1 || !m_fNeedDeviceChecker) {
            device = defaultSourceName();
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

//音频dbus服务默认输入源的活跃端口
AudioPort AudioWatcher::defaultSourceActivePort()
{
    AudioPort port;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSourcePath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
        //qInfo() << "音频服务： "<< AudioService <<" 默认输入源地址"<< m_defaultSourcePath << " 属性接口："<< PropertiesInterface;
        QDBusReply<QDBusVariant> reply = inter->call("Get", SourceInterface, "ActivePort");
        reply.value().variant().value<QDBusArgument>() >> port;
        //qInfo() << "ActivePort:" << port;
    } else {
        qWarning() << "默认输入源地址 (" << m_defaultSourcePath << ") 不存在";
    }
    delete inter;
    inter = nullptr;
    return port;
}

//音频dbus服务默认输入源的音量
double AudioWatcher::defaultSourceVolume()
{
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        return m_defaultSourceDBusInterface->property("Volume").value<double>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return 0.0;
    }
}

//音频dbus服务默认输入源是否静音
bool AudioWatcher::defaultSourceMute()
{
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        return m_defaultSourceDBusInterface->property("Mute").value<bool>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return true;
    }
}

//音频dbus服务默认输入源名称
QString AudioWatcher::defaultSourceName()
{
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        return m_defaultSourceDBusInterface->property("Name").value<QString>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return "";
    }
}

//音频dbus服务默认输出源的活跃端口
AudioPort AudioWatcher::defaultSinkActivePort()
{
    AudioPort port;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSinkPath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
        //qInfo() << "音频服务： "<< AudioService <<" 默认输出源地址"<< m_defaultSinkPath << " 属性接口："<< PropertiesInterface;
        QDBusReply<QDBusVariant> reply = inter->call("Get", SourceInterface, "ActivePort");
        reply.value().variant().value<QDBusArgument>() >> port;
        //qInfo() << "ActivePort:" << port;
    } else {
        qWarning() << "默认输出源地址 (" << m_defaultSinkPath << ") 不存在";
    }
    delete inter;
    inter = nullptr;
    return port;
}

//音频dbus服务默认输出源的音量
double AudioWatcher::defaultSinkVolume()
{
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        return m_defaultSinkDBusInterface->property("Volume").value<double>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return 0.0;
    }
}

//音频dbus服务默认输出源是否静音
bool AudioWatcher::defaultSinkMute()
{
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        return m_defaultSinkDBusInterface->property("Mute").value<bool>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return true;
    }
}

//音频dbus服务默认输出源名称
QString AudioWatcher::defaultSinkName()
{
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        return m_defaultSinkDBusInterface->property("Name").value<QString>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return "";
    }
}
