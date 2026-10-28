// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// WebKitWebView —— 供 QML 使用的 WebKitGTK 桥视图（QQuickItem）。
//
// 目标：作为 QQuickWebEngineView 的等价替换，保持 WebEngineView.qml 既有调用面
// （runJavaScript / triggerWebAction / findText / webChannel / url / zoomFactor /
//  contextMenuRequested / loadingChanged），底层走「webhelper 独立进程 + D-Bus 桥 +
// 浮层真窗嵌入」方案（见 3rdparty/webkitbridge/docs/bridge-api.md）。
//
// 关键行为约定：
//   1) runJavaScript 的回调结果做标量转换（"true"/"false"/数字/undefined），
//      尽量贴近 QWebEngine runJavaScript 的 QVariant 语义；结构化结果由 JS 侧
//      JSON.stringify 后回传字符串。
//   2) helper 未就绪/页面未加载完成期间，JS 调用排队，LoadFinished(ok) 后按序执行。
//   3) webChannel 属性接 QML WebChannel 对象：宿主创建 WebChannelTransport 并
//      connectTo()，页面报文经 ScriptMessage 汇入，DnD 消息单独分流。
//   4) 上下文菜单：helper 抑制默认菜单后转发 ContextMenu(x, y, ctx 位掩码)，
//      本类整理为与 WebEngine 请求兼容的 QVariantMap {position, editFlags, ctx}。
//   5) 拖放：helper 接管 WebKit 默认拖放目标，文件/文本拖放以 filesDropped /
//      textDropped 信号回传 QML（替代 QML DropArea，拖放目标在独立窗口内）。
#pragma once

#include <QColor>
#include <QJSValue>
#include <QQuickItem>
#include <QQueue>
#include <QUrl>
#include <QVariant>

#include "webkitbridgeclient.h"

namespace webnote {

class WebChannelTransport;

class WebKitWebView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged FINAL)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor
                   NOTIFY backgroundColorChanged FINAL)
    Q_PROPERTY(QObject *webChannel READ webChannel WRITE setWebChannel NOTIFY webChannelChanged FINAL)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged FINAL)
    Q_PROPERTY(bool webFocused READ webFocused NOTIFY webFocusedChanged FINAL)
public:
    explicit WebKitWebView(QQuickItem *parent = nullptr);
    ~WebKitWebView() override;

    QUrl url() const { return m_url; }
    void setUrl(const QUrl &url);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor &color);

    QObject *webChannel() const { return m_webChannel; }
    void setWebChannel(QObject *channel);

    qreal zoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(qreal factor);

    bool webFocused() const { return m_webFocused; }

    // ---- QQuickWebEngineView 兼容调用面 ----
    Q_INVOKABLE void runJavaScript(const QString &script, const QJSValue &callback = QJSValue());
    Q_INVOKABLE void triggerWebAction(int action);
    Q_INVOKABLE void findText(const QString &text);
    Q_INVOKABLE void findTextClear();
    // 把 X 键盘焦点交给 web 浮窗（IM 链路；QML 用它替代对 view 的 forceActiveFocus）
    Q_INVOKABLE void focusWeb();

Q_SIGNALS:
    void urlChanged();
    void backgroundColorChanged();
    void webChannelChanged();
    void zoomFactorChanged();
    void webFocusedChanged();

    // 对应 WebEngineView 的 onLoadingChanged(LoadSucceededStatus)
    void loadFinished(const QUrl &url, bool ok, const QString &error);
    // 上下文菜单：position 为 view 本地坐标；editFlags 为 WebEngine 编辑标志位
    // （可复制=2 可剪=4 可粘贴=8 可全选=1 可删除=16），ctx 为 WebKit 命中区掩码
    void contextMenuRequested(const QVariantMap &request);
    void filesDropped(const QVariantList &urls, qreal x, qreal y);
    void textDropped(const QString &text, qreal x, qreal y);

protected:
    void componentComplete() override;

private:
    void maybeStart();
    void evalJsInternal(const QString &script, const QJSValue &callback);
    void flushPendingJs();
    void applyBackgroundColor();
    void setWebFocused(bool focused);
    void handleScriptMessage(const QString &name, const QString &body);
    static QString jsStringLiteral(const QString &text);

private:
    webkitbridge::WebKitBridgeClient *m_client = nullptr;
    WebChannelTransport *m_transport = nullptr;
    QObject *m_webChannel = nullptr;

    QUrl m_url;
    QColor m_backgroundColor = QColor(Qt::white);
    qreal m_zoomFactor = 1.0;
    bool m_webFocused = false;
    bool m_pageLoaded = false;
    bool m_initialLoadDone = false;

    struct PendingJs
    {
        QString script;
        QJSValue callback;
    };
    QQueue<PendingJs> m_pendingJs;
};

}  // namespace webnote
