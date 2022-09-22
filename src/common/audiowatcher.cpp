// Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    qInfo() << "\nInput:"
            << "\ncurrent input device path:" << m_pAudioInterface->defaultSource().path()
            << "\ncurrent input active port name:" << m_inAudioPort.name
            << "\ncurrent input active port availability:" << m_inAudioPort.availability
            << "\ncurrent input device name:" << m_pDefaultSource->name()
            << "\ncurrent input port count:" <<m_pDefaultSource->ports().count()
            << "\nOutput:"
            << "\ncurrent output device path:" << m_pAudioInterface->defaultSink().path()
            << "\ncurrent output active port name:" << m_outAudioPort.name
            << "\ncurrent output active port availability:" << m_outAudioPort.availability
            << "\ncurrent output device name:" << m_pDefaultSink->name()
            << "\ncurrent output port count:" <<m_pDefaultSink->ports().count();

    updateDeviceEnabled(m_pAudioInterface->cards(),false);
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
    //检测音频设备状态是否被改变
    QDBusConnection::sessionBus().connect(m_serviceName,
                                          "/com/deepin/daemon/Audio",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          "sa{sv}as",
                                          this,
                                          SLOT(onDeviceEnableState(QDBusMessage))
                                          );
}


/**
 * @brief AudioWatcher::updateDeviceEnabled 更新控制中心中是否更改设备的使能状态
 * 调用时机
 *  1.默认的输入或输出设备改变时需要更新下（不需要发信号）
 *  2.控制中心更改设备的使能状态（需要发信号）
 * @param cardsStr:设备信息
 * @param isEmitSig:是否发信号
 */
void AudioWatcher::updateDeviceEnabled(QString cardsStr,bool isEmitSig)
{
    //所有声卡信息
    QJsonDocument doc = QJsonDocument::fromJson(cardsStr.toUtf8());
    QJsonArray cards = doc.array();
    //遍历声卡，找出当前默认声卡对应的使能状态
    foreach(QJsonValue card , cards){
        //该声卡的所有端口
        QJsonArray ports = card.toObject()["Ports"].toArray();
        //qDebug() << "ports: " << ports;
        //遍历所有端口，找出当前默认设备对应的端口并判断是否可用
        foreach(QJsonValue port , ports){
            QString portName = port.toObject()["Name"].toString();
            //qDebug() << "portName: " << portName << "m_outAudioPort.name: " << m_outAudioPort.name << "m_inAudioPort.name: " <<m_inAudioPort.name ;
            if(portName == m_outAudioPort.name){
                m_outIsEnable = port.toObject()["Enabled"].toBool();
                if(isEmitSig)
                    sigDeviceEnableChanged(Internal,m_outIsEnable);
            }
            if(portName == m_inAudioPort.name){
                m_inIsEnable = port.toObject()["Enabled"].toBool();
                if(isEmitSig)
                    sigDeviceEnableChanged(Micphone,m_inIsEnable);
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

    updateDeviceEnabled(m_pAudioInterface->cards(),false);
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

    updateDeviceEnabled(m_pAudioInterface->cards(),false);
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
 * @brief AudioWatcher::onDeviceEnableState 控制中心改变默认设备使能状态
 * @param msg
 */
void AudioWatcher::onDeviceEnableState(QDBusMessage msg)
{
    QList<QVariant> arguments = msg.arguments();
    //参数固定长度
    if (3 != arguments.count()) {
        qWarning() << "从控制中心获取设备使能状态失败！参数长度不为3！参数长度:" <<arguments.count();
        return;
    }
    //音频dbus服务接口
    QString interfaceName = arguments.at(0).toString();
    if("com.deepin.daemon.Audio" == interfaceName){
        //被改变的属性
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        //qDebug() << "changedProps: " << changedProps;
        foreach (const QString &prop, keys) {
            if (prop == "CardsWithoutUnavailable") {
                qInfo() << "控制中心改变默认设备使能状态！" << prop;
                updateDeviceEnabled(changedProps[prop].toString(),true);
            }
        }
    }
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
        //如果没有端口(m_pDefaultSource->ports().count()=0)，说明音声卡可能是虚拟的
        if ((m_outAudioPort.availability != 1 || !m_fNeedDeviceChecker)&&m_pDefaultSink->ports().count()!=0) {
            device = m_pDefaultSink->name();
            if (!device.isEmpty() && !device.endsWith(".monitor")) {
                device += ".monitor";
            }
        }
    } else {
        //如果没有端口(m_pDefaultSource->ports().count()=0)，说明音声卡可能是虚拟的
        if ((m_inAudioPort.availability != 1 || !m_fNeedDeviceChecker)&&m_pDefaultSource->ports().count()!=0) {
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
 * @brief AudioWatcher::getDeviceEnable
 * @param mode
 * @return 设备是否通过控制中心禁用
 */
bool AudioWatcher::getDeviceEnable(AudioWatcher::AudioMode mode)
{
    return mode != Internal ? m_inIsEnable : m_outIsEnable;
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
