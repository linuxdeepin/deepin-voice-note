#ifndef QTPLAYER_H
#define QTPLAYER_H

#include <QObject>

// 条件编译：根据 Qt 版本包含不同的头文件
#ifdef USE_QT5
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioFormat>
#else
#include <QMediaPlayer>
#include <QAudioOutput>
#endif

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
