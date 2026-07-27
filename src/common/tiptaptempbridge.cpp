// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tiptaptempbridge.h"

#include <QResource>
#include <QCoreApplication>
#include <QDebug>

TiptapTempBridge::TiptapTempBridge(QObject *parent)
    : QObject(parent)
{
}

bool TiptapTempBridge::debugEnabled() const
{
    return !qgetenv("DVN_TIPTAP_DEBUG").isEmpty();
}

QString TiptapTempBridge::tiptapHtmlPath() const
{
    // 首选 qrc 资源路径；若 qrc 不存在则回退 install 目录
    if (QResource(":/tiptap-editor.html").isValid()) {
        return QStringLiteral("qrc:/tiptap-editor.html");
    }
    return QStringLiteral("file://" TIPTAP_WEB_PATH "/tiptap-editor.html");
}

void TiptapTempBridge::jsEditorReady()
{
    qInfo() << "Tiptap editor ready";
}

void TiptapTempBridge::jsContentSaved(const QString &envelopeJson)
{
    qInfo() << "Tiptap content saved, envelope length:" << envelopeJson.length();
}
