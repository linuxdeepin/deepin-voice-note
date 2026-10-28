#include "qtplayer.h"
#include "ddlog.h"

#include <QDebug>

QtPlayer::QtPlayer(QObject *parent)
    : VoicePlayerBase(parent)
{
    qCInfo(voiceNoteLog) << "Initializing QtPlayer";
    m_player = new QMediaPlayer(this);
    
#ifdef USE_QT5
    // Qt5 版本：QAudioOutput 构造函数需要 QAudioFormat 参数
    m_audioOutput = new QAudioOutput(QAudioFormat(), this);
#else
    // Qt6 版本：QAudioOutput 构造函数只需要 parent 参数
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
#endif
    
    initConnection();
}

QtPlayer::~QtPlayer()
{
    qCDebug(voiceNoteLog) << "Destroying QtPlayer";
    if (m_player) {
        delete m_player;
        m_player = nullptr;
        qCDebug(voiceNoteLog) << "Media player deleted";
    }
    if (m_audioOutput) {
        delete m_audioOutput;
        m_audioOutput = nullptr;
        qCDebug(voiceNoteLog) << "Audio output deleted";
    }
}

void QtPlayer::setFilePath(QString path)
{
    qCDebug(voiceNoteLog) << "Setting media file path:" << path;
#ifdef USE_QT5
    // Qt5 版本使用 setMedia
    m_player->setMedia(QUrl(path));
#else
    // Qt6 版本使用 setSource
    m_player->setSource(QUrl(path));
#endif
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
        qCDebug(voiceNoteLog) << "Media playback reached end";
        return VoicePlayerBase::PlayerState::Ended;
    }
    
#ifdef USE_QT5
    // Qt5 版本使用 state() 方法
    QMediaPlayer::State playState = m_player->state();
    switch(playState) {
    case QMediaPlayer::PlayingState:
        state = VoicePlayerBase::PlayerState::Playing;
        qCDebug(voiceNoteLog) << "Player state: Playing";
        break;
    case QMediaPlayer::PausedState:
        state = VoicePlayerBase::PlayerState::Paused;
        qCDebug(voiceNoteLog) << "Player state: Paused";
        break;
    case QMediaPlayer::StoppedState:
        state = VoicePlayerBase::PlayerState::Stopped;
        qCDebug(voiceNoteLog) << "Player state: Stopped";
        break;
    default:
        qCDebug(voiceNoteLog) << "Player state: Unknown";
        break;
    }
#else
    // Qt6 版本使用 playbackState() 方法
    QMediaPlayer::PlaybackState playState = m_player->playbackState();
    switch(playState) {
    case QMediaPlayer::PlayingState:
        state = VoicePlayerBase::PlayerState::Playing;
        qCDebug(voiceNoteLog) << "Player state: Playing";
        break;
    case QMediaPlayer::PausedState:
        state = VoicePlayerBase::PlayerState::Paused;
        qCDebug(voiceNoteLog) << "Player state: Paused";
        break;
    case QMediaPlayer::StoppedState:
        state = VoicePlayerBase::PlayerState::Stopped;
        qCDebug(voiceNoteLog) << "Player state: Stopped";
        break;
    default:
        qCDebug(voiceNoteLog) << "Player state: Unknown";
        break;
    }
#endif
    return state;
}

void QtPlayer::initConnection()
{
    connect(m_player, &QMediaPlayer::durationChanged, this, &QtPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &QtPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, [=](QMediaPlayer::MediaStatus status){
        qCDebug(voiceNoteLog) << "Media status changed:" << status;
        if (status == QMediaPlayer::EndOfMedia) {
            qCDebug(voiceNoteLog) << "Emitting playEnd signal";
            emit playEnd();
        }
    });
}
