// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tiptapchannelbridge.h"

#include <QResource>
#include <QCoreApplication>
#include <QDebug>

TiptapChannelBridge::TiptapChannelBridge(QObject *parent)
    : QObject(parent)
    , m_editorReady(false)
{
}

bool TiptapChannelBridge::debugEnabled() const
{
    return !qgetenv("DVN_TIPTAP_DEBUG").isEmpty();
}

QString TiptapChannelBridge::tiptapHtmlPath() const
{
    // 首选 qrc 资源路径；若 qrc 不存在则回退 install 目录
    if (QResource(":/tiptap-editor.html").isValid()) {
        return QStringLiteral("qrc:/tiptap-editor.html");
    }
    return QStringLiteral("file://" TIPTAP_WEB_PATH "/tiptap-editor.html");
}

QString TiptapChannelBridge::resourceBaseUrl() const
{
    // 适配层：返回宿主资源根路径，前端用此 + "images/xxx" 拼接绝对路径
    // 只读复用 jscontent.cpp 的 WEB_PATH 路径约定，不改其源码
    return QStringLiteral("file://" WEB_PATH);
}

// ---------------------------------------------------------------------------
// 加载就绪缓存语义（R6）
// ---------------------------------------------------------------------------

void TiptapChannelBridge::notifyEditorReady()
{
    m_editorReady = true;
    emit editorReady();

    // 就绪后补发缓存的 envelope，不丢数据
    if (!m_pendingEnvelope.isEmpty()) {
        emit loadEnvelopeRequested(m_pendingEnvelope);
        m_pendingEnvelope.clear();
    }
}

void TiptapChannelBridge::loadEnvelope(const QString &envelopeJson)
{
    if (m_editorReady) {
        // 已就绪，直接下发
        emit loadEnvelopeRequested(envelopeJson);
    } else {
        // 未就绪，缓存待发
        m_pendingEnvelope = envelopeJson;
    }
}

QString TiptapChannelBridge::pendingEnvelope() const
{
    return m_pendingEnvelope;
}

bool TiptapChannelBridge::isEditorReady() const
{
    return m_editorReady;
}

// ---------------------------------------------------------------------------
// 保存往返
// ---------------------------------------------------------------------------

void TiptapChannelBridge::requestEditorContent()
{
    emit requestContent();
}

// ---------------------------------------------------------------------------
// 插入图片 / 语音
// ---------------------------------------------------------------------------

void TiptapChannelBridge::sendInsertImage(const QString &imageInfoJson)
{
    emit insertImage(imageInfoJson);
}

void TiptapChannelBridge::sendInsertVoiceBlock(const QString &voiceInfoJson)
{
    emit insertVoiceBlock(voiceInfoJson);
}

// ---------------------------------------------------------------------------
// JS→C++ 回告入口
// ---------------------------------------------------------------------------

void TiptapChannelBridge::jsEditorReady()
{
    qInfo() << "Tiptap editor ready (TiptapChannelBridge)";
    notifyEditorReady();
}

void TiptapChannelBridge::jsContentChanged()
{
    emit contentChanged();
}

void TiptapChannelBridge::jsContentSaved(const QString &envelopeJson)
{
    qInfo() << "Tiptap content saved, envelope length:" << envelopeJson.length();
    emit contentSaved(envelopeJson);
}

void TiptapChannelBridge::jsInsertImageFailed(const QString &reason)
{
    qWarning() << "Tiptap insert image failed:" << reason;
    emit insertImageFailed(reason);
}

void TiptapChannelBridge::jsInsertVoiceBlockFailed(const QString &reason)
{
    qWarning() << "Tiptap insert voice block failed:" << reason;
    emit insertVoiceBlockFailed(reason);
}
