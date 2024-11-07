#ifndef QTPLAYER_H
#define QTPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>

#include "voiceplayerbase.h"

class QtPlayer : public VoicePlayerBase
{
    Q_OBJECT
public:
    QtPlayer(QObject *parent);
    ~QtPlayer();

    void setFilePath(QString path);
    void setPosition(qint64 pos);
    void play();
    void pause();
    void stop();
    PlayerState getState();

private:
    void initConnection();

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
};

#endif // QTPLAYER_H
