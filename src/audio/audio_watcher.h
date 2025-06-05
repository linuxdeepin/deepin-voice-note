#ifndef AUDIOWATCHER_H
#define AUDIOWATCHER_H

#include <QObject>
#include <QtDBus/QtDBus>

#define DEEPIN_VOICE_NOTE "deepin-voice-note"

const static QString AudioService = QStringLiteral("org.deepin.dde.Audio1");

const static QString AudioPath = QStringLiteral("/org/deepin/dde/Audio1");

const static QString AudioInterface = QStringLiteral("org.deepin.dde.Audio1");

const static QString SinkInterface = QStringLiteral("org.deepin.dde.Audio1.Sink");

const static QString SourceInterface = QStringLiteral("org.deepin.dde.Audio1.Source");

const static QString PropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
const static QString PropertiesChanged = QStringLiteral("PropertiesChanged");

class AudioPort
{
public:
    AudioPort() {}
    friend QDebug operator<<(QDebug argument, const AudioPort &port)
    {
        argument << port.name << port.description << port.availability;

        return argument;
    }

    friend QDBusArgument &operator<<(QDBusArgument &argument, const AudioPort &port)
    {
        argument.beginStructure();
        argument << port.name << port.description << port.availability;
        argument.endStructure();

        return argument;
    }

    friend const QDBusArgument &operator>>(const QDBusArgument &argument, AudioPort &port)
    {

        argument.beginStructure();
        argument >> port.name >> port.description >> port.availability;
        argument.endStructure();
        return argument;
    }

    bool operator==(const AudioPort what) const
    {
        return what.name == name && what.description == description && what.availability == availability;
    }

    bool operator!=(const AudioPort what) const
    {
        return what.name != name || what.description != description || what.availability != availability;
    }
public:
    QString name;
    QString description;
    uchar availability; // 0 for Unknown, 1 for Not Available, 2 for Available.
};


class AudioWatcher : public QObject
{
    Q_OBJECT
public:
    enum AudioMode {
        Internal,
        Micphone
    };
    Q_ENUM(AudioMode)
    explicit AudioWatcher(QObject *parent = nullptr);
    QString getDeviceName(AudioMode mode);
    double getVolume(AudioMode mode);
    bool getMute(AudioMode mode);
    bool getDeviceEnable(AudioMode mode);
signals:
    void sigVolumeChange(AudioMode mode);
    void sigDeviceChange(AudioMode mode);
    void sigMuteChanged(AudioMode mode);
    void sigDeviceEnableChanged(AudioMode mode, bool enable);
    void sigReduceNoiseChanged(bool reduceNoiseChanged);
protected slots:
    void onDefaultSourceActivePortChanged(AudioPort value);
    void onDefaultSinkActivePortChanged(AudioPort value);
    void onDefaultSourceChanaged(const QDBusObjectPath &value);
    void onDefaultSinkChanaged(const QDBusObjectPath &value);
    void onSourceVolumeChanged(double value);
    void onSinkVolumeChanged(double value);
    void onSourceMuteChanged(bool value);
    void onSinkMuteChanged(bool value);
    void onDBusAudioPropertyChanged(QDBusMessage msg);

private:
    void initWatcherCofing();
    void initDeviceWacther();
    void initConnections();
    void initDefaultSourceDBusInterface();
    void initDefaultSinkDBusInterface();
    AudioPort defaultSourceActivePort();
    double defaultSourceVolume();
    bool defaultSourceMute();
    QString defaultSourceName();
    QList<AudioPort> defaultSourcePorts();
    AudioPort defaultSinkActivePort();
    double defaultSinkVolume();
    bool defaultSinkMute();
    QString defaultSinkName();
    QList<AudioPort> defaultSinkPorts();

    bool isVirtualMachineHw();
    QString vnSystemInfo();
    void updateDeviceEnabled(const QString cardsStr, bool isEmitSig);
    AudioPort currentAuidoPort(const QList<AudioPort> &auidoPorts,AudioMode audioMode);
private:
    QDBusInterface *m_audioDBusInterface = nullptr;
    QDBusInterface *m_defaultSourceDBusInterface = nullptr;
    QDBusInterface *m_defaultSinkDBusInterface = nullptr;
    QString m_defaultSourcePath;
    QString m_defaultSinkPath;
    AudioPort m_outAudioPort;
    AudioPort m_inAudioPort;
    double m_outAudioPortVolume = 0.0;
    double m_inAudioPortVolume = 0.0;
    bool m_fNeedDeviceChecker {true};
    bool m_inAudioMute {false};
    bool m_outAudioMute {false};
    QList<AudioPort> m_outAuidoPorts;
    QList<AudioPort> m_inAuidoPorts;
    bool m_inIsEnable{false};
    bool m_outIsEnable{false};
    bool m_isVirtualMachineHw{false};
    bool m_isReduceNoise {false};
};

#endif // AUDIOWATCHER_H
