#ifndef VOICERECODERHANDLER_H
#define VOICERECODERHANDLER_H

#include <QObject>

class GstreamRecorder;
class AudioWatcher;
class QAudioBuffer;
class VoiceRecoderHandler: public QObject
{
    Q_OBJECT
    Q_ENUMS(SkinStyle)
public:
    enum RecoderType
    {
        Idle = 0,
        Recording,
        Paused
    };
public:
    static VoiceRecoderHandler* instance();
    Q_INVOKABLE RecoderType getRecoderType();
    Q_INVOKABLE void startRecoder();
    Q_INVOKABLE void stopRecoder();
    Q_INVOKABLE void pauseRecoder();
    Q_INVOKABLE void setAudioDevice(const QString &device);
    Q_INVOKABLE void changeMode(const int &mode);

signals:
    void recoderStateChange(RecoderType type);
    void finishedRecod(const QString &path, qint64 voiceSize);
    void recoderDurationChange(qint64 duation);
    void updateRecorderTime(const QVariant &time);

private:
    VoiceRecoderHandler();

    void intRecoder();
    void initRecordPath();
    void initAudioWatcher();

private slots:
    void onAudioDeviceChange(int mode);
    void onAudioBufferProbed(const QAudioBuffer &buffer);

private:
    GstreamRecorder *m_audioRecoder;
    AudioWatcher *m_audioWatcher;
    RecoderType m_type;

    QString m_recordDir {""};
    QString m_recordPath {""};
    qint64 m_recordMsec {0};
    int m_currentMode {0};
};

#endif // VOICERECODERHANDLER_H
