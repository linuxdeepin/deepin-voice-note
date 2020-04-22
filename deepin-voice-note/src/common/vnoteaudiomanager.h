#ifndef VNOTEAUDIOMANAGER_H
#define VNOTEAUDIOMANAGER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioRecorder>
#include <QAudioProbe>

class VNoteAudioManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteAudioManager(QObject *parent = nullptr);

    static VNoteAudioManager* instance();
    //The max record limit is 3 hours
    static constexpr int MAX_REC_TIME_INMSEC = (60*60*1000);
    static constexpr int REC_NOTFY_TIME = 500; //ms

    void initAudio();
    void updateAudioInputParam();
    void initConnections();

    //Audio player
    void setPlayFileName(const QString& fileName);
    void startPlay();
    void pausePlay();
    void stopPlay();

    QMediaPlayer *getPlayerObject();

    //Audio record
    void setRecordFileName(const QString& fileName);
    void startRecord();
    void pauseRecord();
    void stopRecord();
signals:
    void recDurationChange(qint64 duration);
    void recAudioBufferProbed(const QAudioBuffer &buffer);
public slots:
    void recordDurationChanged(qint64 duration);
    void onDefaultInputChanged(const QString& name);
    void onRecordError(QMediaRecorder::Error error);

protected:
    static VNoteAudioManager * _instance;

    QString m_defaultAudioFmt {"audio/mpeg"};
    //Audio player
    QScopedPointer<QMediaPlayer> m_pAudioPlayer;
    QScopedPointer<QAudioProbe>  m_pAudioPlayerProbe;
    QString m_playFileName;
    qint64  m_playPosition {0};
    //Audio record
    QScopedPointer<QAudioRecorder> m_pAudioRecord;
    QScopedPointer<QAudioProbe> m_pAudioRecProbe;

    enum AudioParam {
        MaxChannelCount = 2,   //Limit the channel count
        MaxSampleRate = 44100, //Limit sample rate to 44.1kz
    };

    QString m_audioCodec;
    QString m_audioContainer;

    QAudioEncoderSettings m_audioEncoderSetting;

    QString m_recordFileName;
};

#endif // VNOTEAUDIOMANAGER_H
