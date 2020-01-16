#include "vnoteaudiodevicewatcher.h"
#include "globaldef.h"

#include <DLog>

VNoteAudioDeviceWatcher::VNoteAudioDeviceWatcher(QObject *parent)
    : QThread(parent)
{
    initDeviceWatcher();
    initConnections();
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
    static const double DBL_EPSILON = 0.0001;

    bool   currentState = false;
    double volume = 0;

    do {
        //Quit watch thread
        if (m_quitWatcher) {
            break;
        }

        volume = 0;

        //Check audio meter
        if (!m_defaultSourceMeter.isNull()) {
            QVariant vol = m_defaultSourceMeter->property("Volume");

            volume = vol.toDouble();

            if (volume > DBL_EPSILON) {
                currentState = true;
            } else {
                currentState = false;
            }

            if (m_microphoneAviable != currentState) {
                m_microphoneAviable = currentState;
                emit microphoneAvailableState(m_microphoneAviable);

                qInfo() << "Microphone aviable state change:" << m_microphoneAviable;
            }
        } else {
            initAudioMeter();
        }

#ifdef QT_QML_DEBUG
        qInfo() << "Volume:" << volume;
#endif
        //polling audio state per seconds
        msleep(AUDIO_DEV_CHECK_TIME);
    } while (1);
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
