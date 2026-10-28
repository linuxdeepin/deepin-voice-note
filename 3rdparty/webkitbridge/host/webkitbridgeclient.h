// SPDX-License-Identifier: GPL-3.0-or-later
// webkitbridgeclient.h — WebKitGTK 桥宿主侧 Qt6 封装（公共库头文件）
//
// 用途：Qt 应用以「独立 WebKitGTK 进程(webhelper) + D-Bus 桥 + 浮层真窗嵌入」
// 替代 QtWebEngine 时，接入方只需使用本类，无需直接接触 D-Bus / X11 / 进程管理。
//
// 嵌入模型（实测结论，见 docs/im-verification.md）：kwin_x11 会立即回收非受管
// 嵌入窗（含非 override-redirect）的 X 输入焦点，键盘/输入法全部失效；故 helper
// 窗口必须是真实受管顶层窗（COMBO 类型 + transient-for 宿主顶层窗），悬浮于容器
// 上方，几何由宿主以全局坐标持续同步——观感与嵌入一致，焦点/IM 走标准 WM 路径。
//
// 职责（对应任务工作项一）：
//   1) 嵌入与几何同步：attach(QWidget*) → 浮层定位 + 容器 Move/Resize 事件驱动同步
//      （一律宿主驱动，与 Qt 布局模型一致；高 DPI 下自动乘 devicePixelRatio）
//   2) 焦点路由：Qt 侧控件获得焦点 → X 焦点收回宿主；focusWeb() → D-Bus FocusWeb
//      → helper 内 grab_focus（IM 链路）
//   3) 崩溃恢复：helper 异常退出后按退避序列自动重启，重嵌入并重载最近页面
//   4) bus name 唯一性：org.deepin.WebHelper.<appId>[.<instance>]；启动前清理残留
//      helper 进程；名称被占(BUSY)时强制清理并重试一次
//   5) EvalJS 容忍 undefined 结果（"undefined" 字符串回包），真 JS 异常回错误串
//   6) helper stderr 经 QProcess::ForwardedErrorChannel 落宿主日志（默认开启）
//
// 红线（见 docs/bridge-api.md）：UI 进程禁止创建独立 JSCContext；信号 JSON 解析在
// 宿主侧用 QJsonDocument，helper 侧用纯 C —— 双端都不得引入独立 JSC VM。
//
// 推荐调用顺序：
//   WebKitBridgeClient c({"deepin-editor"});
//   c.attach(ui->webContainer);            // 容器 widget（可为空布局占位）
//   QString err; c.start(&err);            // spawn helper → READY → 自动嵌入
//   connect(&c, &WebKitBridgeClient::loadFinished, ...);
//   c.loadUri("file:///path/page.html");
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <functional>

#include <QProcess>
#include <QWidget>
#include <QQuickItem>  // QPointer<QQuickItem> 成员需完整类型（AUTOMOC 生成 TU 中实例化）

class QDBusInterface;

namespace webkitbridge {

struct BridgeOptions {
    QString appId;            ///< 必填：bus name 命名与残留清理识别（'-'/非法字符自动转 '_'）
    QString instanceId;       ///< 可选：同应用多实例时区分 bus name
    QString helperPath;       ///< webhelper 可执行文件路径；默认 applicationDirPath()/webhelper
    bool webChannelShim = false;   ///< 注入 WebChannel 兼容垫片（WEBHELPER_SHIM=1）
    bool forwardHelperStderr = true;///< helper stderr 直通宿主 stderr（QProcess::ForwardedErrorChannel）
    int readyTimeoutMs = 12000;    ///< READY 握手超时
    QStringList restartBackoffMs = { "1000", "2000", "4000", "8000", "16000" }; ///< 崩溃重启退避
    int stableResetMs = 60000;     ///< helper 稳定运行该时长后重置重启计数
};

class WebKitBridgeClient : public QObject {
    Q_OBJECT
public:
    using EvalCallback = std::function<void(const QString &result, const QString &err)>;
    using BoolCallback = std::function<void(bool ok, const QString &err)>;
    struct PrintResult {
        bool ok = false;
        QString err;
        quint64 bytes = 0;
        double ms = 0.0;
    };
    using PrintCallback = std::function<void(const PrintResult &)>;

    explicit WebKitBridgeClient(const BridgeOptions &options, QObject *parent = nullptr);
    ~WebKitBridgeClient() override;

    /// 启动 helper（含残留清理与 BUSY 重试）；失败返回 false 并填 *error
    bool start(QString *error = nullptr);
    /// 停止 helper 并禁用自动重启（析构自动调用）
    void shutdown();
    /// 绑定嵌入容器（可在 start 前调用；READY 后自动嵌入）
    void attach(QWidget *container);
    /// QML 场景绑定：锚定任意 QQuickItem（浮层几何跟随其全局位置/尺寸）。
    /// voice-note 等纯 QML 应用用此重载，容器无需 QWidget。
    void attach(QQuickItem *anchor);

    bool isReady() const { return m_ready; }
    QWidget *container() const { return m_container.data(); }
    QQuickItem *anchorItem() const { return m_anchor.data(); }

    QString busName() const { return m_busName; }
    static QString interfaceName() { return QStringLiteral("org.deepin.WebHelper.Bridge"); }
    static QString objectPath() { return QStringLiteral("/org/deepin/WebHelper"); }
    /// helper X11 窗口 id（嵌入后有效；用于外部诊断）
    quint64 helperXid() const { return m_webwin; }

    // ---- Web 能力（与 D-Bus 接口一一对应，见 docs/bridge-api.md）----
    void loadUri(const QString &uri);
    void loadHtml(const QString &html, const QString &baseUri = QString());
    /// 执行 JS；undefined 结果回 "undefined"（非错误），JS 异常回 err 非空
    void evalJS(const QString &script, EvalCallback callback);
    /// 同步便捷版；超时或错误返回空串并填 *err（"undefined" 结果原样返回）
    QString evalJSSync(const QString &script, int timeoutMs = 10000, QString *err = nullptr);
    void printToPdf(const QString &path, BoolCallback callback);
    /// 任务式打印：uri 非空则先加载待 LoadFinished 再打印；回包含错误/字节/耗时
    PrintResult printToPdfExSync(const QString &uri, const QString &path,
                                 int timeoutMs = 30000, QString *err = nullptr);
    void printToPdfEx(const QString &uri, const QString &path, PrintCallback callback);
    void snapshot(const QString &path, BoolCallback callback);
    PrintResult snapshotSync(const QString &path, int timeoutMs = 20000, QString *err = nullptr);
    /// 焦点移交 web 区（IM 链路：helper 内 grab_focus）
    void focusWeb();
    /// 编辑命令（WebKit 命令名：Cut/Copy/Paste/SelectAll/Undo/Redo/Delete…）
    void executeEditCommand(const QString &command);
    /// 页面缩放（WebKit 原生缩放级别；Ctrl+滚轮时 helper 自动回 zoomChanged）
    void setZoomLevel(double level);
    /// 页面底色（加载前背景，避免主题切换闪色）
    void setBackgroundColor(const QColor &color);

    /// D-Bus 名称清洗：appId/instance 段中非法字符替换为 '_'，数字开头补前缀
    static QString sanitizeNameToken(const QString &raw);

signals:
    void ready();                                   ///< helper 就绪（首次或重启后）
    void scriptMessage(const QString &name, const QString &body);
    void consoleMessage(const QString &level, const QString &text);
    void loadFinished(const QString &uri, const QString &status); ///< status=="ok" 或错误消息
    void webFocused();
    /// Ctrl+滚轮原生缩放回传（宿主可同步 QML 侧 zoomFactor 状态）
    void zoomChanged(double level);
    /// 右键菜单请求（helper 抑制默认菜单后转发；x/y 为 webview 本地坐标，
    /// ctx 为 WebKitHitTestResultContext 位掩码：document=2 link=4 image=8
    /// media=16 editable=32 scrollbar=64 selection=128）
    void contextMenuRequested(int x, int y, int ctx);
    void helperCrashed(const QString &reason);      ///< helper 意外退出（每次）
    void helperRestarted(int attempt);              ///< 崩溃重启完成并重新嵌入+重载后

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void spawn();
    void handleProcessFinished(int exitCode);
    void handleProcessError(QProcess::ProcessError err);
    void handleStdout();
    void embedAndLoad();
    void syncGeometry();
    void syncItemGeometry();
    void connectItemSignals();
    void scheduleRestart(const QString &reason);
    void cleanupStaleHelpers(bool forceKill);
    void connectDBusSignals();
    QDBusInterface *iface();

    BridgeOptions m_opts;
    QString m_busName;
    QProcess *m_proc = nullptr;
    QDBusInterface *m_iface = nullptr;
    QPointer<QWidget> m_container;
    QPointer<QQuickItem> m_anchor;
    bool m_ready = false;
    bool m_shuttingDown = false;
    bool m_signalsConnected = false;
    bool m_itemSignalsConnected = false;
    bool m_busyRetried = false;
    bool m_firstReadySeen = false;
    int m_restartAttempt = 0;
    qint64 m_lastUriLoadedAt = 0;
    QString m_lastUri;
    bool m_hasLastUri = false;
    quint64 m_webwin = 0;       // helper X11 window id
    void *m_dpy = nullptr;      // 宿主侧私有 X 连接（XDisplay*，实现内转换，避免头文件引入 X11）
    quint64 m_qtWin = 0;        // 容器 X11 window id
    QElapsedTimer m_stableTimer;
};

} // namespace webkitbridge
