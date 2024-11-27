#include "voice_recoder_handler.h"
#include "audio/gstreamrecorder.h"
#include "audio/audio_watcher.h"
#include "utils.h"
#include "opsstateinterface.h"

VoiceRecoderHandler::VoiceRecoderHandler() {
    intRecoder();
    initAudioWatcher();
}


VoiceRecoderHandler *VoiceRecoderHandler::instance()
{
    static VoiceRecoderHandler voiceHandler;
    return &voiceHandler;
}

VoiceRecoderHandler::RecoderType VoiceRecoderHandler::getRecoderType()
{
    return m_type;
}

void VoiceRecoderHandler::startRecoder()
{
    if (!checkVolume()) {
        confirmStartRecoder();
        emit volumeTooLow(false);
    } else {
        qInfo() << "Volume less than 20%";
        emit volumeTooLow(true);
    }
}

void VoiceRecoderHandler::stopRecoder()
{
    m_audioRecoder->stopRecord();
    if (m_type != RecoderType::Idle) {
        emit finishedRecod(m_recordPath, m_recordMsec);
        m_type = RecoderType::Idle;
        OpsStateInterface::instance()->operState(OpsStateInterface::StateRecording, false);
    }
}

void VoiceRecoderHandler::pauseRecoder()
{
    if (m_type == RecoderType::Recording) {
        m_audioRecoder->pauseRecord();
        m_type = RecoderType::Paused;
    } else {
        m_audioRecoder->startRecord();
        m_type = RecoderType::Recording;
    }
    emit recoderStateChange(m_type);
}

void VoiceRecoderHandler::setAudioDevice(const QString &device)
{

}

void VoiceRecoderHandler::changeMode(const int &mode)
{
    m_currentMode = mode;
    onAudioDeviceChange(m_currentMode);
}

void VoiceRecoderHandler::onDeviceEnableChanged(int mode, bool enabled)
{
    if (m_currentMode == mode) {
        emit updateRecordBtnState(enabled);
    }
}

void VoiceRecoderHandler::intRecoder()
{
    m_audioRecoder = new GstreamRecorder(this);
    connect(m_audioRecoder, &GstreamRecorder::audioBufferProbed, this, &VoiceRecoderHandler::onAudioBufferProbed);
}

void VoiceRecoderHandler::initRecordPath()
{
    m_recordDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_recordDir += "/voicenote/";
    QDir dir(m_recordDir);
    if (!dir.exists()) {
        dir.mkdir(m_recordDir);
    }
}

void VoiceRecoderHandler::initAudioWatcher()
{
    m_audioWatcher = new AudioWatcher(this);
    connect(m_audioWatcher, &AudioWatcher::sigDeviceEnableChanged, this, &VoiceRecoderHandler::onDeviceEnableChanged);
}

bool VoiceRecoderHandler::checkVolume()
{
    double volume = m_audioWatcher->getVolume(
        static_cast<AudioWatcher::AudioMode>(m_currentMode));
    return (volume - 0.2 < 0.0) ? true : false;
}

void VoiceRecoderHandler::confirmStartRecoder()
{
    m_audioRecoder->setDevice(m_audioWatcher->getDeviceName(static_cast<AudioWatcher::AudioMode>(m_currentMode)));
    QString fileName = QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + ".mp3";

    initRecordPath();
    m_recordMsec = 0;
    m_recordPath = m_recordDir + fileName;
    m_audioRecoder->setOutputFile(m_recordPath);
    bool ret = m_audioRecoder->startRecord();
    if (!ret) {
        m_type = RecoderType::Idle;
        stopRecoder();
    } else {
        m_type = RecoderType::Recording;
        OpsStateInterface::instance()->operState(OpsStateInterface::StateRecording, true);
        emit recoderStateChange(m_type);
    }
}

void VoiceRecoderHandler::onAudioDeviceChange(int mode)
{
    if (m_currentMode == mode) {
        QString info = m_audioWatcher->getDeviceName(
            static_cast<AudioWatcher::AudioMode>(mode));
        qInfo() << "Current Audio Device Name: " << info;
        if (info.isEmpty()) {
            stopRecoder();
            updateRecordBtnState(false);
        } else {
            bool isEnable = m_audioWatcher->getDeviceEnable(static_cast<AudioWatcher::AudioMode>(m_currentMode));
            stopRecoder();
            updateRecordBtnState(isEnable);
        }
    }
}

void VoiceRecoderHandler::onAudioBufferProbed(const QAudioBuffer &buffer)
{
    qint64 msec = buffer.startTime();
    if (m_recordMsec != msec) {
        m_recordMsec = msec;
        QString strTime = Utils::formatMillisecond(msec, 0);
        emit updateRecorderTime(strTime);
        if (msec > (60 * 60 * 1000))
            stopRecoder();
    }
    qreal maxValue = -100;
    for (int i = 0; i < buffer.frameCount(); i++) {
        qreal averageValue = 0;
        int channel = buffer.format().channelCount();
        for (int j = 0; j < channel; ++j) {
            averageValue += qAbs(qreal(buffer.constData<qint16>()[i * channel + j]));
        }
        averageValue = (averageValue) / channel;
        if (maxValue < averageValue)
            maxValue = averageValue;
    }
    maxValue = maxValue / 10000;
    updateWave(maxValue);
}
