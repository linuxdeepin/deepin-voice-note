#include "audio_watcher.h"

AudioWatcher::AudioWatcher(QObject *parent)
    : QObject(parent)
{
    m_isVirtualMachineHw = isVirtualMachineHw();
    initWatcherCofing();
    initDeviceWacther();
    initConnections();
}

/**
 * @brief AudioWatcher::initDeviceWacther
 */
void AudioWatcher::initDeviceWacther()
{
    m_audioDBusInterface = new QDBusInterface(AudioService,
                                              AudioPath,
                                              AudioInterface,
                                              QDBusConnection::sessionBus());
    if (m_audioDBusInterface->isValid()) {
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
        qWarning() << "Initialization failed！Audio Services (" << AudioService << ") does not exist";
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
    bool isVirtualMachine = false;
    QString reslut = vnSystemInfo();
    if (reslut.isEmpty()) {
        return isVirtualMachine;
    }
    if (!reslut.contains("none")) {
        isVirtualMachine = true;
    }
    return isVirtualMachine;
}

QString AudioWatcher::vnSystemInfo()
{

    QProcess process;
    process.start("systemd-detect-virt");
    process.waitForFinished();
    process.waitForReadyRead();
    QByteArray tempArray =  process.readAllStandardOutput();
    QString reslut = QString(QLatin1String( tempArray.data()));
    qDebug() << "reslut: " << reslut;
    process.close();
    return reslut;
}

void AudioWatcher::updateDeviceEnabled(const QString cardsStr, bool isEmitSig)
{
    QJsonDocument doc = QJsonDocument::fromJson(cardsStr.toUtf8());
    QJsonArray cards = doc.array();
    if(cards.isEmpty()){
        qWarning() << "Current Audio Cards is Empty!!!!";
        return;
    }else{
        qInfo() << "Current Audio Cards: "<< cards;
    }
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

    qInfo() << "current select in audioPort: "<< m_inAudioPort << ",is enable:" << m_inIsEnable;
    qInfo() << "current select out audioPort: "<< m_outAudioPort << ",is enable:" << m_outIsEnable;
}

AudioPort AudioWatcher::currentAuidoPort(const QList<AudioPort> &auidoPorts,AudioMode audioMode)
{
    AudioPort currentAudioPort;
    if(auidoPorts.count()){
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
        currentAudioPort.name = "null";
        currentAudioPort.description = "null";
        currentAudioPort.availability = 1;
    }
    return currentAudioPort;
}

void AudioWatcher::initDefaultSourceDBusInterface()
{
    if (m_defaultSourceDBusInterface) {
        delete m_defaultSourceDBusInterface;
        m_defaultSourceDBusInterface = nullptr;
    }
    m_defaultSourceDBusInterface = new QDBusInterface(AudioService,
                                                      m_defaultSourcePath,
                                                      SourceInterface,
                                                      QDBusConnection::sessionBus());
    if (m_defaultSourceDBusInterface->isValid()) {
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
}

void AudioWatcher::initDefaultSinkDBusInterface()
{
    if (m_defaultSinkDBusInterface) {
        delete m_defaultSinkDBusInterface;
        m_defaultSinkDBusInterface = nullptr;
    }
    m_defaultSinkDBusInterface = new QDBusInterface(AudioService,
                                                    m_defaultSinkPath,
                                                    SinkInterface,
                                                    QDBusConnection::sessionBus());
    if (m_defaultSinkDBusInterface->isValid()) {
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
}

void AudioWatcher::onDBusAudioPropertyChanged(QDBusMessage msg)
{
    QList<QVariant> arguments = msg.arguments();
    if (3 != arguments.count()) {
        return;
    }
    QString interfaceName = msg.arguments().at(0).toString();
    if (interfaceName == AudioInterface) {
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            if (prop == QStringLiteral("DefaultSource")) {
                const QDBusObjectPath &defaultSourcePath = qvariant_cast<QDBusObjectPath>(changedProps[prop]);
                onDefaultSourceChanaged(defaultSourcePath);
            } else if (prop == QStringLiteral("DefaultSink")) {
                const QDBusObjectPath &defaultSinkePath = qvariant_cast<QDBusObjectPath>(changedProps[prop]);
                onDefaultSinkChanaged(defaultSinkePath);
            }else if (prop == "CardsWithoutUnavailable") {
                updateDeviceEnabled(changedProps[prop].toString(), true);
            }
        }
    } else if (interfaceName == SourceInterface) {
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
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys =  changedProps.keys();
        foreach (const QString &prop, keys) {
            if (prop == QStringLiteral("Volume")) {
                double outAudioPortVolume = qvariant_cast<double>(changedProps[prop]);
                if (abs(m_outAudioPortVolume - outAudioPortVolume) >= 0.000001) {
                    onSourceVolumeChanged(outAudioPortVolume);
                }
            } else if (prop == QStringLiteral("Mute")) {
                const bool outAudioMute = qvariant_cast<bool>(changedProps[prop]);
                if (m_outAudioMute != outAudioMute) {
                    onSourceMuteChanged(outAudioMute);
                }
            } else if (prop == QStringLiteral("ActivePort")) {
                const AudioPort outAudioPort = defaultSinkActivePort();
                if (m_outAudioPort != outAudioPort) {
                    onDefaultSinkActivePortChanged(outAudioPort);
                }
            }
        }
    }
}

void AudioWatcher::onSourceVolumeChanged(double value)
{
    m_inAudioPortVolume = value;
    AudioPort activePort = defaultSourceActivePort();
    if (m_inAudioPort.name == activePort.name) {
        emit sigVolumeChange(Micphone);
    } else {
        m_inAudioPort = activePort;
    }
}

void AudioWatcher::onSinkVolumeChanged(double value)
{
    m_outAudioPortVolume = value;
    AudioPort activePort = defaultSinkActivePort();
    if (m_outAudioPort.name == activePort.name) {
        emit sigVolumeChange(Internal);
    } else {
        m_outAudioPort = activePort;
    }
}

void AudioWatcher::onDefaultSourceChanaged(const QDBusObjectPath &defaultSourcePath)
{
    if (m_defaultSourcePath != defaultSourcePath.path()) {
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
}

void AudioWatcher::onDefaultSinkChanaged(const QDBusObjectPath &defaultSinkePath)
{
    if (m_defaultSinkPath != defaultSinkePath.path()) {
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
    m_outAudioMute = value;
    emit sigMuteChanged(Internal);
}

void AudioWatcher::onSourceMuteChanged(bool value)
{
    m_inAudioMute = value;
    emit sigMuteChanged(Micphone);
}

QString AudioWatcher::getDeviceName(AudioMode mode)
{
    QString device = "";
    if (mode == Internal) {
        if ((m_outAudioPort.availability != 1 || !m_fNeedDeviceChecker) &&
            (defaultSinkPorts().count() != 0 || m_isVirtualMachineHw)) {
            device = defaultSinkName();
            if (!device.isEmpty() && !device.endsWith(".monitor")) {
                device += ".monitor";
            }
        }
    } else {
        if ((m_inAudioPort.availability != 1 || !m_fNeedDeviceChecker) &&
            (defaultSourcePorts().count() != 0 || m_isVirtualMachineHw)) {
            device = defaultSourceName();
            if (device.endsWith(".monitor") && m_fNeedDeviceChecker) {
                device.clear();
            }
        }
    }
    return device;
}

double AudioWatcher::getVolume(AudioMode mode)
{
    return mode != Internal ? m_inAudioPortVolume : m_outAudioPortVolume;
}

bool AudioWatcher::getMute(AudioMode mode)
{
    return mode != Internal ? m_inAudioMute : m_outAudioMute;
}

bool AudioWatcher::getDeviceEnable(AudioWatcher::AudioMode mode)
{
    QString cards = m_audioDBusInterface->property("Cards").value<QString>();
    if (m_isVirtualMachineHw && (cards.isEmpty() || cards.toLower() == "null")) {
        return true;
    } else {
        return mode != Internal ? m_inIsEnable : m_outIsEnable;
    }
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

AudioPort AudioWatcher::defaultSourceActivePort()
{
    AudioPort port;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSourcePath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
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
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        return m_defaultSourceDBusInterface->property("Volume").value<double>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return 0.0;
    }
}

bool AudioWatcher::defaultSourceMute()
{
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        return m_defaultSourceDBusInterface->property("Mute").value<bool>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return true;
    }
}

QString AudioWatcher::defaultSourceName()
{
    if (m_defaultSourceDBusInterface && m_defaultSourceDBusInterface->isValid()) {
        return m_defaultSourceDBusInterface->property("Name").value<QString>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSourceDBusInterface is nullptr or invalid";
        return "";
    }
}

QList<AudioPort> AudioWatcher::defaultSourcePorts()
{
    QList<AudioPort> ports;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSourcePath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
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
    AudioPort port;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSinkPath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
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
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        return m_defaultSinkDBusInterface->property("Volume").value<double>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return 0.0;
    }
}

bool AudioWatcher::defaultSinkMute()
{
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        return m_defaultSinkDBusInterface->property("Mute").value<bool>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return true;
    }
}

QString AudioWatcher::defaultSinkName()
{
    if (m_defaultSinkDBusInterface && m_defaultSinkDBusInterface->isValid()) {
        return m_defaultSinkDBusInterface->property("Name").value<QString>();
    } else {
        qInfo() << __FUNCTION__ << __LINE__ << "m_defaultSinkDBusInterface is nullptr or invalid";
        return "";
    }
}

QList<AudioPort> AudioWatcher::defaultSinkPorts()
{
    QList<AudioPort> ports;
    auto inter = new QDBusInterface(AudioService,
                                    m_defaultSinkPath,
                                    PropertiesInterface,
                                    QDBusConnection::sessionBus());

    if (inter->isValid()) {
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

