// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// QtWebChannel 与 WebKitGTK helper（webhelper）之间的最小传输层。
//
// 页面侧 qwebchannel.js 仍使用标准 window.qt.webChannelTransport 契约（由
// helper 的 shim 注入），因此 Web 代码无需感知底层是 WebEngine 还是 WebKitGTK：
//   页面 → 宿主：transport.send(json) → helper postMessage → D-Bus ScriptMessage
//                → 宿主 acceptMessage() → Q_EMIT messageReceived
//   宿主 → 页面：QWebChannel 服务器调用 sendMessage(json) → EvalJS 注入
//                window.__wcDeliverChannel(base64) → 页面 transport.onmessage
#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QWebChannelAbstractTransport>
#include <functional>

namespace webnote {

class WebChannelTransport : public QWebChannelAbstractTransport
{
    Q_OBJECT
public:
    using Sender = std::function<void(const QJsonObject &)>;

    explicit WebChannelTransport(Sender sender, QObject *parent = nullptr)
        : QWebChannelAbstractTransport(parent)
        , m_sender(std::move(sender))
    {
    }

    // QWebChannel 服务器 → 页面
    void sendMessage(const QJsonObject &message) override
    {
        if (m_sender)
            m_sender(message);
    }

public Q_SLOTS:
    // 页面 → QWebChannel 服务器（body 为页面发来的 JSON 字符串）
    void acceptMessage(const QString &body)
    {
        if (body.trimmed().isEmpty())
            return;
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[webkitwebview] invalid WebChannel payload:" << body.left(200);
            return;
        }
        Q_EMIT messageReceived(doc.object(), this);
    }

private:
    Sender m_sender;
};

}  // namespace webnote
