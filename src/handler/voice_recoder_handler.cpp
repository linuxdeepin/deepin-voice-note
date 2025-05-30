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
    qDebug() << "Starting voice recorder";
    if (!checkVolume()) {
        qDebug() << "Volume check passed, confirming start";
        confirmStartRecoder();
        emit volumeTooLow(false);
    } else {
        qWarning() << "Volume too low (less than 20%)";
        emit volumeTooLow(true);
    }
}

void VoiceRecoderHandler::stopRecoder()
{
    qDebug() << "Stopping voice recorder";
    m_audioRecoder->stopRecord();
    if (m_type != RecoderType::Idle) {
        qInfo() << "Recording finished, duration:" << m_recordMsec << "ms, path:" << m_recordPath;
        emit finishedRecod(m_recordPath, m_recordMsec);
        m_type = RecoderType::Idle;
        OpsStateInterface::instance()->operState(OpsStateInterface::StateRecording, false);
    } else {
        qDebug() << "Recorder already idle";
    }
}

void VoiceRecoderHandler::pauseRecoder()
{
    qDebug() << "Toggling recorder pause state, current type:" << m_type;
    if (m_type == RecoderType::Recording) {
        qDebug() << "Pausing recording";
        m_audioRecoder->pauseRecord();
        m_type = RecoderType::Paused;
    } else {
        qDebug() << "Resuming recording";
        m_audioRecoder->startRecord();
        m_type = RecoderType::Recording;
    }
    qInfo() << "Recorder state changed to:" << m_type;
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
    qDebug() << "Initializing recording path";
    m_recordDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_recordDir += "/voicenote/";
    QDir dir(m_recordDir);
    if (!dir.exists()) {
        qDebug() << "Creating recording directory:" << m_recordDir;
        dir.mkdir(m_recordDir);
    }
    qInfo() << "Recording path initialized:" << m_recordDir;
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

// 通过脚本获取默认音源输入信息。只有获取的是所给的降噪字段的时候才使用，其他依然走dbus的方式
QString VoiceRecoderHandler::tryGetMicNameFromPactl() const
{
    QProcess process;
    QString commandOutput;

    qInfo() << "Attempting to get default source via 'pactl get-default-source'";
    process.start("pactl", QStringList() << "get-default-source");

    if (!process.waitForStarted(500)) {
        qWarning() << "'pactl get-default-source' failed to start:" << process.errorString()
        << "(Error type: " << process.error() << ")";
        return QString();
    }

    if (!process.waitForFinished(1000)) {
        qWarning() << "'pactl get-default-source' command timed out. Killing process.";
        process.kill();
        process.waitForFinished(500);
        return QString();
    }

    commandOutput = process.readAllStandardOutput().trimmed();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "'pactl get-default-source' execution failed.";
        qWarning() << "  Exit Status:" << process.exitStatus();
        qWarning() << "  Exit Code  :" << process.exitCode();
        qWarning() << "  Stdout     :" << commandOutput;
        qWarning() << "  Stderr     :" << process.readAllStandardError().trimmed();
        return QString();
    }

    if (commandOutput.isEmpty()) {
        qWarning() << "'pactl get-default-source' returned empty output.";
        return QString();
    }

    if (commandOutput.endsWith(".monitor")) {
        qWarning() << "Default source from 'pactl get-default-source' is a monitor source ("
                   << commandOutput << "). This is usually not a microphone. Will attempt D-Bus fallback.";
        return QString(); // Returning empty to trigger fallback
    }

    qInfo() << "Successfully obtained default microphone from 'pactl get-default-source':" << commandOutput;
    return commandOutput;
}

QString VoiceRecoderHandler::getDefaultMicDeviceName() const
{
    QString defaultName = tryGetMicNameFromPactl();

    if (defaultName != "echo-cancel-source") {
        defaultName = m_audioWatcher->getDeviceName(static_cast<AudioWatcher::AudioMode>(m_currentMode));
    }

    return defaultName;
}

void VoiceRecoderHandler::confirmStartRecoder()
{
    qDebug() << "Confirming start of recording";
    m_audioRecoder->setDevice(getDefaultMicDeviceName());
    QString fileName = QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + ".mp3";

    initRecordPath();
    m_recordMsec = 0;
    m_recordPath = m_recordDir + fileName;
    qDebug() << "Setting output file:" << m_recordPath;
    m_audioRecoder->setOutputFile(m_recordPath);
    
    bool ret = m_audioRecoder->startRecord();
    if (!ret) {
        qWarning() << "Failed to start recording";
        m_type = RecoderType::Idle;
        stopRecoder();
    } else {
        qInfo() << "Recording started successfully";
        m_type = RecoderType::Recording;
        OpsStateInterface::instance()->operState(OpsStateInterface::StateRecording, true);
        emit recoderStateChange(m_type);
    }
}

void VoiceRecoderHandler::onAudioDeviceChange(int mode)
{
    qDebug() << "Audio device changed, mode:" << mode;
    if (m_currentMode == mode) {
        QString info = m_audioWatcher->getDeviceName(
            static_cast<AudioWatcher::AudioMode>(mode));
        qInfo() << "Current audio device:" << info;
        
        if (info.isEmpty()) {
            qWarning() << "No audio device available";
            stopRecoder();
            updateRecordBtnState(false);
        } else {
            bool isEnable = m_audioWatcher->getDeviceEnable(static_cast<AudioWatcher::AudioMode>(m_currentMode));
            qDebug() << "Device enabled state:" << isEnable;
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
        qDebug() << "Recording time updated:" << strTime;
        emit updateRecorderTime(strTime);
        if (msec > (60 * 60 * 1000)) {
            qInfo() << "Recording reached maximum duration (1 hour), stopping";
            stopRecoder();
        }
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
