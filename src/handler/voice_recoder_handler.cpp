#include "voice_recoder_handler.h"
#include "audio/gstreamrecorder.h"
#include "audio/audio_watcher.h"
#include "common/utils.h"
#include "common/VNoteMainManager.h"
#include <QAudioBuffer>
#include "opsstateinterface.h"
// 条件编译：根据 Qt 版本包含不同的音频设备头文件
#ifdef USE_QT5
#include <QAudioDeviceInfo>
#else
#include <QMediaDevices>
#include <QAudioDevice>
#endif
#include <QDebug>

VoiceRecoderHandler::VoiceRecoderHandler() {
    qInfo() << "VoiceRecoderHandler constructor called";
#ifndef USE_QT5
    m_mediaDevices = new QMediaDevices(this);
#endif
    intRecoder();
    initAudioWatcher();
    qInfo() << "VoiceRecoderHandler constructor finished";
}

VoiceRecoderHandler *VoiceRecoderHandler::instance()
{
    // qInfo() << "VoiceRecoderHandler instance requested";
    static VoiceRecoderHandler voiceHandler;
    return &voiceHandler;
}

VoiceRecoderHandler::RecoderType VoiceRecoderHandler::getRecoderType()
{
    // qInfo() << "Getting recorder type:" << m_type;
    return m_type;
}

void VoiceRecoderHandler::startRecoder()
{
    qInfo() << "Starting voice recorder";
    if (VNoteMainManager::instance()->isInSearchMode()) {
        qDebug() << "Cannot start recording while in search mode";
        return;
    }
    
    qDebug() << "Starting voice recorder";
    if (!checkVolume()) {
        qDebug() << "Volume check passed, confirming start";
        confirmStartRecoder();
        emit volumeTooLow(false);
    } else {
        qWarning() << "Volume too low (less than 20%)";
        emit volumeTooLow(true);
    }
    qInfo() << "Voice recorder start finished";
}

void VoiceRecoderHandler::stopRecoder()
{
    qDebug() << "Stopping voice recorder, current file:" << m_recordPath;
    m_audioRecoder->stopRecord();
    if (m_type != RecoderType::Idle) {
        qInfo() << "Recording finished, duration:" << m_recordMsec << "ms, path:" << m_recordPath;
        emit finishedRecod(m_recordPath, m_recordMsec);
        m_type = RecoderType::Idle;
        OpsStateInterface::instance()->operState(OpsStateInterface::StateRecording, false);
        updateWave(0.0);
    } else {
        qDebug() << "Recorder already idle";
    }
    qInfo() << "Voice recorder stop finished";
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
    qInfo() << "Setting audio device to:" << device;
}

void VoiceRecoderHandler::changeMode(const int &mode)
{
    qInfo() << "Changing mode to:" << mode;
    m_currentMode = mode;
    onAudioDeviceChange(m_currentMode);
    qInfo() << "Mode change finished";
}

void VoiceRecoderHandler::onDeviceEnableChanged(int mode, bool enabled)
{
    qInfo() << "Device enable state changed, mode:" << mode << "enabled:" << enabled;
    if (m_currentMode == mode) {
        qInfo() << "mode is equal to current mode";
        emit updateRecordBtnState(enabled);
    }
    qInfo() << "Device enable state change handling finished";
}

void VoiceRecoderHandler::intRecoder()
{
    qInfo() << "Initializing recorder";
    m_audioRecoder = new GstreamRecorder(this);
    connect(m_audioRecoder, &GstreamRecorder::audioBufferProbed, this, &VoiceRecoderHandler::onAudioBufferProbed);
    qInfo() << "Recorder initialization finished";
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
    qInfo() << "Initializing audio watcher";
    m_audioWatcher = new AudioWatcher(this);
    connect(m_audioWatcher, &AudioWatcher::sigDeviceEnableChanged, this, &VoiceRecoderHandler::onDeviceEnableChanged);
    connect(m_audioWatcher, &AudioWatcher::sigReduceNoiseChanged, this, &VoiceRecoderHandler::onReduceNoiseChanged);
    qInfo() << "Audio watcher initialization finished";
}

bool VoiceRecoderHandler::checkVolume()
{
    qInfo() << "Checking volume for mode:" << m_currentMode;
    double volume = m_audioWatcher->getVolume(
        static_cast<AudioWatcher::AudioMode>(m_currentMode));
    return (volume - 0.2 < 0.0) ? true : false;
}

// 通过脚本获取默认音源输入信息。只有获取的是所给的降噪字段的时候才使用，其他依然走dbus的方式
QString VoiceRecoderHandler::tryGetMicNameFromPactl() const
{
    qInfo() << "Attempting to get default source via 'pactl get-default-source'";
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
    qInfo() << "Getting default mic device name";
    QString defaultName;

    // 只有当m_currentMode是麦克风模式时，才尝试使用pactl获取默认音源
    if (static_cast<AudioWatcher::AudioMode>(m_currentMode) == AudioWatcher::Micphone) {
        defaultName = tryGetMicNameFromPactl();
        if (defaultName == "echo-cancel-source") {
            // 如果pactl获取到有效且非降噪字段，则使用它
            qInfo() << "Default mic device name retrieval finished";
            return defaultName;
        }
    }

    // 否则，回退到使用m_audioWatcher获取设备名称
    defaultName = m_audioWatcher->getDeviceName(static_cast<AudioWatcher::AudioMode>(m_currentMode));
    qInfo() << "Default mic device name retrieval finished";
    return defaultName;
}

void VoiceRecoderHandler::confirmStartRecoder()
{
    qDebug() << "Confirming start of recording";
    m_audioRecoder->setDevice(getDefaultMicDeviceName());
    // 将文件名控制更加精细，避免文件名冲突
    QString fileName = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz") + ".mp3";

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
    qInfo() << "Recording confirmation finished";
}

void VoiceRecoderHandler::onAudioDeviceChange(int mode)
{
    qInfo() << "Audio device changed, mode:" << mode;
    // qCritical() << "=== onAudioDeviceChange验证 === 音频设备变化，模式:" << mode;
    // qCritical() << "当前录音状态:" << m_type;
    
    qDebug() << "Audio device changed, mode:" << mode;
    if (m_currentMode == mode) {
        QString info = m_audioWatcher->getDeviceName(
            static_cast<AudioWatcher::AudioMode>(mode));
        qInfo() << "Current audio device:" << info;
        
        if (info.isEmpty()) {
            qWarning() << "No audio device available";
            stopRecoder();
            updateRecordBtnState(false);
            updateWave(0.0);
        } else {
            bool isEnable = m_audioWatcher->getDeviceEnable(static_cast<AudioWatcher::AudioMode>(m_currentMode));
            qDebug() << "Device enabled state:" << isEnable;
            
            // 如果正在录音，需要完全停止录音并通知UI关闭界面
            if (m_type != RecoderType::Idle) {
                stopRecoder();
                emit recoderStateChange(RecoderType::Idle);
            }
            updateRecordBtnState(isEnable);
            // 此时停止也需要将波形曲线归零，否则可能会在停止录制之后，波形曲线依然显示
            updateWave(0.0);
        }
    }
    qInfo() << "Audio device change handling finished";
}

void VoiceRecoderHandler::onAudioBufferProbed(const QAudioBuffer &buffer)
{
    qInfo() << "Audio buffer probed";
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
    qInfo() << "Audio buffer probe handling finished";
}

void VoiceRecoderHandler::onReduceNoiseChanged(bool reduceNoiseChanged)
{
    qInfo() << "Reduce noise changed to:" << reduceNoiseChanged;
    onAudioDeviceChange(m_currentMode);
    qInfo() << "Reduce noise change handling finished";
}
