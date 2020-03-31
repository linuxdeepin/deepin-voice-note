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
                    this )
                );

    m_defaultSource.reset(
                new com::deepin::daemon::audio::Source (
                    m_serviceName,
                    m_audioInterface->defaultSource().path(),
                    QDBusConnection::sessionBus(),
                    this )
                );
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
    // the source Object
    qInfo() << "Source change-->from:" << m_defaultSource->path()
            << " name:" << m_defaultSource->name();

    m_defaultSource.reset(
                new com::deepin::daemon::audio::Source (
                    m_serviceName,
                    value.path(),
                    QDBusConnection::sessionBus(),
                    this )
                );

    qInfo() << "Source change:-->" << value.path()
            << " name:" << m_defaultSource->name();;
}

void VNoteAudioDeviceWatcher::run()
{
    static const double DBL_EPSILON = 0.000001;
    static const double volumeLowMark = 0.2; //20% volume

    MicrophoneState currentState = MicrophoneState::NotAvailable;

    //log microphone state one time per 1 minutes
    const int LOG_FREQ = 1*60*1000;

    static struct timeval curret = {0,0};
    static struct timeval lastPrint = {0,0};

    do {
        //Quit watch thread
        if (m_quitWatcher) {
            break;
        }

        gettimeofday(&curret, nullptr);

        double currentMicrophoneVolume = 0;

        AudioPort activePort = m_defaultSource->activePort();

        if (activePort.availability == PortState::Available || (!m_fNeedDeviceChecker)) {
            currentMicrophoneVolume = m_defaultSource->volume();

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

        if (TM(lastPrint, curret) > LOG_FREQ) {
            qInfo() << "Volume:" << currentMicrophoneVolume
                    << "MicrophoneState:" << currentState
                    << "activePort {"
                    << activePort.name << ", "
                    << activePort.description << ", "
                    << activePort.availability << " }";

            //Update the log time
            lastPrint = curret;
        }

        //polling audio state per seconds
        msleep(AUDIO_DEV_CHECK_TIME);
    } while (1);
}

void VNoteAudioDeviceWatcher::initConnections()
{
    //Default source change event
    connect(m_audioInterface.get(), &com::deepin::daemon::Audio::DefaultSourceChanged,
            this, &VNoteAudioDeviceWatcher::OnDefaultSourceChanaged);
}
