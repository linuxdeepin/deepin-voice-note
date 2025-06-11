#ifndef VOICERECODERHANDLER_H
#define VOICERECODERHANDLER_H

#include <QObject>
// 条件编译：根据 Qt 版本包含不同的音频设备头文件
#ifdef USE_QT5
#include <QAudioDeviceInfo>
#else
#include <QMediaDevices>
#include <QAudioDevice>
#endif

class GstreamRecorder;
class AudioWatcher;
class QAudioBuffer;
class VoiceRecoderHandler: public QObject
{
    Q_OBJECT
    Q_ENUMS(RecoderType)
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
    Q_INVOKABLE void confirmStartRecoder();

public slots:
    void onDeviceEnableChanged(int mode, bool enabled);

signals:
    void recoderStateChange(RecoderType type);
    void finishedRecod(const QString &path, qint64 voiceSize);
    void recoderDurationChange(qint64 duation);
    void updateRecorderTime(const QVariant &time);
    void updateWave(qreal max);
    void updateRecordBtnState(bool enable);
    void volumeTooLow(bool isLow);

private:
    VoiceRecoderHandler();

    void intRecoder();
    void initRecordPath();
    void initAudioWatcher();

    bool checkVolume();
    QString tryGetMicNameFromPactl() const;
    QString getDefaultMicDeviceName() const;
    
private slots:
    void onAudioDeviceChange(int mode);
    void onAudioBufferProbed(const QAudioBuffer &buffer);
    void onReduceNoiseChanged(bool reduceNoiseChanged);
private:
    GstreamRecorder *m_audioRecoder;
    AudioWatcher *m_audioWatcher;
#ifndef USE_QT5
    QMediaDevices *m_mediaDevices;
#endif
    RecoderType m_type;

    QString m_recordDir {""};
    QString m_recordPath {""};
    qint64 m_recordMsec {0};
    int m_currentMode {1};
};

#endif // VOICERECODERHANDLER_H
