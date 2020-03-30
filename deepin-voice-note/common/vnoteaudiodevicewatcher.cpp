#include "vnoteaudiodevicewatcher.h"
#include "globaldef.h"

#include <QSettings>

#include <DLog>

VNoteAudioDeviceWatcher::VNoteAudioDeviceWatcher(QObject *parent)
    : QThread(parent)
{
//    initDeviceWatcher();
//    initConnections();
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
                    this )
                );

    m_defaultSource.reset(
                new com::deepin::daemon::audio::Source (
                    m_serviceName,
                    m_audioInterface->defaultSource().path(),
                    QDBusConnection::sessionBus(),
                    this )
                );

    initAudioMeter();
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

void VNoteAudioDeviceWatcher::exitWatcher()
{
    m_quitWatcher = true;
}

void VNoteAudioDeviceWatcher::OnDefaultSourceChanaged(const QDBusObjectPath &value)
{
    //TODO:
    //    When default source changed, need recreate
    // the source Object and the audio meter
    qInfo() << "source change:" << value.path();

    m_defaultSource.reset(
                new com::deepin::daemon::audio::Source (
                    m_serviceName,
                    value.path(),
                    QDBusConnection::sessionBus(),
                    this )
                );

    initAudioMeter();
}

void VNoteAudioDeviceWatcher::run()
{
    static const double DBL_EPSILON = 0.000001;
    static const double volumeLowMark = 0.2; //20% volume

    MicrophoneState currentState = MicrophoneState::NotAvailable;
    double volume = 0;
    //log volume one time per 5 minutes
    const int logTime = 60*5;
    int checkTimes = 0;

    //TODO:
    //    The server may be not stable now,so
    //need connect it every time,this need to
    //be optimized in future.
    do {
        //Quit watch thread
        if (m_quitWatcher) {
            break;
        }

        volume = 0;

        com::deepin::daemon::Audio audioInterface(
                    m_serviceName,
                    "/com/deepin/daemon/Audio",
                    QDBusConnection::sessionBus() );

        com::deepin::daemon::audio::Source defaultSource(
                    m_serviceName,
                    audioInterface.defaultSource().path(),
                    QDBusConnection::sessionBus() );

        QDBusPendingReply<QDBusObjectPath> reply = defaultSource.GetMeter();

    //    qInfo() << "GetMeter have error:" << reply.isError()
    //            << "error:" << reply.error();

        if (!reply.isError()) {
            QDBusObjectPath meterPath = reply.value();

            com::deepin::daemon::audio::Meter defaultSourceMeter(
                        m_serviceName,
                        meterPath.path(),
                        QDBusConnection::sessionBus() );

            if (m_fNeedDeviceChecker) {
                volume = defaultSourceMeter.volume();
            } else {
                volume = 0.1;
            }

            if (volume > DBL_EPSILON) {

                double currentMicrophoneVolume = defaultSource.volume();

                if ((currentMicrophoneVolume-volumeLowMark) > DBL_EPSILON) {
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

                qInfo() << "Microphone aviable state change:" << m_microphoneState;
            }
        }

        checkTimes++;

        if (0 == (checkTimes%logTime)) {
            qInfo() << "Volume:" << volume << "MicrophoneState:" << currentState;
        }

        //polling audio state per seconds
        msleep(AUDIO_DEV_CHECK_TIME);
    } while (1);

    //    do {
    //        initDeviceWatcher();
    //        //Quit watch thread
    //        if (m_quitWatcher) {
    //            break;
    //        }

    //        volume = 0;

    //        //Check audio meter
    //        if (!m_defaultSourceMeter.isNull()) {
    //            QVariant vol = m_defaultSourceMeter->property("Volume");

    //            volume = vol.toDouble();

    //            if (volume > DBL_EPSILON) {
    //                currentState = true;
    //            } else {
    //                currentState = false;
    //            }

    //            if (m_microphoneAviable != currentState) {
    //                m_microphoneAviable = currentState;
    //                emit microphoneAvailableState(m_microphoneAviable);

    //                qInfo() << "Microphone aviable state change:" << m_microphoneAviable;
    //            }
    //        } else {
    //            initAudioMeter();
    //        }

    //#ifdef QT_QML_DEBUG
    //        qInfo() << "Volume:" << volume;
    //#endif
    //        //polling audio state per seconds
    //        msleep(AUDIO_DEV_CHECK_TIME);
    //    } while (1);
}

void VNoteAudioDeviceWatcher::initAudioMeter()
{
    QDBusPendingReply<QDBusObjectPath> reply = m_defaultSource->GetMeter();

    if (!reply.isError()) {
        QDBusObjectPath meterPath = reply.value();

        qInfo() << "GetMeter have error:" << reply.isError()
                << "error:" << reply.error()
                << "Meter:" << meterPath.path();

        m_defaultSourceMeter.reset(
                    new com::deepin::daemon::audio::Meter (
                        m_serviceName,
                        meterPath.path(),
                        QDBusConnection::sessionBus(),
                        this )
                    );
    }
}

void VNoteAudioDeviceWatcher::initConnections()
{
    //Default source change event
    connect(m_audioInterface.get(), &com::deepin::daemon::Audio::DefaultSourceChanged,
            this, &VNoteAudioDeviceWatcher::OnDefaultSourceChanaged);
}
