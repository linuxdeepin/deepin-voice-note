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

    void initAudio();

    //Audio player
    void setPlayFileName(const QString& fileName);
    void startPlay();
    void pausePlay();
    void stopPlay();

    //Audio record
    void setRecordFileName(const QString& fileName);
    void startRecord();
    void pauseRecord();
    void stopRecord();
signals:

public slots:

protected:
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
