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
#include "vnoteaudiodevicewatcher.h"
#include "globaldef.h"

#include <QSettings>

#include <DLog>

VNoteAudioDeviceWatcher::VNoteAudioDeviceWatcher(QObject *parent)
    : QThread(parent)
{
    initDeviceWatcher();
    initConnections();
    initWatcherCofing();
}

VNoteAudioDeviceWatcher::~VNoteAudioDeviceWatcher()
{
}

void VNoteAudioDeviceWatcher::initDeviceWatcher()
{
    m_audioInterface.reset(
        new com::deepin::daemon::Audio(
            m_serviceName,
            "/com/deepin/daemon/Audio",
            QDBusConnection::sessionBus(),
            this));

    m_defaultSource.reset(
        new com::deepin::daemon::audio::Source(
            m_serviceName,
            m_audioInterface->defaultSource().path(),
            QDBusConnection::sessionBus(),
            this));

    //Initialze the cards
    //onCardsChanged(m_audioInterface->cards());
}

void VNoteAudioDeviceWatcher::initWatcherCofing()
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

void VNoteAudioDeviceWatcher::exitWatcher()
{
    m_quitWatcher = true;
}

void VNoteAudioDeviceWatcher::onDefaultSourceChanaged(const QDBusObjectPath &value)
{
    //TODO:
    //    When default source changed, need recreate
    // the source Object
    qInfo() << "Source change-->from:" << m_defaultSource->path()
            << " name:" << m_defaultSource->name();

    m_defaultSource.reset(
        new com::deepin::daemon::audio::Source(
            m_serviceName,
            value.path(),
            QDBusConnection::sessionBus(),
            this));

    AudioPort activePort = m_defaultSource->activePort();

    qInfo() << "Source change:-->to:" << value.path()
            << " name:" << m_defaultSource->name()
            << " activePort:" << activePort.name;

    emit inputSourceChanged(activePort.description);
}

//void VNoteAudioDeviceWatcher::onCardsChanged(const QString &value)
//{
//    qInfo() << "Cards changed:" << value;

//    initAvailInputPorts(value);
//}

void VNoteAudioDeviceWatcher::run()
{
    static const double DBL_EPSILON = 0.0;
    static const double volumeLowMark = 0.2; //20% volume

    MicrophoneState currentState = MicrophoneState::NotAvailable;

    //log microphone state one time per 1 minutes
    const int LOG_FREQ = 1 * 60 * 1000;

    static struct timeval curret = {0, 0};
    static struct timeval lastPrint = {0, 0};

    //
    bool fHighPriorityLog = false;

    do {
        //Quit watch thread
        if (m_quitWatcher) {
            break;
        }

        gettimeofday(&curret, nullptr);

        double currentMicrophoneVolume = 0;

        AudioPort activePort = m_defaultSource->activePort();

        if (isMicrophoneAvail(activePort) || (!m_fNeedDeviceChecker)) {
            currentMicrophoneVolume = m_defaultSource->volume();

            if ((currentMicrophoneVolume - volumeLowMark) >= DBL_EPSILON) {
                currentState = MicrophoneState::Normal;
            } else {
                currentState = MicrophoneState::VolumeTooLow;
            }
        } else {
            currentState = MicrophoneState::NotAvailable;
        }

        if (m_microphoneState != currentState) {
            m_microphoneState = currentState;
            emit microphoneAvailableState(m_microphoneState);

            qInfo() << "Microphone available state change:" << m_microphoneState;

            //We need log the state change log.
            fHighPriorityLog = true;
        }

        if (TM(lastPrint, curret) > LOG_FREQ || fHighPriorityLog) {
            qInfo() << "Volume:" << currentMicrophoneVolume
                    << ", MicrophoneState:" << currentState
                    << ", FHighPriorityLog:" << fHighPriorityLog
                    << ", ActivePort {"
                    << activePort.name << ", "
                    << activePort.description << ", "
                    << activePort.availability << " }";

            //Update the log time
            UPT(lastPrint, curret);

            //Clear important log flag
            if (fHighPriorityLog) {
                fHighPriorityLog = false;
            }
        }

        //polling audio state per seconds
        msleep(AUDIO_DEV_CHECK_TIME);
    } while (1);
}

void VNoteAudioDeviceWatcher::initConnections()
{
    //Default source change event
    connect(m_audioInterface.get(), &com::deepin::daemon::Audio::DefaultSourceChanged,
            this, &VNoteAudioDeviceWatcher::onDefaultSourceChanaged);

    //connect(m_audioInterface.get(), &com::deepin::daemon::Audio::CardsChanged, this, &VNoteAudioDeviceWatcher::onCardsChanged);
}

void VNoteAudioDeviceWatcher::initAvailInputPorts(const QString &cards)
{
    m_availableInputPorts.clear();

    QJsonDocument doc = QJsonDocument::fromJson(cards.toUtf8());
    QJsonArray jCards = doc.array();

    for (QJsonValue cardVar : jCards) {
        QJsonObject jCard = cardVar.toObject();
        const QString cardName = jCard["Name"].toString();
        const int cardId = jCard["Id"].toInt();

        QJsonArray jPorts = jCard["Ports"].toArray();

        for (QJsonValue portVar : jPorts) {
            Port port;

            QJsonObject jPort = portVar.toObject();
            port.available = jPort["Available"].toInt();

            // 0 Unknow 1 Not available 2 Available
            if (port.available == PortState::Available || port.available == PortState::Unkown) {
                port.portId = jPort["Name"].toString();
                port.portName = jPort["Description"].toString();
                port.cardId = cardId;
                port.cardName = cardName;

                if (port.isInputPort()) {
                    m_availableInputPorts.insert(port.portId, port);

                    qDebug() << " " << port;
                }
            }
        }
    }
}

bool VNoteAudioDeviceWatcher::isMicrophoneAvail(const AudioPort &activePort) const
{
//    bool fAvailable = false;

//    QMap<PortID, Port>::const_iterator iter = m_availableInputPorts.find(activePort);

//    if (iter != m_availableInputPorts.end()) {
//        if (!iter->isLoopback()) {
//            fAvailable = true;
//        }
//    }

//    return fAvailable;
    return  activePort.availability != 1 && activePort.name.contains("input", Qt::CaseInsensitive);
}

bool VNoteAudioDeviceWatcher::Port::isInputPort() const
{
    const QString inputPortFingerprint("input");

    return portId.contains(inputPortFingerprint, Qt::CaseInsensitive);
}

bool VNoteAudioDeviceWatcher::Port::isLoopback() const
{
    const QString loopbackFingerprint("Loopback");

    return cardName.contains(loopbackFingerprint, Qt::CaseInsensitive);
}

QDebug &operator<<(QDebug &out, const VNoteAudioDeviceWatcher::Port &port)
{
    out << "\n Port { "
        << "portId=" << port.portId << ","
        << "portName=" << port.portName << ","
        << "available=" << port.available << ","
        << "isActive=" << port.isActive << ","
        << "cardName=" << port.cardName << ","
        << "cardId=" << port.cardId << ","
        << " }\n";

    return out;
}
