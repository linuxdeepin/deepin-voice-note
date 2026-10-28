// SPDX-License-Identifier: GPL-3.0-or-later
// webkitbridgeclient.cpp — WebKitGTK 桥宿主侧 Qt6 封装实现
#include "webkitbridgeclient.h"

#include <QApplication>
#include <QColor>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQuickItem>
#include <QQuickWindow>
#include <QResizeEvent>
#include <QTimer>
#include <QThread>
#include <QWidget>
#include <QDebug>

#include <X11/Xlib.h>
/* X11 宏与 Qt 标识符冲突，解除（X11 头必须在 Qt 头之后 include） */
#undef FocusIn
#undef FocusOut
#undef KeyPress
#undef KeyRelease
#undef ButtonPress
#undef ButtonRelease

#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>

namespace webkitbridge {

static const int kBusyExitCode = 3;

QString WebKitBridgeClient::sanitizeNameToken(const QString &raw)
{
    QString out;
    out.reserve(raw.size());
    for (const QChar &c : raw) {
        const ushort u = c.unicode();
        if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '_')
            out.append(c);
        else
            out.append(QLatin1Char('_'));
    }
    if (!out.isEmpty() && out.at(0).isDigit())
        out.prepend(QLatin1Char('_'));
    return out;
}

WebKitBridgeClient::WebKitBridgeClient(const BridgeOptions &options, QObject *parent)
    : QObject(parent), m_opts(options)
{
    if (m_opts.appId.isEmpty())
        m_opts.appId = QStringLiteral("default");
    QString bus = QStringLiteral("org.deepin.WebHelper.%1").arg(sanitizeNameToken(m_opts.appId));
    if (!m_opts.instanceId.isEmpty())
        bus += QStringLiteral(".%1").arg(sanitizeNameToken(m_opts.instanceId));
    m_busName = bus;
}

WebKitBridgeClient::~WebKitBridgeClient()
{
    shutdown();
    if (m_dpy) {
        XCloseDisplay(static_cast<Display *>(m_dpy));
        m_dpy = nullptr;
    }
}

bool WebKitBridgeClient::start(QString *error)
{
    if (m_proc) {
        if (error) *error = QStringLiteral("already started");
        return false;
    }
    if (m_opts.helperPath.isEmpty()) {
        m_opts.helperPath = QCoreApplication::applicationDirPath() + QStringLiteral("/webhelper");
    }
    if (!QFileInfo::exists(m_opts.helperPath)) {
        if (error) *error = QStringLiteral("webhelper not found: %1").arg(m_opts.helperPath);
        return false;
    }
    // 任务 3：启动前清理同 bus name 的残留实例，避免旧实例占用名称
    cleanupStaleHelpers(false);
    m_shuttingDown = false;
    spawn();
    return true;
}

void WebKitBridgeClient::shutdown()
{
    m_shuttingDown = true;
    if (m_proc) {
        m_proc->terminate();
        if (!m_proc->waitForFinished(2000))
            m_proc->kill();
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    m_ready = false;
}

void WebKitBridgeClient::attach(QWidget *container)
{
    m_container = container;
    m_anchor.clear();
    if (m_ready)
        embedAndLoad();
}

void WebKitBridgeClient::attach(QQuickItem *anchor)
{
    m_anchor = anchor;
    m_container.clear();
    if (m_ready)
        embedAndLoad();
}

void WebKitBridgeClient::spawn()
{
    m_proc = new QProcess(this);
    m_proc->setProgram(m_opts.helperPath);
    m_proc->setArguments({ QStringLiteral("--bus-name"), m_busName });
    if (m_opts.forwardHelperStderr)
        m_proc->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (m_opts.webChannelShim)
        env.insert(QStringLiteral("WEBHELPER_SHIM"), QStringLiteral("1"));
    m_proc->setProcessEnvironment(env);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &WebKitBridgeClient::handleStdout);
    connect(m_proc, &QProcess::finished, this,
            [this](int code, QProcess::ExitStatus) { handleProcessFinished(code); });
    connect(m_proc, &QProcess::errorOccurred, this, &WebKitBridgeClient::handleProcessError);
    m_proc->start();
}

void WebKitBridgeClient::handleStdout()
{
    if (!m_proc)
        return;
    while (m_proc->canReadLine()) {
        const QByteArray line = m_proc->readLine().trimmed();
        if (line.startsWith("READY")) {
            const QList<QByteArray> parts = line.split(' ');
            if (parts.size() >= 3) {
                m_webwin = parts[2].toULongLong();
                const bool wasRestarted = m_firstReadySeen;
                m_ready = true;
                m_firstReadySeen = true;
                m_stableTimer.restart();
                embedAndLoad();
                connectDBusSignals();
                if (wasRestarted) {
                    // 崩溃恢复：重新嵌入后重载最近页面
                    if (m_hasLastUri && !m_lastUri.isEmpty())
                        loadUri(m_lastUri);
                    emit helperRestarted(m_restartAttempt);
                    m_restartAttempt = 0;
                }
                emit ready();
            }
        } else if (line.startsWith("BUSY")) {
            // 名称被占：等待进程退出(3)后由 handleProcessFinished 强制清理重试
            qWarning() << "[webkitbridge]" << m_busName << "bus name busy, helper requested cleanup";
        }
    }
}

void WebKitBridgeClient::handleProcessError(QProcess::ProcessError err)
{
    if (m_shuttingDown)
        return;
    if (err == QProcess::FailedToStart) {
        m_proc->deleteLater();
        m_proc = nullptr;
        emit helperCrashed(QStringLiteral("failed to start: %1").arg(m_opts.helperPath));
    }
    // Crashed 由 finished() 统一处理，避免双发
}

void WebKitBridgeClient::handleProcessFinished(int exitCode)
{
    if (m_proc) {
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    if (m_shuttingDown)
        return;
    m_ready = false;
    if (m_iface) {
        m_iface->deleteLater();
        m_iface = nullptr;
    }

    // BUSY(3)：强制清理后重试一次（残留进程顽固占用 bus name）
    if (exitCode == kBusyExitCode && !m_busyRetried) {
        m_busyRetried = true;
        cleanupStaleHelpers(true);
        spawn();
        return;
    }
    m_busyRetried = false;

    const QString reason = exitCode == 0 ? QStringLiteral("exited(0)")
                                         : QStringLiteral("crashed(exit=%1)").arg(exitCode);
    emit helperCrashed(reason);
    // 任务 2：崩溃恢复——退避重启，重启完成后重新嵌入并重载页面
    scheduleRestart(reason);
}

void WebKitBridgeClient::scheduleRestart(const QString &reason)
{
    if (m_opts.restartBackoffMs.isEmpty() || m_restartAttempt >= m_opts.restartBackoffMs.size()) {
        qWarning() << "[webkitbridge]" << m_busName << "restart budget exhausted, giving up:" << reason;
        return;
    }
    const int delay = m_opts.restartBackoffMs.at(m_restartAttempt).toInt();
    ++m_restartAttempt;
    qWarning() << "[webkitbridge]" << m_busName << "helper died (" << reason
               << "), restarting in" << delay << "ms (attempt" << m_restartAttempt << ")";
    cleanupStaleHelpers(false);   // 清理可能残留的旧实例
    QTimer::singleShot(delay, this, [this] {
        if (!m_shuttingDown && !m_proc)
            spawn();
    });
}

void WebKitBridgeClient::connectDBusSignals()
{
    if (m_signalsConnected)
        return;   // 同一 well-known name，重启后 match 规则继续生效，重复 connect 会双收
    m_signalsConnected = true;
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.connect(m_busName, objectPath(), interfaceName(), "ScriptMessage", this,
                SIGNAL(scriptMessage(QString, QString)));
    bus.connect(m_busName, objectPath(), interfaceName(), "ConsoleMessage", this,
                SIGNAL(consoleMessage(QString, QString)));
    bus.connect(m_busName, objectPath(), interfaceName(), "LoadFinished", this,
                SIGNAL(loadFinished(QString, QString)));
    bus.connect(m_busName, objectPath(), interfaceName(), "WebFocused", this, SIGNAL(webFocused()));
    bus.connect(m_busName, objectPath(), interfaceName(), "ZoomChanged", this, SIGNAL(zoomChanged(double)));
    bus.connect(m_busName, objectPath(), interfaceName(), "ContextMenu", this,
                SIGNAL(contextMenuRequested(int, int, int)));
}

QDBusInterface *WebKitBridgeClient::iface()
{
    if (!m_iface && m_ready) {
        m_iface = new QDBusInterface(m_busName, objectPath(), interfaceName(),
                                     QDBusConnection::sessionBus(), this);
        m_iface->setTimeout(30000);
    }
    return m_iface;
}

void WebKitBridgeClient::embedAndLoad()
{
    if (!m_container && !m_anchor)
        return;
    if (!m_dpy)
        m_dpy = XOpenDisplay(nullptr);  // 存 void*
    if (!m_dpy) {
        qWarning() << "[webkitbridge] XOpenDisplay failed";
        return;
    }
    if (m_anchor) {
        connectItemSignals();
        QQuickWindow *win = m_anchor->window();
        if (!win) {
            // 宿主 QML 窗口尚未创建（如应用窗口还未 show）：挂 windowChanged，
            // 待窗口建立后再完成嵌入，避免对空窗口取 winId。
            connect(m_anchor.data(), &QQuickItem::windowChanged, this, [this](QQuickWindow *) {
                if (m_anchor && m_anchor->window())
                    embedAndLoad();
            }, Qt::UniqueConnection);
            return;
        }
        win->installEventFilter(this);
        m_qtWin = (quint64)win->winId();   // 宿主顶层窗（transient 挂靠目标）
    } else {
        qApp->installEventFilter(this);   // 容器 Move/Resize → 浮层几何同步
        m_qtWin = (quint64)m_container->window()->winId();   // 宿主顶层窗（transient 挂靠目标）
    }
    if (m_webwin) {
        /* 浮层模式：helper 是 kwin 受管真窗（transient + COMBO 悬浮在宿主之上）。
         * 实测 reparent 嵌入窗（含非 OR）会被 kwin 立即回写 X 焦点，键盘/IM 失效，
         * 故不再 ReparentWindow；几何由全局坐标持续同步。 */
        if (auto *i = iface())
            QDBusPendingCall(i->asyncCall(QStringLiteral("EmbedNotify"),
                                          QVariant::fromValue<quint64>(m_qtWin)));
        syncGeometry();
    }
}

void WebKitBridgeClient::connectItemSignals()
{
    if (!m_anchor || m_itemSignalsConnected)
        return;
    m_itemSignalsConnected = true;
    // 锚点几何/位置变化 + 所在窗口变化 → 浮层几何同步
    connect(m_anchor.data(), &QQuickItem::xChanged, this, [this] { syncItemGeometry(); });
    connect(m_anchor.data(), &QQuickItem::yChanged, this, [this] { syncItemGeometry(); });
    connect(m_anchor.data(), &QQuickItem::widthChanged, this, [this] { syncItemGeometry(); });
    connect(m_anchor.data(), &QQuickItem::heightChanged, this, [this] { syncItemGeometry(); });
    connect(m_anchor.data(), &QQuickItem::parentChanged, this, [this] { syncItemGeometry(); });
    connect(m_anchor.data(), &QQuickItem::visibleChanged, this, [this] { syncItemGeometry(); });
    connect(m_anchor.data(), &QQuickItem::windowChanged, this, [this](QQuickWindow *w) {
        if (w)
            w->installEventFilter(this);
        syncItemGeometry();
    });
}

void WebKitBridgeClient::syncGeometry()
{
    if (!m_dpy || !m_webwin)
        return;
    if (m_anchor) {
        syncItemGeometry();
        return;
    }
    if (!m_container)
        return;
    const QSize s = m_container->size();
    const qreal dpr = m_container->devicePixelRatioF();
    const int w = qMax(1, qRound(s.width() * dpr));
    const int h = qMax(1, qRound(s.height() * dpr));
    const QPoint gp = m_container->mapToGlobal(QPoint(0, 0));   // 浮层窗口用全局坐标
    XMoveResizeWindow(static_cast<Display *>(m_dpy), (Window)m_webwin,
                      qRound(gp.x() * dpr), qRound(gp.y() * dpr), w, h);
    XFlush(static_cast<Display *>(m_dpy));
}

void WebKitBridgeClient::syncItemGeometry()
{
    if (!m_dpy || !m_webwin || !m_anchor)
        return;
    QQuickWindow *win = m_anchor->window();
    if (!win || !m_anchor->isVisible())
        return;
    const qreal dpr = win->devicePixelRatio();
    const int w = qMax(1, qRound(m_anchor->width() * dpr));
    const int h = qMax(1, qRound(m_anchor->height() * dpr));
    const QPointF gp = m_anchor->mapToGlobal(QPointF(0, 0));   // 浮层窗口用全局坐标
    XMoveResizeWindow(static_cast<Display *>(m_dpy), (Window)m_webwin,
                      qRound(gp.x() * dpr), qRound(gp.y() * dpr), w, h);
    XFlush(static_cast<Display *>(m_dpy));
}

bool WebKitBridgeClient::eventFilter(QObject *obj, QEvent *ev)
{
    if (m_ready && ev->type() == QEvent::Move) {
        if (m_container && obj == m_container)
            syncGeometry();
        else if (m_anchor && obj == m_anchor->window())
            syncItemGeometry();
    } else if (m_ready && ev->type() == QEvent::Resize) {
        if (m_container && obj == m_container)
            syncGeometry();
        else if (m_anchor && obj == m_anchor->window())
            syncItemGeometry();
    } else if (m_ready && ev->type() == QEvent::Show) {
        if (m_anchor && obj == m_anchor->window())
            syncItemGeometry();
    }
    // 焦点说明（勿在 FocusIn 里反向 XSetInputFocus）：focus 进入 helper 子窗时，
    // Qt toplevel 也会收到 detail=Inferior 的 FocusIn，此时反向拉焦点会与
    // helper 的 FocusWeb 竞争导致焦点乒乓。Qt↔web 焦点切换由 X click-to-focus
    // 与 focusWeb() 各自天然处理，宿主无需干预。
    return QObject::eventFilter(obj, ev);
}

void WebKitBridgeClient::loadUri(const QString &uri)
{
    m_lastUri = uri;
    m_hasLastUri = !uri.isEmpty();
    if (auto *i = iface())
        i->asyncCall(QStringLiteral("LoadURI"), uri);
}

void WebKitBridgeClient::loadHtml(const QString &html, const QString &baseUri)
{
    m_lastUri.clear();
    m_hasLastUri = false;
    if (auto *i = iface())
        i->asyncCall(QStringLiteral("LoadHTML"), html, baseUri);
}

void WebKitBridgeClient::evalJS(const QString &script, EvalCallback callback)
{
    auto *i = iface();
    if (!i) {
        if (callback) callback(QString(), QStringLiteral("helper not ready"));
        return;
    }
    QDBusPendingCall call = i->asyncCall(QStringLiteral("EvalJS"), script);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, callback](QDBusPendingCallWatcher *w) {
                QDBusPendingReply<QString> reply = *w;
                w->deleteLater();
                if (reply.isError()) {
                    const QDBusError e = reply.error();
                    // 任务 5：容忍 undefined 结果被部分版本包装成 Code601/"Unsupported result type"
                    if (e.name().contains(QLatin1String("Code601")) ||
                        e.message().contains(QLatin1String("Unsupported result type"))) {
                        if (callback) callback(QStringLiteral("undefined"), QString());
                    } else if (callback) {
                        callback(QString(), e.message());
                    }
                    return;
                }
                if (callback) callback(reply.value(), QString());
            });
}

QString WebKitBridgeClient::evalJSSync(const QString &script, int timeoutMs, QString *err)
{
    auto *i = iface();
    if (!i) {
        if (err) *err = QStringLiteral("helper not ready");
        return QString();
    }
    i->setTimeout(timeoutMs);
    const QDBusMessage r = i->call(QDBus::Block, QStringLiteral("EvalJS"), script);
    if (r.type() == QDBusMessage::ReplyMessage)
        return r.arguments().value(0).toString();
    if (err)
        *err = r.errorMessage();
    return QString();
}

void WebKitBridgeClient::printToPdf(const QString &path, BoolCallback callback)
{
    auto *i = iface();
    if (!i) {
        if (callback) callback(false, QStringLiteral("helper not ready"));
        return;
    }
    auto *watcher = new QDBusPendingCallWatcher(i->asyncCall(QStringLiteral("PrintToPdf"), path), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [callback](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<bool> reply = *w;
        w->deleteLater();
        if (reply.isError())
            callback(false, reply.error().message());
        else
            callback(reply.value(), QString());
    });
}

void WebKitBridgeClient::printToPdfEx(const QString &uri, const QString &path, PrintCallback callback)
{
    auto *i = iface();
    if (!i) {
        if (callback) callback({});
        return;
    }
    auto *watcher = new QDBusPendingCallWatcher(
        i->asyncCall(QStringLiteral("PrintToPdfEx"), uri, path), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [callback](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<bool, QString, quint64, double> reply = *w;
        w->deleteLater();
        PrintResult r;
        if (reply.isError()) {
            r.err = reply.error().message();
        } else {
            r.ok = reply.argumentAt<0>();
            r.err = reply.argumentAt<1>();
            r.bytes = reply.argumentAt<2>();
            r.ms = reply.argumentAt<3>();
        }
        callback(r);
    });
}

WebKitBridgeClient::PrintResult WebKitBridgeClient::printToPdfExSync(const QString &uri,
                                                                     const QString &path,
                                                                     int timeoutMs, QString *err)
{
    PrintResult r;
    auto *i = iface();
    if (!i) {
        r.err = QStringLiteral("helper not ready");
        if (err) *err = r.err;
        return r;
    }
    i->setTimeout(timeoutMs);
    const QDBusMessage msg = i->call(QDBus::Block, QStringLiteral("PrintToPdfEx"), uri, path);
    if (msg.type() == QDBusMessage::ReplyMessage && msg.arguments().size() >= 4) {
        r.ok = msg.arguments().at(0).toBool();
        r.err = msg.arguments().at(1).toString();
        r.bytes = msg.arguments().at(2).toULongLong();
        r.ms = msg.arguments().at(3).toDouble();
    } else {
        r.err = msg.errorMessage();
        if (err) *err = r.err;
    }
    return r;
}

void WebKitBridgeClient::snapshot(const QString &path, BoolCallback callback)
{
    auto *i = iface();
    if (!i) {
        if (callback) callback(false, QStringLiteral("helper not ready"));
        return;
    }
    auto *watcher = new QDBusPendingCallWatcher(i->asyncCall(QStringLiteral("Snapshot"), path), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [callback](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<bool> reply = *w;
        w->deleteLater();
        if (reply.isError())
            callback(false, reply.error().message());
        else
            callback(reply.value(), QString());
    });
}

WebKitBridgeClient::PrintResult WebKitBridgeClient::snapshotSync(const QString &path,
                                                                 int timeoutMs, QString *err)
{
    PrintResult r;
    auto *i = iface();
    if (!i) {
        r.err = QStringLiteral("helper not ready");
        if (err) *err = r.err;
        return r;
    }
    i->setTimeout(timeoutMs);
    const QDBusMessage msg = i->call(QDBus::Block, QStringLiteral("Snapshot"), path);
    if (msg.type() == QDBusMessage::ReplyMessage)
        r.ok = msg.arguments().value(0).toBool();
    else {
        r.err = msg.errorMessage();
        if (err) *err = r.err;
    }
    return r;
}

void WebKitBridgeClient::focusWeb()
{
    if (auto *i = iface())
        i->asyncCall(QStringLiteral("FocusWeb"));
}

void WebKitBridgeClient::executeEditCommand(const QString &command)
{
    if (auto *i = iface())
        i->asyncCall(QStringLiteral("ExecuteEditCommand"), command);
}

void WebKitBridgeClient::setZoomLevel(double level)
{
    if (auto *i = iface())
        i->asyncCall(QStringLiteral("SetZoom"), level);
}

void WebKitBridgeClient::setBackgroundColor(const QColor &color)
{
    if (auto *i = iface())
        i->asyncCall(QStringLiteral("SetBackgroundColor"), color.name(QColor::HexRgb));
}

void WebKitBridgeClient::cleanupStaleHelpers(bool forceKill)
{
    // 扫描 /proc：匹配 argv[0] basename == "webhelper" 且 --bus-name 后紧跟本桥名称的进程
    QDir procDir(QStringLiteral("/proc"));
    QVector<pid_t> victims;
    const QStringList entries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &e : entries) {
        bool ok = false;
        const pid_t pid = e.toInt(&ok);
        if (!ok || pid == getpid())
            continue;
        QFile f(QStringLiteral("/proc/%1/cmdline").arg(pid));
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const QList<QByteArray> argvList = f.readAll().split('\0');
        f.close();
        if (argvList.isEmpty())
            continue;
        const QString argv0 = QString::fromLocal8Bit(argvList.first());
        if (!argv0.endsWith(QStringLiteral("webhelper")))
            continue;
        for (int i = 1; i < argvList.size() - 1; ++i) {
            if (argvList.at(i) == "--bus-name" && argvList.at(i + 1) == m_busName.toLocal8Bit()) {
                victims.append(pid);
                break;
            }
        }
    }
    if (victims.isEmpty())
        return;
    qWarning() << "[webkitbridge]" << m_busName << "cleanup stale helpers:" << victims;
    for (pid_t pid : victims)
        kill(pid, SIGTERM);
    // 等待退出；forceKill 或未退出者升级 SIGKILL
    const int waitMs = forceKill ? 500 : 1200;
    for (int waited = 0; waited < waitMs; waited += 100) {
        bool alive = false;
        for (pid_t pid : victims)
            if (kill(pid, 0) == 0) alive = true;
        if (!alive)
            return;
        QThread::msleep(100);
    }
    for (pid_t pid : victims)
        kill(pid, SIGKILL);
    QThread::msleep(200);
    // 等待 D-Bus 名称释放（进程死亡到 daemon 处理断连有短暂窗口）
    auto *iface = QDBusConnection::sessionBus().interface();
    for (int waited = 0; waited < 3000; waited += 150) {
        if (!iface->isServiceRegistered(m_busName))
            return;
        QThread::msleep(150);
    }
    qWarning() << "[webkitbridge]" << m_busName << "stale name still owned after cleanup";
}

} // namespace webkitbridge
