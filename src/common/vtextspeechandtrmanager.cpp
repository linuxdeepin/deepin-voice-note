/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "vtextspeechandtrmanager.h"

#include <QDebug>
#include <QDBusReply>
#include <QDBusConnection>
#include <QProcess>


// 检测是否在朗读文本
bool VTextSpeechAndTrManager::isTextToSpeechInWorking()
{
    QDBusMessage stopReadingMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                      "/aiassistant/tts",
                                                      "com.iflytek.aiassistant.tts",
                                                      "isTTSInWorking");

    QDBusReply<bool> stopReadingStateRet = QDBusConnection::sessionBus().call(stopReadingMsg, QDBus::BlockWithGui);
    if (stopReadingStateRet.isValid()) {
        return stopReadingStateRet.value();
    } else {
        return false;
    }
}

// 检测语音朗读开关是否打开
bool VTextSpeechAndTrManager::getTextToSpeechEnable()
{
    QDBusMessage voiceReadingMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                      "/aiassistant/tts",
                                                      "com.iflytek.aiassistant.tts",
                                                      "getTTSEnable");

    QDBusReply<bool> voiceReadingStateRet = QDBusConnection::sessionBus().call(voiceReadingMsg, QDBus::BlockWithGui);
    if (voiceReadingStateRet.isValid()) {
        return voiceReadingStateRet.value();
    } else {
        return false;
    }
}

// 检测语音听写开关是否打开
bool VTextSpeechAndTrManager::getSpeechToTextEnable()
{
    QDBusMessage dictationMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                      "/aiassistant/iat",
                                                      "com.iflytek.aiassistant.iat",
                                                      "getIatEnable");

    QDBusReply<bool> dictationStateRet = QDBusConnection::sessionBus().call(dictationMsg, QDBus::BlockWithGui);
    if (dictationStateRet.isValid()) {
        return dictationStateRet.value();
    } else {
        return false;
    }
}

// 检测文本翻译开关是否打开
bool VTextSpeechAndTrManager::getTransEnable()
{
    QDBusMessage translateReadingMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                      "/aiassistant/trans",
                                                      "com.iflytek.aiassistant.trans",
                                                      "getTransEnable");

    QDBusReply<bool> translateStateRet = QDBusConnection::sessionBus().call(translateReadingMsg, QDBus::BlockWithGui);
    if (translateStateRet.isValid()) {
        return translateStateRet.value();
    } else {
        return false;
    }
}

// 语音朗读
void VTextSpeechAndTrManager::onTextToSpeech()
{
    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain com.iflytek.aiassistant.mainWindow.TextToSpeech");
}

// 停止语音朗读
void VTextSpeechAndTrManager::onStopTextToSpeech()
{
    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/tts com.iflytek.aiassistant.tts.stopTTSDirectly");
}

// 语音听写
void VTextSpeechAndTrManager::onSpeechToText()
{
    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain com.iflytek.aiassistant.mainWindow.SpeechToText");
}

// 文本翻译
void VTextSpeechAndTrManager::onTextTranslate()
{
    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain com.iflytek.aiassistant.mainWindow.TextToTranslate");
}
