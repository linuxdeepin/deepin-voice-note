// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webkitwebview.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QWebChannel>

#include "webchanneltransport.h"

namespace webnote {

namespace {

// EvalJS 回包是 jsc_value_to_string 的原始字符串，需要把常见标量转回
// QVariant，贴近 QWebEngine runJavaScript 的结果语义。结构化结果由 JS 侧
// JSON.stringify 成字符串后原样透传（QML 端自行 JSON.parse）。
QVariant scalarFromEvalString(const QString &s)
{
    if (s == QLatin1String("true"))
        return QVariant(true);
    if (s == QLatin1String("false"))
        return QVariant(false);
    if (s == QLatin1String("undefined") || s == QLatin1String("null")
        || s.compare(QLatin1String("NaN"), Qt::CaseInsensitive) == 0)
        return QVariant();
    if (s == QLatin1String("Infinity"))
        return QVariant(qInf());
    if (s == QLatin1String("-Infinity"))
        return QVariant(-qInf());
    bool ok = false;
    const double d = s.toDouble(&ok);
    if (ok)
        return QVariant(d);
    return QVariant(s);
}

// QWebEnginePage::WebAction（Qt5/Qt6 在这些取值上保持一致）→ WebKit 编辑命令
QString webActionToEditCommand(int action)
{
    switch (action) {
    case 4: return QStringLiteral("Cut");
    case 5: return QStringLiteral("Copy");
    case 6: return QStringLiteral("Paste");
    case 7: return QStringLiteral("Undo");
    case 8: return QStringLiteral("Redo");
    case 9: return QStringLiteral("SelectAll");
    default: return QString();
    }
}

// WebKitHitTestResultContext 位掩码 → QWebEngine 请求 editFlags
// （QWebEngineContextMenuData：CanSelectAll=1 CanCopy=2 CanCut=4 CanPaste=8
//   CanDelete=16）。可编辑区必然允许全选/粘贴；有选区时允许复制/剪切/删除。
int ctxToEditFlags(int ctx)
{
    const bool editable = (ctx & (1 << 5)) != 0;  // EDITABLE
    const bool selection = (ctx & (1 << 7)) != 0; // SELECTION
    int flags = 0;
    if (editable) {
        flags |= 1 /*SelectAll*/;
        flags |= 8 /*Paste*/;
    }
    if (selection && editable) {
        flags |= 2 /*Copy*/;
        flags |= 4 /*Cut*/;
        flags |= 16 /*Delete*/;
    }
    return flags;
}

QString webhelperCandidatePath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/webhelper"),
        // 构建布局：<build>/src/deepin-voice-note -> <build>/3rdparty/webkitbridge/webhelper
        appDir + QStringLiteral("/../3rdparty/webkitbridge/webhelper"),
        appDir + QStringLiteral("/../../3rdparty/webkitbridge/webhelper"),
    };
    for (const QString &cand : candidates) {
        if (QFileInfo::exists(cand))
            return QDir::cleanPath(cand);
    }
    return appDir + QStringLiteral("/webhelper");
}

}  // namespace

WebKitWebView::WebKitWebView(QQuickItem *parent)
    : QQuickItem(parent)
{
}

WebKitWebView::~WebKitWebView()
{
    if (m_client) {
        m_client->shutdown();
        m_client->deleteLater();
        m_client = nullptr;
    }
}

void WebKitWebView::componentComplete()
{
    QQuickItem::componentComplete();
    // 视图初始不可见（webVisible=false）时延迟到首次可见再拉起 webhelper
    connect(this, &QQuickItem::visibleChanged, this, [this] { maybeStart(); });
    maybeStart();
}

void WebKitWebView::setUrl(const QUrl &url)
{
    if (m_url == url)
        return;
    m_url = url;
    Q_EMIT urlChanged();
    // 运行期改 url → 重载页面（与 WebEngine url 赋值语义一致）
    if (m_client && m_client->isReady() && m_initialLoadDone && m_url.isValid()
        && !m_url.isEmpty()) {
        m_pageLoaded = false;
        m_client->loadUri(m_url.toString());
    }
    maybeStart();
}

void WebKitWebView::setBackgroundColor(const QColor &color)
{
    if (m_backgroundColor == color)
        return;
    m_backgroundColor = color;
    Q_EMIT backgroundColorChanged();
    applyBackgroundColor();
}

void WebKitWebView::setWebChannel(QObject *channel)
{
    if (m_webChannel == channel)
        return;
    m_webChannel = channel;
    Q_EMIT webChannelChanged();

    if (!m_transport) {
        m_transport = new WebChannelTransport(
            [this](const QJsonObject &message) {
                if (!m_client || !m_client->isReady())
                    return;
                // 宿主 → 页面：QWebChannel 服务器报文 → base64 注入页面 transport
                const QByteArray json = QJsonDocument(message).toJson(QJsonDocument::Compact);
                const QByteArray b64 = json.toBase64();
                m_client->evalJS(QStringLiteral("window.__wcDeliverChannel('%1')")
                                     .arg(QString::fromLatin1(b64)), {});
            },
            this);
    }

    // QML WebChannel（QQmlWebChannel 继承 QWebChannel）已完成对象注册，
    // 这里只把 transport 接入服务器；连接时机早于页面 qwebchannel.js 的
    // idle 握手即可（页面在 LoadURI 之后才会发第一条消息）。
    if (auto *ch = qobject_cast<QWebChannel *>(m_webChannel)) {
        ch->connectTo(m_transport);
    } else if (m_webChannel) {
        qWarning() << "[webkitwebview] webChannel is not a QWebChannel:" << m_webChannel;
    }
}

void WebKitWebView::setZoomFactor(qreal factor)
{
    if (qFuzzyCompare(m_zoomFactor, factor))
        return;
    m_zoomFactor = factor;
    Q_EMIT zoomFactorChanged();
    if (m_client && m_client->isReady())
        m_client->setZoomLevel(factor);
}

void WebKitWebView::setWebFocused(bool focused)
{
    if (m_webFocused == focused)
        return;
    m_webFocused = focused;
    Q_EMIT webFocusedChanged();
}

void WebKitWebView::maybeStart()
{
    if (m_client || !isVisible() || !m_url.isValid() || m_url.isEmpty())
        return;

    webkitbridge::BridgeOptions opts;
    opts.appId = QStringLiteral("deepin-voice-note");
    // webChannelShim: helper 注入 window.qt.webChannelTransport + __wcDeliverChannel
    opts.webChannelShim = true;

    m_client = new webkitbridge::WebKitBridgeClient(opts, this);
    m_client->attach(this);

    connect(m_client, &webkitbridge::WebKitBridgeClient::ready, this, [this] {
        // 助手重启后需要重放底色/缩放，并由新的页面加载周期驱动 JS 队列。
        // 重启后的页面重载由公共库内部（helperRestarted 路径）自动完成，
        // 这里只负责首次加载。
        m_pageLoaded = false;
        applyBackgroundColor();
        if (!qFuzzyCompare(m_zoomFactor, 1.0))
            m_client->setZoomLevel(m_zoomFactor);
        if (!m_initialLoadDone && m_url.isValid() && !m_url.isEmpty()) {
            m_initialLoadDone = true;
            m_client->loadUri(m_url.toString());
        }
    });

    connect(m_client, &webkitbridge::WebKitBridgeClient::loadFinished, this,
            [this](const QString &uri, const QString &status) {
                const bool ok = (status == QLatin1String("ok"));
                m_pageLoaded = ok;
                if (ok)
                    flushPendingJs();
                Q_EMIT loadFinished(QUrl(uri), ok, ok ? QString() : status);
            });

    connect(m_client, &webkitbridge::WebKitBridgeClient::scriptMessage, this,
            &WebKitWebView::handleScriptMessage);

    connect(m_client, &webkitbridge::WebKitBridgeClient::webFocused, this,
            [this] { setWebFocused(true); });

    // 用户点击 Qt 侧控件 → 应用窗口重新激活 → web 浮窗失去键盘焦点
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *win) {
        if (!win)
            return;
        connect(win, &QQuickWindow::activeChanged, win, [this, win]() {
            if (win->isActive())
                setWebFocused(false);
        });
    });

    connect(m_client, &webkitbridge::WebKitBridgeClient::zoomChanged, this,
            [this](double level) { setZoomFactor(level); });

    connect(m_client, &webkitbridge::WebKitBridgeClient::contextMenuRequested, this,
            [this](int x, int y, int ctx) {
                QVariantMap request;
                request.insert(QStringLiteral("position"), QPointF(x, y));
                request.insert(QStringLiteral("x"), x);
                request.insert(QStringLiteral("y"), y);
                request.insert(QStringLiteral("editFlags"), ctxToEditFlags(ctx));
                request.insert(QStringLiteral("ctx"), ctx);
                Q_EMIT contextMenuRequested(request);
            });

    QString err;
    if (!m_client->start(&err)) {
        qWarning() << "[webkitwebview] start webhelper failed:" << err;
        m_client->deleteLater();
        m_client = nullptr;
    }
}

void WebKitWebView::applyBackgroundColor()
{
    if (m_client && m_client->isReady())
        m_client->setBackgroundColor(m_backgroundColor);
}

void WebKitWebView::handleScriptMessage(const QString &name, const QString &body)
{
    if (name != QLatin1String("bridge") || !m_transport)
        return;

    // DnD 消息由 helper 拖放目标转发（{"type":"dnd",...}），不属于
    // QtWebChannel 协议，单独分流给 QML。
    const QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
    if (doc.isObject() && doc.object().value(QLatin1String("type")).toString()
            == QLatin1String("dnd")) {
        const QJsonObject obj = doc.object();
        const qreal x = obj.value(QLatin1String("x")).toDouble();
        const qreal y = obj.value(QLatin1String("y")).toDouble();
        const QJsonArray urls = obj.value(QLatin1String("urls")).toArray();
        if (!urls.isEmpty()) {
            QVariantList list;
            for (const QJsonValue &v : urls)
                list.append(v.toString());
            Q_EMIT filesDropped(list, x, y);
        } else {
            Q_EMIT textDropped(obj.value(QLatin1String("text")).toString(), x, y);
        }
        return;
    }

    m_transport->acceptMessage(body);
}

void WebKitWebView::runJavaScript(const QString &script, const QJSValue &callback)
{
    if (!m_client || !m_client->isReady() || !m_pageLoaded) {
        // helper 就绪前的调用排队（WebEngine 对早于页面加载的调用同样会缓冲）
        m_pendingJs.enqueue({ script, callback });
        return;
    }
    evalJsInternal(script, callback);
}

void WebKitWebView::evalJsInternal(const QString &script, const QJSValue &callback)
{
    if (!m_client)
        return;
    m_client->evalJS(script, [this, callback](const QString &result, const QString &err) {
        if (!callback.isCallable())
            return;
        QQmlEngine *engine = qmlEngine(this);
        if (!engine)
            return;
        QJSValue arg = engine->toScriptValue(
            err.isEmpty() ? scalarFromEvalString(result) : QVariant());
        callback.call(QJSValueList{ arg });
    });
}

void WebKitWebView::flushPendingJs()
{
    if (!m_client || !m_client->isReady() || !m_pageLoaded)
        return;
    while (!m_pendingJs.isEmpty()) {
        const PendingJs job = m_pendingJs.dequeue();
        evalJsInternal(job.script, job.callback);
    }
}

void WebKitWebView::triggerWebAction(int action)
{
    const QString command = webActionToEditCommand(action);
    if (command.isEmpty()) {
        qWarning() << "[webkitwebview] unsupported WebAction:" << action;
        return;
    }
    if (m_client && m_client->isReady())
        m_client->executeEditCommand(command);
}

void WebKitWebView::findText(const QString &text)
{
    if (text.isEmpty())
        return;
    runJavaScript(QStringLiteral("window.find(%1)").arg(jsStringLiteral(text)), {});
}

void WebKitWebView::findTextClear()
{
    runJavaScript(QStringLiteral("window.getSelection && window.getSelection().removeAllRanges();"),
                  {});
}

void WebKitWebView::focusWeb()
{
    // Qt 侧 item 抢占 activeFocus 以断开 QML 内部焦点链，再把 X 焦点交给
    // web 浮窗（helper 内 grab_focus，IM 走标准路径）。
    forceActiveFocus();
    if (m_client && m_client->isReady()) {
        m_client->focusWeb();
        setWebFocused(true);
    }
}

QString WebKitWebView::jsStringLiteral(const QString &text)
{
    return QString::fromUtf8(
        QJsonDocument(QJsonArray{ text }).toJson(QJsonDocument::Compact).mid(1).chopped(1));
}

}  // namespace webnote
