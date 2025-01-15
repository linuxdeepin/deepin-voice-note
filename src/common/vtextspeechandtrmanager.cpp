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
            // If call dbus interface success, the uos-ai backend process started.
            auto copilot = QSharedPointer<QDBusInterface>::create(kCopilotService, kCopilotPath, kCopilotInterface);
            Status status = copilotInstalled(copilot);
            if (Enable == status) {
                status = isCopilotEnabled(copilot);
            }

            qInfo() << QString("backend uos-ai status: %1(%2)").arg(Enable == status).arg(status);

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
        QDBusMessage stopReadingMsg =
            QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/tts", "com.iflytek.aiassistant.tts", "isTTSInWorking");

        QDBusReply<bool> stopReadingStateRet = QDBusConnection::sessionBus().call(stopReadingMsg, QDBus::BlockWithGui);
        if (stopReadingStateRet.isValid()) {
            m_isSpeeching = stopReadingStateRet.value();
        } else {
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
    QDBusMessage voiceReadingMsg =
        QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/tts", "com.iflytek.aiassistant.tts", "getTTSEnable");

    QDBusReply<bool> voiceReadingStateRet = QDBusConnection::sessionBus().call(voiceReadingMsg, QDBus::BlockWithGui);
    if (voiceReadingStateRet.isValid()) {
        return voiceReadingStateRet.value();
    } else {
        return false;
    }
}

/**
 * @brief VTextSpeechAndTrManager::getSpeechToTextEnable
 * @return true 听写功能可用
 */
bool VTextSpeechAndTrManager::getSpeechToTextEnable()
{
    QDBusMessage dictationMsg =
        QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/iat", "com.iflytek.aiassistant.iat", "getIatEnable");

    QDBusReply<bool> dictationStateRet = QDBusConnection::sessionBus().call(dictationMsg, QDBus::BlockWithGui);
    if (dictationStateRet.isValid()) {
        return dictationStateRet.value();
    } else {
        return false;
    }
}

/**
 * @brief VTextSpeechAndTrManager::getTransEnable
 * @return true 翻译功能可用
 */
bool VTextSpeechAndTrManager::getTransEnable()
{
    QDBusMessage translateReadingMsg =
        QDBusMessage::createMethodCall(kCopilotService, "/aiassistant/trans", "com.iflytek.aiassistant.trans", "getTransEnable");

    QDBusReply<bool> translateStateRet = QDBusConnection::sessionBus().call(translateReadingMsg, QDBus::BlockWithGui);
    if (translateStateRet.isValid()) {
        return translateStateRet.value();
    } else {
        return false;
    }
}

/**
 * @brief VTextSpeechAndTrManager::onTextToSpeech
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onTextToSpeech()
{
    if (Enable != checkValid()) {
        return status();
    }

    if (!getTextToSpeechEnable()) {
        return NoOutputDevice;
    }

    QDBusInterface interface(kCopilotService, "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    if (!interface.isValid()) {
        return Failed;
    }

    // check if speech now
    if (isTextToSpeechInWorking()) {
        onStopTextToSpeech();
    }

    interface.asyncCall("TextToSpeech");
    m_isSpeeching = true;
    return Success;
}

/**
 * @brief VTextSpeechAndTrManager::onStopTextToSpeech
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onStopTextToSpeech()
{
    if (Enable != status()) {
        return status();
    }

    if (m_isSpeeching) {
        QDBusInterface interface(kCopilotService, "/aiassistant/tts", "com.iflytek.aiassistant.tts");
        if (!interface.isValid()) {
            return Failed;
        }

        interface.asyncCall("stopTTSDirectly");
        m_isSpeeching = false;
    }

    return Success;
}

/**
 * @brief VTextSpeechAndTrManager::onSpeechToText
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onSpeechToText()
{
    if (Enable != checkValid()) {
        return status();
    }

    if (!getSpeechToTextEnable()) {
        return NoInputDevice;
    }

    QDBusInterface interface(kCopilotService, "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    if (!interface.isValid()) {
        return Failed;
    }

    interface.asyncCall("SpeechToText");
    return Success;
}

/**
 * @brief VTextSpeechAndTrManager::onTextTranslate
 */
VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::onTextTranslate()
{
    if (Enable != checkValid()) {
        return status();
    }

    if (!getTransEnable()) {
        return Failed;
    }

    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain "
                            "com.iflytek.aiassistant.mainWindow.TextToTranslate");

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
    switch (m_status) {
        case NotInstalled:
            m_status = copilotInstalled(m_copilot);
            if (Enable != m_status) {
                break;
            }
            // If uos-ai is installed, it detects whether the user agrees to the user agreement.
            Q_FALLTHROUGH();
        case NoUserAgreement: {
            // Detect current state every call
            m_status = isCopilotEnabled(m_copilot);
            if (NoUserAgreement == m_status) {
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
    QDBusReply<QString> version = copilot->call("version");
    if (version.isValid()) {
        qInfo() << "Installed uos-ai, current version:" << version.value();
        return Enable;
    }

    qWarning() << "Query uos-ai installed faild! Maybe need install" << version.error().message();
    return NotInstalled;
}

VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::isCopilotEnabled(const QSharedPointer<QDBusInterface> &copilot)
{
    QDBusReply<bool> state = copilot->call("isCopilotEnabled");
    if (state.isValid()) {
        return state.value() ? Enable : NoUserAgreement;
    }

    // NOTE: Adapt old version, if dbus interface not valid, assume the user agreement agreed.
    qWarning() << "Query uos-ai user exp state failed!" << state.error().message();
    return Enable;
}

VTextSpeechAndTrManager::Status VTextSpeechAndTrManager::launchCopilotChat(const QSharedPointer<QDBusInterface> &copilot)
{
    QDBusMessage message = copilot->call("launchChatPage");
    if (!message.errorMessage().isEmpty()) {
        qWarning() << "Launch uos-ai chat page failed!" << message.errorMessage();
        return Disable;
    }

    return Enable;
}
