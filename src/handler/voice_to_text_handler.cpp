// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice_to_text_handler.h"
#include "voice_to_text_task_manager.h"

#include <QTimer>
#include <QDBusInterface>

#include "globaldef.h"
#include "common/vnoteitem.h"
#include "common/vnotea2tmanager.h"
#include "common/jscontent.h"
#include "common/VNoteMainManager.h"
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
        // 记录发起转换时的上下文（笔记ID 和 语音路径）
        m_originalNoteId = VNoteMainManager::instance()->currentNoteId();
        m_originalVoicePath = m_voiceBlock->voicePath;

        // 注册到任务管理器
        VoiceToTextTaskManager::instance()->addTask(m_originalNoteId, m_originalVoicePath);

        qDebug() << "Starting audio to text conversion for file:" << m_voiceBlock->voicePath
                 << "noteId:" << m_originalNoteId;
        onA2TStart();
    }
}

void VoiceToTextHandler::onA2TStart()
{
    qInfo() << "VoiceToTextHandler onA2TStart called";
    OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, true);
    emit VNoteMainManager::instance()->voiceToTextStateChanged(true);
    Q_EMIT JsContent::instance()->callJsSetVoiceText(QString(""), JsContent::AsrFlag::Start);
    QTimer::singleShot(0, this, [this]() { m_a2tManager->startAsr(m_voiceBlock->voicePath, m_voiceBlock->voiceSize); });
}

bool VoiceToTextHandler::checkNetworkState()
{
    qDebug() << "Checking network state";
    QDBusInterface network("org.freedesktop.NetworkManager",
                           "/org/freedesktop/NetworkManager",
                           "org.freedesktop.NetworkManager",
                           QDBusConnection::systemBus());

    if (!network.isValid()) {
        qWarning() << "Failed to obtain the network status from NetworkManager D-Bus";
        return false;
    }

    QVariant ret = network.property("State");
    bool isConnected = (ret.toInt() == 70);
    qDebug() << "Network state:" << ret.toInt() << (isConnected ? "Connected" : "Disconnected");
    return isConnected;
}

void VoiceToTextHandler::onA2TError(int error)
{
    qInfo() << "VoiceToTextHandler onA2TError called, error:" << error;

    // 更新任务管理器状态
    VoiceToTextTaskManager::instance()->setTaskResult(m_originalVoicePath, QString(), false);

    // 检查当前笔记是否是发起转换的笔记
    int currentNoteId = VNoteMainManager::instance()->currentNoteId();
    if (currentNoteId == m_originalNoteId) {
        // 当前在原始笔记，通过 JS 更新 UI
        Q_EMIT JsContent::instance()->callJsSetVoiceText("", JsContent::AsrFlag::End);
    } else {
        // 已切换到其他笔记，通过 voicePath 更新（如果用户切回时恢复）
        Q_EMIT JsContent::instance()->callJsSetVoiceTextByPath(m_originalVoicePath, "", JsContent::AsrFlag::End);
    }

    OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, false);
    emit VNoteMainManager::instance()->voiceToTextStateChanged(false);
}

void VoiceToTextHandler::onA2TSuccess(const QString &text)
{
    qInfo() << "VoiceToTextHandler onA2TSuccess called, text length:" << text.length();

    // 更新任务管理器状态
    VoiceToTextTaskManager::instance()->setTaskResult(m_originalVoicePath, text, true);

    // 检查当前笔记是否是发起转换的笔记
    int currentNoteId = VNoteMainManager::instance()->currentNoteId();

    if (currentNoteId == m_originalNoteId) {
        // 当前在原始笔记，走现有 JS 插入逻辑
        Q_EMIT JsContent::instance()->callJsSetVoiceText(text, JsContent::AsrFlag::End);
    } else {
        // 已切换到其他笔记，在 C++ 层直接修改原始笔记的 HTML
        qInfo() << "Note switched during conversion, saving result to original note:"
                << m_originalNoteId << "current note:" << currentNoteId;
        VNoteMainManager::instance()->insertVoiceTextToNote(
            m_originalNoteId, m_originalVoicePath, text);
    }

    OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, false);
    emit VNoteMainManager::instance()->voiceToTextStateChanged(false);
}
