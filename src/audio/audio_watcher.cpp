#include "audio_watcher.h"

AudioWatcher::AudioWatcher(QObject *parent)
    : QObject(parent)
{
    qDebug() << "Initializing AudioWatcher...";
    m_isVirtualMachineHw = isVirtualMachineHw();
    initWatcherCofing();
    initDeviceWacther();
    initConnections();

    // 初始获取 ReduceNoise 状态 (使用更简洁的property()方法)
    if (m_audioDBusInterface->isValid()) {
        m_isReduceNoise = m_audioDBusInterface->property("ReduceNoise").value<bool>();
        qInfo() << "Initial ReduceNoise state:" << m_isReduceNoise;
    } else {
        qWarning() << "Failed to get initial ReduceNoise state: m_audioDBusInterface is invalid.";
    }

    qDebug() << "AudioWatcher initialization completed";
}

/**
 * @brief AudioWatcher::initDeviceWacther
 */
void AudioWatcher::initDeviceWacther()
{
    qDebug() << "Initializing audio device watcher...";
    m_audioDBusInterface = new QDBusInterface(AudioService,
                                              AudioPath,
                                              AudioInterface,
                                              QDBusConnection::sessionBus());
    if (m_audioDBusInterface->isValid()) {
        qInfo() << "m_audioDBusInterface is valid";
        m_defaultSourcePath = m_audioDBusInterface->property("DefaultSource").value<QDBusObjectPath>().path();
        initDefaultSourceDBusInterface();
        m_defaultSinkPath = m_audioDBusInterface->property("DefaultSink").value<QDBusObjectPath>().path();
        initDefaultSinkDBusInterface();
        qInfo() << "\n**************** cunrrent default input or output active port **********************"
                << "\ncurrent input active port name:" << m_inAudioPort.name
                << "\ncurrent input active port availability(0 for Unknown, 1 for Not Available, 2 for Available.):" << m_inAudioPort.availability
                << "\ncurrent input device name:" << defaultSourceName()
                << "\ncurrent output active port name:" << m_outAudioPort.name
                << "\ncurrent output active port availability(0 for Unknown, 1 for Not Available, 2 for Available.):" << m_outAudioPort.availability
                << "\ncurrent output device name:" << defaultSinkName()
                << "\n***********************************************************************************";
        QDBusConnection::sessionBus().connect(AudioService,
                                              AudioPath,
                                              PropertiesInterface,
                                              "PropertiesChanged",
                                              "sa{sv}as",
                                              this,
                                              SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                              );
        updateDeviceEnabled(m_audioDBusInterface->property("CardsWithoutUnavailable").value<QString>(), false);
    } else {
        qCritical() << "Failed to initialize audio service. Audio service (" << AudioService << ") does not exist";
    }
}

/**
 * @brief AudioWatcher::initConnections
 */
void AudioWatcher::initConnections()
{
    QDBusConnection::sessionBus().connect(AudioService,
                                          AudioPath,
                                          PropertiesInterface,
                                          "PropertiesChanged",
                                          "sa{sv}as",
                                          this,
                                          SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                          );

}

bool AudioWatcher::isVirtualMachineHw()
{
    qInfo() << "Checking if virtual machine hardware";
    bool isVirtualMachine = false;
    QString reslut = vnSystemInfo();
    if (reslut.isEmpty()) {
        qInfo() << "reslut is empty";
        return isVirtualMachine;
    }
    if (!reslut.contains("none")) {
        qInfo() << "reslut contains none";
        isVirtualMachine = true;
    }
    return isVirtualMachine;
}

QString AudioWatcher::vnSystemInfo()
{
    qInfo() << "Getting system information";
    QProcess process;
    process.start("systemd-detect-virt");
    process.waitForFinished();
    process.waitForReadyRead();
    QByteArray tempArray =  process.readAllStandardOutput();
    QString reslut = QString(QLatin1String( tempArray.data()));
    qDebug() << "reslut: " << reslut;
    process.close();
    qInfo() << "System information:" << reslut;
    return reslut;
}

void AudioWatcher::updateDeviceEnabled(const QString cardsStr, bool isEmitSig)
{
    qInfo() << "Updating audio device status";
    QJsonDocument doc = QJsonDocument::fromJson(cardsStr.toUtf8());
    QJsonArray cards = doc.array();
    if(cards.isEmpty()){
        qWarning() << "Current Audio Cards is Empty!!!!";
        return;
    }else{
        qInfo() << "Current Audio Cards: "<< cards;
    }
    qInfo() << "Updating audio device status. Found" << cards.size() << "audio cards";
    
    m_outAuidoPorts.clear();
    m_inAuidoPorts.clear();
    foreach (QJsonValue card, cards) {
        QJsonArray ports = card.toObject()["Ports"].toArray();
        foreach (QJsonValue port, ports) {
            QString portName = port.toObject()["Name"].toString();
            bool isEnable = false;
            AudioPort audioPort;
            audioPort.name = portName;
            audioPort.description = port.toObject()["Description"].toString();
            if(port.toObject().contains("Enabled")){
                isEnable = port.toObject()["Enabled"].toBool();
            }
            audioPort.availability = isEnable? 2:1;
            if(port.toObject().contains("Direction")){
                if(port.toObject()["Direction"].toInt() == 1 && isEnable){
                    m_outAuidoPorts.append(audioPort);
                }else if(port.toObject()["Direction"].toInt() == 2 && isEnable){
                    m_inAuidoPorts.append(audioPort);
                }
            }
        }
    }
    m_inAudioPort = currentAuidoPort(m_inAuidoPorts,Micphone);
    m_inIsEnable = (m_inAudioPort.availability == 2 || m_inAudioPort.availability == 0)? true : false;
    if (isEmitSig)
        sigDeviceEnableChanged(Micphone, m_inIsEnable);
    m_outAudioPort = currentAuidoPort(m_outAuidoPorts,Internal);
    m_outIsEnable = (m_outAudioPort.availability == 2 || m_inAudioPort.availability == 0)? true : false;
    if (isEmitSig)
        sigDeviceEnableChanged(Internal, m_outIsEnable);

    qInfo() << "Audio device status updated:"
            << "\nInput device:"
            << "\n  - Selected port:" << m_inAudioPort.name
            << "\n  - Enabled:" << m_inIsEnable
            << "\nOutput device:"
            << "\n  - Selected port:" << m_outAudioPort.name
            << "\n  - Enabled:" << m_outIsEnable;
}

AudioPort AudioWatcher::currentAuidoPort(const QList<AudioPort> &auidoPorts,AudioMode audioMode)
{
    qInfo() << "Getting current audio port";
    AudioPort currentAudioPort;
    if(auidoPorts.count()){
        qInfo() << "auidoPorts is not empty";
        foreach (AudioPort audioPort, auidoPorts) {
            AudioPort defaultAudioPort;
            if(audioMode == AudioMode::Internal){
                defaultAudioPort = defaultSinkActivePort();
            }else{
                defaultAudioPort = defaultSourceActivePort();
            }
            if(audioPort.name ==  defaultAudioPort.name){
                currentAudioPort = defaultAudioPort;
                break;
            }else{
                currentAudioPort = audioPort;
            }
        }
    }else{
        qInfo() << "auidoPorts is empty";
        currentAudioPort.name = "null";
        currentAudioPort.description = "null";
        currentAudioPort.availability = 1;
    }
    qInfo() << "Current audio port:" << currentAudioPort.name;
    return currentAudioPort;
}

void AudioWatcher::initDefaultSourceDBusInterface()
{
    qInfo() << "Initializing default source DBus interface";
    if (m_defaultSourceDBusInterface) {
        qInfo() << "m_defaultSourceDBusInterface is not nullptr";
        delete m_defaultSourceDBusInterface;
        m_defaultSourceDBusInterface = nullptr;
    }
    m_defaultSourceDBusInterface = new QDBusInterface(AudioService,
                                                      m_defaultSourcePath,
                                                      SourceInterface,
                                                      QDBusConnection::sessionBus());
    if (m_defaultSourceDBusInterface->isValid()) {
        qInfo() << "m_defaultSourceDBusInterface is valid";
        m_inAudioPortVolume = defaultSourceVolume();
        m_inAudioPort = defaultSourceActivePort();
        m_inAudioMute = defaultSourceMute();
        QDBusConnection::sessionBus().connect(AudioService,
                                              m_defaultSourcePath,
                                              PropertiesInterface,
                                              "PropertiesChanged",
                                              "sa{sv}as",
                                              this,
                                              SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                              );
    } else {
        qWarning() << "Default audio input source initialization failed！By default, the source address is entered (" << m_defaultSourcePath << ") does not exist";
    }
    qInfo() << "Default audio input source initialization finished";
}

void AudioWatcher::initDefaultSinkDBusInterface()
{
    qInfo() << "Initializing default sink DBus interface";
    if (m_defaultSinkDBusInterface) {
        qInfo() << "m_defaultSinkDBusInterface is not nullptr";
        delete m_defaultSinkDBusInterface;
        m_defaultSinkDBusInterface = nullptr;
    }
    m_defaultSinkDBusInterface = new QDBusInterface(AudioService,
                                                    m_defaultSinkPath,
                                                    SinkInterface,
                                                    QDBusConnection::sessionBus());
    if (m_defaultSinkDBusInterface->isValid()) {
        qInfo() << "m_defaultSinkDBusInterface is valid";
        m_outAudioPortVolume = defaultSinkVolume();
        m_outAudioPort = defaultSinkActivePort();
        m_outAudioMute = defaultSinkMute();

        QDBusConnection::sessionBus().connect(AudioService,
                                              m_defaultSinkPath,
                                              PropertiesInterface,
                                              "PropertiesChanged",
                                              "sa{sv}as",
                                              this,
                                              SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                              );
    } else {
        qWarning() << "Default audio output source initialization failed！The default output source address (" << m_defaultSinkPath << ") does not exist";
    }
    qInfo() << "Default audio output source initialization finished";
}

void AudioWatcher::onDBusAudioPropertyChanged(QDBusMessage msg)
{
    qInfo() << "Audio property changed";
    QList<QVariant> arguments = msg.arguments();
    if (3 != arguments.count()) {
        qWarning() << "Invalid DBus message received: incorrect argument count";
        return;
    }
    QString interfaceName = msg.arguments().at(0).toString();
    qInfo() << "Audio property changed on interface:" << interfaceName;
    
    if (interfaceName == AudioInterface) {
        qInfo() << "Audio property changed on interface:" << interfaceName;
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            if (prop == QStringLiteral("DefaultSource")) {
                const QDBusObjectPath &defaultSourcePath = qvariant_cast<QDBusObjectPath>(changedProps[prop]);
                qDebug() << "Default audio source changed to:" << defaultSourcePath.path();
                onDefaultSourceChanaged(defaultSourcePath);
            } else if (prop == QStringLiteral("DefaultSink")) {
                const QDBusObjectPath &defaultSinkePath = qvariant_cast<QDBusObjectPath>(changedProps[prop]);
                qDebug() << "Default audio sink changed to:" << defaultSinkePath.path();
                onDefaultSinkChanaged(defaultSinkePath);
            }else if (prop == "CardsWithoutUnavailable") {
                qDebug() << "Audio cards configuration changed";
                updateDeviceEnabled(changedProps[prop].toString(), true);
            } else if (prop == QStringLiteral("ReduceNoise")) {
                bool newReduceNoiseState = qvariant_cast<bool>(changedProps[prop]);
                if (m_isReduceNoise != newReduceNoiseState) {
                    m_isReduceNoise = newReduceNoiseState;
                    qInfo() << "ReduceNoise state changed to:" << m_isReduceNoise;
                    emit sigReduceNoiseChanged(m_isReduceNoise);
                }
            }
        }
    } else if (interfaceName == SourceInterface) {
        qInfo() << "Audio property changed on interface:" << interfaceName;
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            qInfo() << "property: " << prop << changedProps[prop];
            if (prop == QStringLiteral("Volume")) {
                double inAudioPortVolume = qvariant_cast<double>(changedProps[prop]);
                if (abs(m_inAudioPortVolume - inAudioPortVolume) >= 0.000001) {
                    onSourceVolumeChanged(inAudioPortVolume);
                }
            } else if (prop == QStringLiteral("Mute")) {
                const bool inAudioMute = qvariant_cast<bool>(changedProps[prop]);
                if (m_inAudioMute != inAudioMute) {
                    onSourceMuteChanged(inAudioMute);
                }
            } else if (prop == QStringLiteral("ActivePort")) {
                const AudioPort inAudioPort = defaultSourceActivePort();
                if (m_inAudioPort != inAudioPort) {
                    onDefaultSourceActivePortChanged(inAudioPort);
                }
            }
        }
    } else if (interfaceName == SinkInterface) {
        qInfo() << "Audio property changed on interface:" << interfaceName;
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            if (prop == QStringLiteral("Volume")) {
                double outAudioPortVolume = qvariant_cast<double>(changedProps[prop]);
                if (abs(m_outAudioPortVolume - outAudioPortVolume) >= 0.000001) {
                    onSinkVolumeChanged(outAudioPortVolume);
                }
            } else if (prop == QStringLiteral("Mute")) {
                const bool outAudioMute = qvariant_cast<bool>(changedProps[prop]);
                if (m_outAudioMute != outAudioMute) {
                    onSinkMuteChanged(outAudioMute);
                }
            } else if (prop == QStringLiteral("ActivePort")) {
                const AudioPort outAudioPort = defaultSinkActivePort();
                if (m_outAudioPort != outAudioPort) {
                    onDefaultSinkActivePortChanged(outAudioPort);
                }
            }
        }
    }
    qInfo() << "Audio property changed on interface:" << interfaceName << "finished";
}

void AudioWatcher::onSourceVolumeChanged(double value)
{
    qDebug() << "Input device volume changed from" << m_inAudioPortVolume << "to" << value;
    m_inAudioPortVolume = value;
    AudioPort activePort = defaultSourceActivePort();
    if (m_inAudioPort.name == activePort.name) {
        qInfo() << "Input device volume changed from" << m_inAudioPortVolume << "to" << value << "and active port is the same";
        emit sigVolumeChange(Micphone);
    } else {
        qInfo() << "Input device volume changed from" << m_inAudioPortVolume << "to" << value << "and active port is not the same";
        m_inAudioPort = activePort;
    }
    qInfo() << "Input device volume changed finished";
}

void AudioWatcher::onSinkVolumeChanged(double value)
{
    qDebug() << "Output device volume changed from" << m_outAudioPortVolume << "to" << value;
    m_outAudioPortVolume = value;
    AudioPort activePort = defaultSinkActivePort();
    if (m_outAudioPort.name == activePort.name) {
        qInfo() << "Output device volume changed from" << m_outAudioPortVolume << "to" << value << "and active port is the same";
        emit sigVolumeChange(Internal);
    } else {
        qInfo() << "Output device volume changed from" << m_outAudioPortVolume << "to" << value << "and active port is not the same";
        m_outAudioPort = activePort;
    }
    qInfo() << "Output device volume changed finished";
}

void AudioWatcher::onDefaultSourceChanaged(const QDBusObjectPath &defaultSourcePath)
{
    qInfo() << "Default audio input source changed to:" << defaultSourcePath.path();
    if (m_defaultSourcePath != defaultSourcePath.path()) {
        qInfo() << "default source path is not the same";
        QDBusConnection::sessionBus().disconnect(AudioService,
                                                 m_defaultSourcePath,
                                                 PropertiesInterface,
                                                 "PropertiesChanged",
                                                 "sa{sv}as",
                                                 this,
                                                 SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                                 );
        m_defaultSourcePath = defaultSourcePath.path();
        initDefaultSourceDBusInterface();
        emit sigDeviceChange(Micphone);
    }
    qInfo() << "Default audio input source changed finished";
}

void AudioWatcher::onDefaultSinkChanaged(const QDBusObjectPath &defaultSinkePath)
{
    qInfo() << "Default audio output source changed to:" << defaultSinkePath.path();
    if (m_defaultSinkPath != defaultSinkePath.path()) {
        qInfo() << "default sink path is not the same";
        QDBusConnection::sessionBus().disconnect(AudioService,
                                                 m_defaultSinkPath,
                                                 PropertiesInterface,
                                                 "PropertiesChanged",
                                                 "sa{sv}as",
                                                 this,
                                                 SLOT(onDBusAudioPropertyChanged(QDBusMessage))
                                                 );
        m_defaultSinkPath = defaultSinkePath.path();
        initDefaultSinkDBusInterface();
        emit sigDeviceChange(Internal);
    }
    qInfo() << "Default audio output source changed finished";
}

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

void AudioWatcher::onSinkMuteChanged(bool value)
{
    qDebug() << "Output device mute state changed to:" << value;
    m_outAudioMute = value;
    emit sigMuteChanged(Internal);
}

void AudioWatcher::onSourceMuteChanged(bool value)
{
    qDebug() << "Input device mute state changed to:" << value;
    m_inAudioMute = value;
    emit sigMuteChanged(Micphone);
}

QString AudioWatcher::getDeviceName(AudioMode mode)
{
    qInfo() << "Getting device name";
    QString device = "";
    if (mode == Internal) {
        qInfo() << "Getting output device name";
        if ((m_outAudioPort.availability != 1 || !m_fNeedDeviceChecker) &&
            (defaultSinkPorts().count() != 0 || m_isVirtualMachineHw)) {
            device = defaultSinkName();
            if (!device.isEmpty() && !device.endsWith(".monitor")) {
                device += ".monitor";
            }
        }
    } else {
        qInfo() << "Getting input device name";
        if ((m_inAudioPort.availability != 1 || !m_fNeedDeviceChecker) &&
            (defaultSourcePorts().count() != 0 || m_isVirtualMachineHw)) {
            device = defaultSourceName();
            if (device.endsWith(".monitor") && m_fNeedDeviceChecker) {
                device.clear();
            }
        }
    }
    qInfo() << "Device name:" << device;
    return device;
}

double AudioWatcher::getVolume(AudioMode mode)
{
    qInfo() << "Getting volume";
    return mode != Internal ? m_inAudioPortVolume : m_outAudioPortVolume;
}

bool AudioWatcher::getMute(AudioMode mode)
{
    qInfo() << "Getting mute";
    return mode != Internal ? m_inAudioMute : m_outAudioMute;
}

bool AudioWatcher::getDeviceEnable(AudioWatcher::AudioMode mode)
{
    qInfo() << "Getting device enable";
    QString cards = m_audioDBusInterface->property("Cards").value<QString>();
    if (m_isVirtualMachineHw && (cards.isEmpty() || cards.toLower() == "null")) {
        qInfo() << "Device enable is true";
        return true;
    } else {
        qInfo() << "Device enable is false";
        return mode != Internal ? m_inIsEnable : m_outIsEnable;
    }
}

/**
 * @brief AudioWatcher::initWatcherCofing
 */
void AudioWatcher::initWatcherCofing()
{
    qInfo() << "Initializing watcher config";
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
            qInfo() << "Device watcher config exists:" << configFileName;
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

AudioPort AudioWatcher::defaultSourceActivePort()
{
    qInfo() << "Getting default source active port";
    AudioPort port;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSourcePath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
        qInfo() << "Default source active port is valid";
        QDBusReply<QDBusVariant> reply = inter->call("Get", SourceInterface, "ActivePort");
        reply.value().variant().value<QDBusArgument>() >> port;
    } else {
        qWarning() << "By default, the source address is entered (" << m_defaultSourcePath << ") does not exist";
    }
    delete inter;
    inter = nullptr;
    return port;
}

double AudioWatcher::defaultSourceVolume()
{
    qInfo() << "Getting default source volume";
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        qInfo() << "Default source volume is valid";
        return m_defaultSourceDBusInterface->property("Volume").value<double>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return 0.0;
    }
}

bool AudioWatcher::defaultSourceMute()
{
    qInfo() << "Getting default source mute";
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        qInfo() << "Default source mute is valid";
        return m_defaultSourceDBusInterface->property("Mute").value<bool>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return true;
    }
}

QString AudioWatcher::defaultSourceName()
{
    qInfo() << "Getting default source name";
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        qInfo() << "Default source name is valid";
        return m_defaultSourceDBusInterface->property("Name").value<QString>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return "";
    }
}

QList<AudioPort> AudioWatcher::defaultSourcePorts()
{
    qInfo() << "Getting default source ports";
    QList<AudioPort> ports;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSourcePath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
        qInfo() << "Default source ports is valid";
        QDBusReply<QDBusVariant> reply = inter->call("Get", SourceInterface, "Ports");
        reply.value().variant().value<QDBusArgument>() >> ports;
        qInfo() << "Current Audio Source Ports Size:"<<ports.size() << "Ports" << ports;
    } else {
        qInfo() << "Current Audio Source Ports is nullptr or invalid";
    }
    delete inter;
    inter = nullptr;
    return ports;
}

AudioPort AudioWatcher::defaultSinkActivePort()
{
    qInfo() << "Getting default sink active port";
    AudioPort port;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSinkPath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
        qInfo() << "Default sink active port is valid";
        QDBusReply<QDBusVariant> reply = inter->call("Get", SinkInterface, "ActivePort");
        reply.value().variant().value<QDBusArgument>() >> port;
    } else {
        qWarning() << "The default output source address (" << m_defaultSinkPath << ") does not exist";
    }
    delete inter;
    inter = nullptr;
    return port;
}

double AudioWatcher::defaultSinkVolume()
{
    qInfo() << "Getting default sink volume";
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        qInfo() << "Default sink volume is valid";
        return m_defaultSinkDBusInterface->property("Volume").value<double>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return 0.0;
    }
}

bool AudioWatcher::defaultSinkMute()
{
    qInfo() << "Getting default sink mute";
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        qInfo() << "Default sink mute is valid";
        return m_defaultSinkDBusInterface->property("Mute").value<bool>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return true;
    }
}

QString AudioWatcher::defaultSinkName()
{
    qInfo() << "Getting default sink name";
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        qInfo() << "Default sink name is valid";
        return m_defaultSinkDBusInterface->property("Name").value<QString>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return "";
    }
}

QList<AudioPort> AudioWatcher::defaultSinkPorts()
{
    qInfo() << "Getting default sink ports";
    QList<AudioPort> ports;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSinkPath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
        qInfo() << "Default sink ports is valid";
        QDBusReply<QDBusVariant> reply = inter->call("Get", SinkInterface, "Ports");
        reply.value().variant().value<QDBusArgument>() >> ports;
        qInfo() << "Current Audio Sink Ports Size:"<< ports.size() << "Ports" << ports;
    } else {
        qInfo() << "Current Audio Sink Ports is nullptr or invalid";
    }
    delete inter;
    inter = nullptr;
    return ports;
}

