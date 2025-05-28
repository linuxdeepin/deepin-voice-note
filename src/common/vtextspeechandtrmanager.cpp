// Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vtextspeechandtrmanager.h"

#include <mutex>

#include <QDebug>
#include <QDBusReply>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QProcess>
#include <QThreadPool>
#include <QStandardPaths>

static const QString kUosAiBin = "uos-ai-assistant";
static const QString kCopilotService = "com.deepin.copilot";
static const QString kCopilotPath = "/com/deepin/copilot";
static const QString kCopilotInterface = "com.deepin.copilot";
static const QString kFlytekService = "com.iflytek.aiassistant";

VTextSpeechAndTrManager::VTextSpeechAndTrManager(QObject *parent)
    : QObject(parent)
    , m_copilot{new QDBusInterface(kCopilotService, kCopilotPath, kCopilotInterface)}
{
    m_copilot->setTimeout(2000);
}

VTextSpeechAndTrManager *VTextSpeechAndTrManager::instance()
{
    static VTextSpeechAndTrManager ins;
    return &ins;
}

void VTextSpeechAndTrManager::checkUosAiExists()
{
    static std::once_flag kAiFlag;
    std::call_once(kAiFlag, [this]() {
        auto initFunc = []() {
            qDebug() << "Checking UOS AI backend status";
            // If call dbus interface success, the uos-ai backend process started.
            auto copilot = QSharedPointer<QDBusInterface>::create(kCopilotService, kCopilotPath, kCopilotInterface);
            Status status = copilotInstalled(copilot);
            if (Enable == status) {
                status = isCopilotEnabled(copilot);
            }

            qInfo() << QString("UOS AI backend status: %1(%2)").arg(Enable == status).arg(status);

            // call on non-gui thread, so queued connection.
            auto ins = VTextSpeechAndTrManager::instance();
            QMetaObject::invokeMethod(
                ins, [status, ins]() { ins->m_status = status; }, Qt::QueuedConnection);
        };

        // run on non-gui thread.
        QThreadPool::globalInstance()->start(std::move(initFunc), QThread::TimeCriticalPriority);
    });
}

/**
 * @brief VTextSpeechAndTrManager::isTextToSpeechInWorking
 * @return true 正在朗读
 */
bool VTextSpeechAndTrManager::isTextToSpeechInWorking()
{
    if (m_isSpeeching) {
        qDebug() << "Checking text-to-speech working status";
        QDBusMessage stopReadingMsg =
            QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/tts", "com.iflytek.aiassistant.tts", "isTTSInWorking");

        QDBusReply<bool> stopReadingStateRet = QDBusConnection::sessionBus().call(stopReadingMsg, QDBus::BlockWithGui);
        if (stopReadingStateRet.isValid()) {
            m_isSpeeching = stopReadingStateRet.value();
            qDebug() << "Text-to-speech status:" << m_isSpeeching;
        } else {
            qWarning() << "Failed to get text-to-speech status:" << stopReadingStateRet.error().message();
            m_isSpeeching = false;
        }
    }

    return m_isSpeeching;
}

/**
 * @brief VTextSpeechAndTrManager::getTextToSpeechEnable
 * @return true 朗读功能可用
 */
bool VTextSpeechAndTrManager::getTextToSpeechEnable()
{
    qDebug() << "Checking text-to-speech enable status";
    QDBusMessage voiceReadingMsg =
        QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/tts", "com.iflytek.aiassistant.tts", "getTTSEnable");

    QDBusReply<bool> voiceReadingStateRet = QDBusConnection::sessionBus().call(voiceReadingMsg, QDBus::BlockWithGui);
    if (voiceReadingStateRet.isValid()) {
        qDebug() << "Text-to-speech enabled:" << voiceReadingStateRet.value();
        return voiceReadingStateRet.value();
    } else {
        qWarning() << "Failed to get text-to-speech enable status:" << voiceReadingStateRet.error().message();
        return false;
    }
}

/**
 * @brief VTextSpeechAndTrManager::getSpeechToTextEnable
 * @return true 听写功能可用
 */
bool VTextSpeechAndTrManager::getSpeechToTextEnable()
{
    qDebug() << "Checking speech-to-text enable status";
    QDBusMessage dictationMsg =
        QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/iat", "com.iflytek.aiassistant.iat", "getIatEnable");

    QDBusReply<bool> dictationStateRet = QDBusConnection::sessionBus().call(dictationMsg, QDBus::BlockWithGui);
    if (dictationStateRet.isValid()) {
        qDebug() << "Speech-to-text enabled:" << dictationStateRet.value();
        return dictationStateRet.value();
    } else {
        qWarning() << "Failed to get speech-to-text enable status:" << dictationStateRet.error().message();
        return false;
    }
}

/**
 * @brief VTextSpeechAndTrManager::getTransEnable
 * @return true 翻译功能可用
 */
bool VTextSpeechAndTrManager::getTransEnable()
{
    qDebug() << "Checking translation enable status";
    QDBusMessage translateReadingMsg =
        QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/trans", "com.iflytek.aiassistant.trans", "getTransEnable");

    QDBusReply<bool> translateStateRet = QDBusConnection::sessionBus().call(translateReadingMsg, QDBus::BlockWithGui);
    if (translateStateRet.isValid()) {
        qDebug() << "Translation enabled:" << translateStateRet.value();
        return translateStateRet.value();
    } else {
        qWarning() << "Failed to get translation enable status:" << translateStateRet.error().message();
        return false;
    }
}

/**
 * @brief VTextSpeechAndTrManager::onTextToSpeech
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onTextToSpeech()
{
    qDebug() << "Starting text-to-speech operation";
    if (Enable != checkValid()) {
        qWarning() << "Text-to-speech operation failed: Invalid status" << status();
        return status();
    }

    if (!getTextToSpeechEnable()) {
        qWarning() << "Text-to-speech operation failed: No output device";
        return NoOutputDevice;
    }

    QDBusInterface interface(kCopilotService, "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    if (!interface.isValid()) {
        qWarning() << "Text-to-speech operation failed: Invalid interface";
        return Failed;
    }

    // check if speech now
    if (isTextToSpeechInWorking()) {
        qDebug() << "Stopping current text-to-speech before starting new one";
        onStopTextToSpeech();
    }

    interface.asyncCall("TextToSpeech");
    m_isSpeeching = true;
    qDebug() << "Text-to-speech operation started successfully";
    return Success;
}

/**
 * @brief VTextSpeechAndTrManager::onStopTextToSpeech
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onStopTextToSpeech()
{
    qDebug() << "Stopping text-to-speech operation";
    if (Enable != status()) {
        qWarning() << "Stop text-to-speech failed: Invalid status" << status();
        return status();
    }

    if (m_isSpeeching) {
        QDBusInterface interface(kCopilotService, "/aiassistant/tts", "com.iflytek.aiassistant.tts");
        if (!interface.isValid()) {
            qWarning() << "Stop text-to-speech failed: Invalid interface";
            return Failed;
        }

        interface.asyncCall("stopTTSDirectly");
        m_isSpeeching = false;
        qDebug() << "Text-to-speech stopped successfully";
    }

    return Success;
}

/**
 * @brief VTextSpeechAndTrManager::onSpeechToText
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onSpeechToText()
{
    qDebug() << "Starting speech-to-text operation";
    if (Enable != checkValid()) {
        qWarning() << "Speech-to-text operation failed: Invalid status" << status();
        return status();
    }

    if (!getSpeechToTextEnable()) {
        qWarning() << "Speech-to-text operation failed: No input device";
        return NoInputDevice;
    }

    QDBusInterface interface(kCopilotService, "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    if (!interface.isValid()) {
        qWarning() << "Speech-to-text operation failed: Invalid interface";
        return Failed;
    }

    interface.asyncCall("SpeechToText");
    qDebug() << "Speech-to-text operation started successfully";
    return Success;
}

/**
 * @brief VTextSpeechAndTrManager::onTextTranslate
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onTextTranslate()
{
    qDebug() << "Starting text translation operation";
    if (Enable != checkValid()) {
        qWarning() << "Text translation failed: Invalid status" << status();
        return status();
    }

    if (!getTransEnable()) {
        qWarning() << "Text translation failed: Translation not enabled";
        return Failed;
    }

    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain "
                            "com.iflytek.aiassistant.mainWindow.TextToTranslate");
    qDebug() << "Text translation operation started successfully";
    return Success;
}

QString VTextSpeechAndTrManager::errorString(Status status)
{
    switch (status) {
        case NotInstalled:
            return QObject::tr("Please install 'UOS AI' from the App Store before using");
        case NoInputDevice:
            return QObject::tr("No audio input device detected. Please check and try again");
        case NoOutputDevice:
            return QObject::tr("No audio output device detected. Please check and try again");
        default:
            // Unknown error
            return {};
    }
}

VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::checkValid()
{
    qDebug() << "Checking UOS AI validity, current status:" << m_status;
    switch (m_status) {
        case NotInstalled:
            m_status = copilotInstalled(m_copilot);
            if (Enable != m_status) {
                qWarning() << "UOS AI not installed";
                break;
            }
            // If uos-ai is installed, it detects whether the user agrees to the user agreement.
            Q_FALLTHROUGH();
        case NoUserAgreement: {
            // Detect current state every call
            m_status = isCopilotEnabled(m_copilot);
            if (NoUserAgreement == m_status) {
                qInfo() << "Launching UOS AI chat page for user agreement";
                launchCopilotChat(m_copilot);
            }
        } break;
        default:
            break;
    }

    return m_status;
}

VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::copilotInstalled(const QSharedPointer<QDBusInterface> &copilot)
{
    qDebug() << "Checking if UOS AI is installed";
    QDBusReply<QString> version = copilot->call("version");
    if (version.isValid()) {
        qInfo() << "UOS AI installed, version:" << version.value();
        return Enable;
    }

    qWarning() << "UOS AI installation check failed:" << version.error().message();
    return NotInstalled;
}

VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::isCopilotEnabled(const QSharedPointer<QDBusInterface> &copilot)
{
    qDebug() << "Checking if UOS AI is enabled";
    QDBusReply<bool> state = copilot->call("isCopilotEnabled");
    if (state.isValid()) {
        qInfo() << "UOS AI enabled status:" << state.value();
        return state.value() ? Enable : NoUserAgreement;
    }

    // NOTE: Adapt old version, if dbus interface not valid, assume the user agreement agreed.
    qWarning() << "Failed to get UOS AI enabled status:" << state.error().message();
    return Enable;
}

VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::launchCopilotChat(const QSharedPointer<QDBusInterface> &copilot)
{
    qDebug() << "Launching UOS AI chat page";
    QDBusMessage message = copilot->call("launchChatPage");
    if (!message.errorMessage().isEmpty()) {
        qWarning() << "Failed to launch UOS AI chat page:" << message.errorMessage();
        return Disable;
    }

    qInfo() << "UOS AI chat page launched successfully";
    return Enable;
}
