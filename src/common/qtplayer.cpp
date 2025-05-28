#include "qtplayer.h"

#include <QDebug>

QtPlayer::QtPlayer(QObject *parent)
    : VoicePlayerBase(parent)
{
    qInfo() << "Initializing QtPlayer";
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    initConnection();
}

QtPlayer::~QtPlayer()
{
    qDebug() << "Destroying QtPlayer";
    if (m_player) {
        delete m_player;
        m_player = nullptr;
        qDebug() << "Media player deleted";
    }
    if (m_audioOutput) {
        delete m_audioOutput;
        m_audioOutput = nullptr;
        qDebug() << "Audio output deleted";
    }
}

void QtPlayer::setFilePath(QString path)
{
    qDebug() << "Setting media file path:" << path;
    m_player->setSource(QUrl(path));
}

void QtPlayer::setPosition(qint64 pos)
{
    m_player->setPosition(pos);
}

void QtPlayer::play()
{
    m_player->play();
}

void QtPlayer::pause()
{
    m_player->pause();
}

void QtPlayer::stop()
{
    m_player->stop();
}

VoicePlayerBase::PlayerState QtPlayer::getState()
{
    VoicePlayerBase::PlayerState state = VoicePlayerBase::None;
    QMediaPlayer::MediaStatus mediaState = m_player->mediaStatus();
    if (mediaState == QMediaPlayer::EndOfMedia) {
        qDebug() << "Media playback reached end";
        return VoicePlayerBase::PlayerState::Ended;
    }
    
    QMediaPlayer::PlaybackState playState = m_player->playbackState();
    switch(playState) {
    case QMediaPlayer::PlayingState:
        state = VoicePlayerBase::PlayerState::Playing;
        qDebug() << "Player state: Playing";
        break;
    case QMediaPlayer::PausedState:
        state = VoicePlayerBase::PlayerState::Paused;
        qDebug() << "Player state: Paused";
        break;
    case QMediaPlayer::StoppedState:
        state = VoicePlayerBase::PlayerState::Stopped;
        qDebug() << "Player state: Stopped";
        break;
    default:
        qDebug() << "Player state: Unknown";
        break;
    }
    return state;
}

void QtPlayer::initConnection()
{
    connect(m_player, &QMediaPlayer::durationChanged, this, &QtPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &QtPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, [=](QMediaPlayer::MediaStatus status){
        qDebug() << "Media status changed:" << status;
        if (status == QMediaPlayer::EndOfMedia) {
            qDebug() << "Emitting playEnd signal";
            emit playEnd();
        }
    });
}
