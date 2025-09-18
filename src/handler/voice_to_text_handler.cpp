// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice_to_text_handler.h"

#include <QTimer>
#include <QDBusInterface>

#include "globaldef.h"
#include "common/vnoteitem.h"
#include "common/vnotea2tmanager.h"
#include "common/jscontent.h"
#include "opsstateinterface.h"

/**
 * @class VoiceToTextHandler
 * @brief 处理语音转文字的类，连接 JsContent 和 VNoteA2TManager ，处理音频数据并向 JS 前端提供音频数据转换结果
 */
VoiceToTextHandler::VoiceToTextHandler(QObject *parent)
    : QObject { parent }
    , m_a2tManager(new VNoteA2TManager(this))
{
    qInfo() << "VoiceToTextHandler constructor called";
    connect(m_a2tManager, &VNoteA2TManager::asrError, this, &VoiceToTextHandler::onA2TError);
    connect(m_a2tManager, &VNoteA2TManager::asrSuccess, this, &VoiceToTextHandler::onA2TSuccess);
}

void VoiceToTextHandler::setAudioToText(const QSharedPointer<VNVoiceBlock> &voiceBlock)
{
    qDebug() << "Setting audio for text conversion";
    m_voiceBlock = voiceBlock;
    if (!m_voiceBlock) {
        qWarning() << "Voice block is null";
        return;
    }

    if (!checkNetworkState()) {
        qWarning() << "No network connection available";
        Q_EMIT noNetworkConnection();
        return;
    }

    // 超过20分钟的语音不支持转文字
    if (m_voiceBlock->voiceSize > MAX_A2T_AUDIO_LEN_MS) {
        qWarning() << "Audio length exceeds limit:" << m_voiceBlock->voiceSize << "ms (max:" << MAX_A2T_AUDIO_LEN_MS << "ms)";
        // 弹窗提示
        Q_EMIT audioLengthLimit();
        return;
    } else {
        qDebug() << "Starting audio to text conversion for file:" << m_voiceBlock->voicePath;
        onA2TStart();
    }
}

void VoiceToTextHandler::onA2TStart()
{
    qInfo() << "VoiceToTextHandler onA2TStart called";
    OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, true);
    Q_EMIT JsContent::instance()->callJsSetVoiceText(QString(""), JsContent::AsrFlag::Start);
    QTimer::singleShot(0, this, [this]() { m_a2tManager->startAsr(m_voiceBlock->voicePath, m_voiceBlock->voiceSize); });
}

bool VoiceToTextHandler::checkNetworkState()
{
    qDebug() << "Checking network state";
    QDBusInterface network("org.deepin.dde.Network1", "/org/deepin/dde/Network1",
                           "org.deepin.dde.Network1");

    if (!network.isValid()) {
        qWarning() << "Failed to obtain the network status DBUS";
        return false;
    }

    QVariant ret = network.property("State");
    bool isConnected = (ret.toInt() == 70);
    qDebug() << "Network state:" << (isConnected ? "Connected" : "Disconnected");
    return isConnected;
}

void VoiceToTextHandler::onA2TError(int error)
{
    qInfo() << "VoiceToTextHandler onA2TError called";
    // TODO(renbin): 弹窗提示
    Q_EMIT JsContent::instance()->callJsSetVoiceText("", JsContent::AsrFlag::End);
    OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, false);
}

void VoiceToTextHandler::onA2TSuccess(const QString &text)
{
    qInfo() << "VoiceToTextHandler onA2TSuccess called";
    Q_EMIT JsContent::instance()->callJsSetVoiceText(text, JsContent::AsrFlag::End);
    OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, false);
}
