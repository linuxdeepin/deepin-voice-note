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

VTextSpeechAndTrManager::VTextSpeechAndTrManager(QObject *parent) {}

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
            QDBusConnection connection = QDBusConnection::sessionBus();
            QDBusConnectionInterface *bus = connection.interface();
            Status status = Disable;

            if (bus->isServiceRegistered("com.iflytek.aiassistant")) {
                status = Enable;
            }

            if (Enable != status) {
                // Check if install uos-ai
                if (QStandardPaths::findExecutable(kUosAiBin).isEmpty()) {
                    status = NotInstalled;
                }

                // Try to start uos-ai
                if (NotInstalled != status) {
                    QProcess process;
                    process.setProgram(kUosAiBin);
                    qint64 pid = 0;
                    if (process.startDetached(&pid)) {
                        status = Enable;
                    }

                    qInfo() << QString("Ai service not register, try to start %1, ret: %2, pid: %3")
                                   .arg(kUosAiBin)
                                   .arg(Enable == status)
                                   .arg(pid);
                } else {
                    qInfo() << QString("Ai service not register, not found %1").arg(kUosAiBin);
                }
            }

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
        QDBusMessage stopReadingMsg = QDBusMessage::createMethodCall(
            "com.iflytek.aiassistant", "/aiassistant/tts", "com.iflytek.aiassistant.tts", "isTTSInWorking");

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
    QDBusMessage voiceReadingMsg = QDBusMessage::createMethodCall(
        "com.iflytek.aiassistant", "/aiassistant/tts", "com.iflytek.aiassistant.tts", "getTTSEnable");

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
    QDBusMessage dictationMsg = QDBusMessage::createMethodCall(
        "com.iflytek.aiassistant", "/aiassistant/iat", "com.iflytek.aiassistant.iat", "getIatEnable");

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
    QDBusMessage translateReadingMsg = QDBusMessage::createMethodCall(
        "com.iflytek.aiassistant", "/aiassistant/trans", "com.iflytek.aiassistant.trans", "getTransEnable");

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
    if (Enable != status()) {
        return status();
    }

    if (!getTextToSpeechEnable()) {
        return NoOutputDevice;
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
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
        QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/tts", "com.iflytek.aiassistant.tts");
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
    if (Enable != status()) {
        return status();
    }

    if (!getSpeechToTextEnable()) {
        return NoInputDevice;
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
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
    if (Enable != status()) {
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
            return "Unknown error";
    }
}
