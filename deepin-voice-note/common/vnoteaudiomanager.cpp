#include "vnoteaudiomanager.h"

#include <QAudioDeviceInfo>

#include <DLog>

#include <iostream>
using namespace std;

VNoteAudioManager* VNoteAudioManager::_instance = nullptr;

VNoteAudioManager::VNoteAudioManager(QObject *parent)
    : QObject(parent)
    , m_pAudioPlayer(new QMediaPlayer(this))
    , m_pAudioPlayerProbe(new QAudioProbe(this))
    , m_pAudioRecord(new QAudioRecorder(this))
    , m_pAudioRecProbe(new QAudioProbe(this))
{
    initAudio();
    initConnections();
}

VNoteAudioManager *VNoteAudioManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteAudioManager();
    }

    return  _instance;
}

void VNoteAudioManager::initAudio()
{
    QAudioDeviceInfo defaultDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
    QAudioFormat perfAudioFmt = defaultDeviceInfo.preferredFormat();

    QStringList supportCodecs = m_pAudioRecord->supportedAudioCodecs();
    QStringList supportContainers = m_pAudioRecord->supportedContainers();

    //Get mp3 codec
    for (auto codec : supportCodecs) {
        if (codec.startsWith(m_defaultAudioFmt)) {
            m_audioCodec = codec;
            break;
        }
    }

    //Get mp3 codec container
    for (auto container : supportContainers) {
        if (container.startsWith(m_defaultAudioFmt)) {
            m_audioContainer = container;
        }
    }

    //Set Audio parameter
    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec(m_audioCodec);

    audioSettings.setQuality(QMultimedia::HighQuality);
    audioSettings.setEncodingMode(QMultimedia::ConstantQualityEncoding);
    audioSettings.setChannelCount(perfAudioFmt.channelCount());
    audioSettings.setSampleRate(perfAudioFmt.sampleRate());
    audioSettings.setBitRate(perfAudioFmt.sampleSize());

    //Set Audio Input
    if (m_pAudioRecord->state() == QMediaRecorder::StoppedState) {
        m_pAudioRecord->setAudioInput(m_pAudioRecord->defaultAudioInput());
        m_pAudioRecord->setEncodingSettings(audioSettings
                                            , QVideoEncoderSettings()
                                            , m_audioContainer);
    }

    m_pAudioRecProbe->setSource(m_pAudioRecord.get());
    m_pAudioPlayerProbe->setSource(m_pAudioPlayer.get());

    qInfo() << "audio settings: {\n"
            << " codec=" << audioSettings.codec()
            << ", "      << audioSettings.sampleRate()
            << ", "      << audioSettings.bitRate()
            << ", channelCount=" << audioSettings.channelCount()
            << ", container =" << m_audioContainer
            << " }";
}

void VNoteAudioManager::initConnections()
{
    connect(m_pAudioRecord.get(), &QAudioRecorder::durationChanged
            ,this, &VNoteAudioManager::recordDurationChanged);
    connect(m_pAudioRecProbe.get(), &QAudioProbe::audioBufferProbed
            ,this, [this](const QAudioBuffer &buffer) {
        emit recAudioBufferProbed(buffer);
    });

}

void VNoteAudioManager::setPlayFileName(const QString &fileName)
{
    if (m_playFileName != fileName) {
        m_playFileName = fileName;
        m_pAudioPlayer->setMedia(QUrl::fromLocalFile(m_playFileName));
    }
}

void VNoteAudioManager::startPlay()
{
    int state = m_pAudioPlayer->state();
    if (QMediaPlayer::PlayingState != state) {
        //Restore the play position in pause state
        if (QMediaPlayer::PausedState == state) {
            m_pAudioPlayer->setPosition(m_playPosition);
        }

        m_pAudioPlayer->play();
    }
}

void VNoteAudioManager::pausePlay()
{
    if (QMediaPlayer::PlayingState == m_pAudioPlayer->state()) {
        m_pAudioPlayer->pause();
        m_playPosition = m_pAudioPlayer->position();
    }
}

void VNoteAudioManager::stopPlay()
{
    if (QMediaPlayer::StoppedState != m_pAudioPlayer->state()) {
        m_pAudioPlayer->stop();
    }
}

void VNoteAudioManager::setRecordFileName(const QString &fileName)
{
    if (QMediaRecorder::StoppedState != m_pAudioRecord->state()) {
        if (m_recordFileName != fileName) {
            m_recordFileName = fileName;
            m_pAudioRecord->setOutputLocation(QUrl::fromLocalFile(m_recordFileName));
        }
    } else {
        qCritical() << "Should not change record name during recording";
    }
}

void VNoteAudioManager::startRecord()
{
    if (QMediaRecorder::RecordingState != m_pAudioRecord->state()) {
        m_pAudioRecord->record();
    }
}

void VNoteAudioManager::pauseRecord()
{
    if (QMediaRecorder::RecordingState == m_pAudioRecord->state()) {
        m_pAudioRecord->pause();
    }
}

void VNoteAudioManager::stopRecord()
{
    if (QMediaRecorder::StoppedState != m_pAudioRecord->state()) {
        m_pAudioRecord->stop();
        m_recordFileName.clear();
    }
}

void VNoteAudioManager::recordDurationChanged(qint64 duration)
{
    if (duration >= MAX_REC_TIME_INMSEC) {
        stopRecord();
        emit recExceedLimit();
    }
}
