#include "qtplayer.h"

#include <QDebug>

QtPlayer::QtPlayer(QObject *parent)
    : VoicePlayerBase(parent)
{
    qInfo() << "The current playback engine is Qt";
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    initConnection();
}

QtPlayer::~QtPlayer()
{
    if (m_player) {
        delete m_player;
        m_player = nullptr;
    }
    if (m_audioOutput) {
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
}

void QtPlayer::setFilePath(QString path)
{
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
    if (mediaState == QMediaPlayer::EndOfMedia)
        return VoicePlayerBase::PlayerState::Ended;
    QMediaPlayer::PlaybackState playState = m_player->playbackState();
    switch(playState) {
    case QMediaPlayer::PlayingState:
        state = VoicePlayerBase::PlayerState::Playing;
        break;
    case QMediaPlayer::PausedState:
        state = VoicePlayerBase::PlayerState::Paused;
        break;
    case QMediaPlayer::StoppedState:
        state = VoicePlayerBase::PlayerState::Stopped;
        break;
    default:
        break;
    }
    return state;
}

void QtPlayer::initConnection()
{
    connect(m_player, &QMediaPlayer::durationChanged, this, &QtPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &QtPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, [=](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia)
            emit playEnd();
    });
}
