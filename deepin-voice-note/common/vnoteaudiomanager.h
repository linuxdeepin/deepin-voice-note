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
    void recExceedLimit();
    void recDurationChange(qint64 duration);
    void recAudioBufferProbed(const QAudioBuffer &buffer);
public slots:
    void recordDurationChanged(qint64 duration);

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
    QString m_audioCodec;
    QString m_audioContainer;
    int     m_channelCount {2};

    QString m_recordFileName;
};

#endif // VNOTEAUDIOMANAGER_H
