#ifndef VNOTEAUDIODEVICEWATCHER_H
#define VNOTEAUDIODEVICEWATCHER_H

#include <com_deepin_daemon_audio.h>
#include <com_deepin_daemon_audio_source.h>
#include <com_deepin_daemon_audio_meter.h>

#include <QThread>

class VNoteAudioDeviceWatcher : public QThread
{
    Q_OBJECT
public:
    explicit VNoteAudioDeviceWatcher(QObject *parent = nullptr);
    virtual ~VNoteAudioDeviceWatcher() override;

    void initDeviceWatcher();
    void exitWatcher();

signals:
    void microphoneAvailableState(bool isAvailable);
public slots:
    void OnDefaultSourceChanaged(const QDBusObjectPath & value);
protected:
    virtual void run() override;

    void initAudioMeter();
    void initConnections();

private:
    const QString m_serviceName {"com.deepin.daemon.Audio"};

    QScopedPointer<com::deepin::daemon::Audio> m_audioInterface;
    QScopedPointer<com::deepin::daemon::audio::Source> m_defaultSource;
    QScopedPointer<com::deepin::daemon::audio::Meter> m_defaultSourceMeter;

    bool m_microphoneAviable {false};
    bool m_quitWatcher {false};
};

#endif // VNOTEAUDIODEVICEWATCHER_H
