#ifndef VOICEPLAYERBASE_H
#define VOICEPLAYERBASE_H

#include <QObject>
#include <QMediaPlayer>

class VoicePlayerBase : public QObject
{
    Q_OBJECT
public:
    VoicePlayerBase(QObject *parent = nullptr);

    enum PlayerState
    {
        None = 0,
        Playing = 3,
        Paused,
        Stopped,
        Ended
    };

    virtual void setFilePath(QString path) = 0;
    virtual void setPosition(qint64 pos) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual PlayerState getState() = 0;
    void setChangePlayFile(bool flag);

signals:
    // 总时长改变
    void durationChanged(qint64 duration);
    // 播放位置改变，单位:毫秒
    void positionChanged(qint64 position);
    // 播放完成
    void playEnd();
};

#endif // VOICEPLAYERBASE_H
