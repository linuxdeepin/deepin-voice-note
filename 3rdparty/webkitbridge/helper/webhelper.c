// SPDX-License-Identifier: GPL-3.0-or-later
// webhelper.c — WebKitGTK(4.1) 桥 helper（产品化版）
// 由《WebKitGTK桥接替代QtWebEngine-实验验证报告》原型 webhelper.c 演化而来。
// 职责：每应用一个常驻 GTK3 进程，持有 WebKitWebView，经 D-Bus 暴露统一 Web 能力，
//       X11 窗口交宿主应用 Reparent 嵌入；几何一律由宿主驱动（GTK 不自行改分配）。
// 产品化点（对应任务工作项一 10 条）：
//   1) FocusWeb → gtk_widget_grab_focus + XSetInputFocus（IM 链路前提）
//   2) SIGTERM/SIGINT 优雅退出；崩溃恢复由宿主负责（本进程提供 SELF_CRASH 测试钩子）
//   3) bus name 按appId 唯一；启动时若名称被占（BUSY）立即退出码 3，由宿主清理后重试
//   4) 打印：gtk_enumerate_printers 枚举（禁止硬编码打印机名）；PrintToPdfEx 任务式
//      接口：先加载待 LoadFinished、20s 超时、回包 (ok,err,bytes,ms) 结构化
//   5) EvalJS 容忍 undefined 结果（含 "Unsupported result type"/Code601 包装）→ 回 "undefined"
//   6) script-message-received 按 WebKitGTK 2.54 签名（WebKitJavascriptResult）
//   7) console 捕获：JS 侧用户脚本劫持 console.*/onerror（2.54 已移除 console-message）；
//      自定义 scheme URI percent-decode（中文路径）
//   8) 几何同步：仅暴露 SetGeometry 供宿主调用，helper 不提供自助改尺寸
//   9) 工程化：--debug 独立调试模式；X11 错误自定义 handler（默认 handler 会 exit）
//  10) UI 进程禁止创建独立 JSCContext（与 WebKit 内部 VM 冲突），JSON 解析用纯 C
//
// 命令行：webhelper --bus-name org.deepin.WebHelper.<appId> [--debug [uri]] [--shim] [--width W] [--height H]
// 环境变量：
//   WEBHELPER_SHIM=1            注入 WebChannel 兼容垫片（同 --shim）
//   WEBHELPER_NOHOOK=1          关闭 console 钩子注入（调试用）
//   WEBHELPER_SELF_CRASH_MS=N   启动 N ms 后 abort()（宿主崩溃恢复自测钩子）
// stdout 握手：READY <busname> <xid> ；名称被占：BUSY <busname>（退出码 3）
#define GDK_DISABLE_DEPRECATION_WARNINGS
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <webkit2/webkit2.h>
#include <jsc/jsc.h>
#include <gio/gio.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib-unix.h>
#include <signal.h>
#include <math.h>
#include <X11/Xlib.h>

#define OBJ_PATH   "/org/deepin/WebHelper"
#define IFACE_NAME "org.deepin.WebHelper.Bridge"

static WebKitWebView *g_webview = NULL;
static GtkWidget *g_win = NULL;
static GDBusConnection *g_bus = NULL;
static gchar *g_bus_name = NULL;
static guint g_reg_id = 0;

static const gchar *g_introspection_xml =
    "<node>"
    "  <interface name='" IFACE_NAME "'>"
    "    <method name='EvalJS'>"
    "      <arg name='script' type='s' direction='in'/>"
    "      <arg name='result' type='s' direction='out'/>"
    "    </method>"
    "    <method name='LoadURI'><arg name='uri' type='s' direction='in'/></method>"
    "    <method name='LoadHTML'>"
    "      <arg name='html' type='s' direction='in'/><arg name='baseUri' type='s' direction='in'/>"
    "    </method>"
    "    <method name='SetGeometry'>"
    "      <arg name='x' type='i' direction='in'/><arg name='y' type='i' direction='in'/>"
    "      <arg name='w' type='i' direction='in'/><arg name='h' type='i' direction='in'/>"
    "    </method>"
    "    <method name='PrintToPdf'>"
    "      <arg name='path' type='s' direction='in'/><arg name='ok' type='b' direction='out'/>"
    "    </method>"
    "    <method name='PrintToPdfEx'>"
    "      <arg name='uri' type='s' direction='in'/><arg name='path' type='s' direction='in'/>"
    "      <arg name='ok' type='b' direction='out'/><arg name='err' type='s' direction='out'/>"
    "      <arg name='bytes' type='t' direction='out'/><arg name='ms' type='d' direction='out'/>"
    "    </method>"
    "    <method name='Snapshot'>"
    "      <arg name='path' type='s' direction='in'/><arg name='ok' type='b' direction='out'/>"
    "    </method>"
    "    <method name='FocusWeb'/>"
    "    <method name='EmbedNotify'><arg name='hostXid' type='t' direction='in'/></method>"
    "    <method name='ExecuteEditCommand'><arg name='command' type='s' direction='in'/></method>"
    "    <method name='SetZoom'><arg name='level' type='d' direction='in'/></method>"
    "    <method name='SetBackgroundColor'><arg name='color' type='s' direction='in'/></method>"
    "    <signal name='ScriptMessage'><arg name='name' type='s'/><arg name='body' type='s'/></signal>"
    "    <signal name='WebFocused'/>"
    "    <signal name='ConsoleMessage'><arg name='level' type='s'/><arg name='text' type='s'/></signal>"
    "    <signal name='LoadFinished'><arg name='uri' type='s'/><arg name='status' type='s'/></signal>"
    "    <signal name='ZoomChanged'><arg name='level' type='d'/></signal>"
    "    <signal name='ContextMenu'>"
    "      <arg name='x' type='i'/><arg name='y' type='i'/><arg name='ctx' type='i'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static void emit_sig(const gchar *name, GVariant *params)
{
    if (g_bus)
        g_dbus_connection_emit_signal(g_bus, NULL, OBJ_PATH, IFACE_NAME, name, params, NULL);
}

/* ---------- X11 错误 handler：GTK 默认 handler 会直接 exit 进程 ---------- */
static int wh_xerror_handler(Display *d, XErrorEvent *e)
{
    char buf[256] = {0};
    XGetErrorText(d, e->error_code, buf, sizeof(buf) - 1);
    fprintf(stderr, "[webhelper] XError: code=%d msg=%s req=%d win=%lu\n",
            e->error_code, buf, e->request_code, (unsigned long)e->resourceid);
    fflush(stderr);
    return 0;
}

static Window helper_xid(void)
{
    GdkWindow *gw = gtk_widget_get_window(g_win);
    return gw ? gdk_x11_window_get_xid(gw) : None;
}

/* ---------- mdimg:// 自定义 scheme（对应 QWebEngineUrlSchemeHandler） ---------- */
/* 仅 image/* 白名单 + 存在性校验；URI 中非 ASCII 路径为 percent-encoded，必须解码 */
static void mdimg_cb(WebKitURISchemeRequest *req, gpointer ud)
{
    const gchar *uri = webkit_uri_scheme_request_get_uri(req);
    /* webkit 对自定义 scheme 不做 Blink 式 host 推断：mdimg:///abs/path → /abs/path */
    const gchar *raw = uri + strlen("mdimg:");
    while (raw[0] == '/' && raw[1] == '/')
        raw++;
    gchar *decoded = g_uri_unescape_string(raw, NULL);
    const gchar *path = decoded ? decoded : raw;

    GError *err = NULL;
    GFile *f = g_file_new_for_path(path);
    GFileInfo *info = g_file_query_info(f, G_FILE_ATTRIBUTE_STANDARD_SIZE "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                        G_FILE_QUERY_INFO_NONE, NULL, &err);
    if (!info || g_file_info_get_file_type(info) != G_FILE_TYPE_REGULAR) {
        if (!err)
            err = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "not a regular file");
        webkit_uri_scheme_request_finish_error(req, err);
        g_object_unref(f);
        g_free(decoded);
        return;
    }
    const gchar *ctype = g_content_type_guess(path, NULL, 0, NULL);
    gchar *mime = ctype ? g_content_type_get_mime_type(ctype) : NULL;
    if (!mime || !g_str_has_prefix(mime, "image/")) {
        webkit_uri_scheme_request_finish_error(req,
            g_error_new(G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED, "non-image mime denied"));
        g_free(mime); g_object_unref(info); g_object_unref(f); g_free(decoded);
        return;
    }
    GFileInputStream *fs = g_file_read(f, NULL, &err);
    if (!fs) {
        webkit_uri_scheme_request_finish_error(req, err);
        g_free(mime); g_object_unref(info); g_object_unref(f); g_free(decoded);
        return;
    }
    webkit_uri_scheme_request_finish(req, G_INPUT_STREAM(fs),
                                     g_file_info_get_size(info), mime);
    g_object_unref(fs); g_free(mime); g_object_unref(info); g_object_unref(f); g_free(decoded);
}

/* ---------- EvalJS（runJavaScript 等价物，异步挂起 D-Bus 回复） ---------- */
static void eval_done(GObject *src, GAsyncResult *res, gpointer ud)
{
    GDBusMethodInvocation *inv = G_DBUS_METHOD_INVOCATION(ud);
    GError *err = NULL;
    JSCValue *v = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(src), res, &err);
    if (err) {
        /* 产品化：容忍 undefined 结果被包装成的错误（部分版本 "Unsupported result type"/
         * Code601；实测 2.54 异常走 WebKitJavascriptError/Code699，不得误吞真异常）。
         * undefined 统一回字符串 "undefined"，由宿主封装层按需转空值。 */
        if (err->code == 601 || strstr(err->message, "Unsupported result type")) {
            g_dbus_method_invocation_return_value(inv, g_variant_new("(s)", "undefined"));
            g_error_free(err);
            return;
        }
        g_dbus_method_invocation_return_gerror(inv, err);
        return;
    }
    gchar *s = v ? jsc_value_to_string(v) : g_strdup("undefined");
    g_dbus_method_invocation_return_value(inv, g_variant_new("(s)", s ? s : ""));
    g_free(s);
}

/* ---------- Snapshot（HTML→图片，mail webpdfconvertimage 场景） ---------- */
typedef struct { GDBusMethodInvocation *inv; gchar *path; } SnapCtx;

static void snap_done(GObject *src, GAsyncResult *res, gpointer ud)
{
    SnapCtx *ctx = (SnapCtx *)ud;
    GError *err = NULL;
    cairo_surface_t *surf = webkit_web_view_get_snapshot_finish(WEBKIT_WEB_VIEW(src), res, &err);
    gboolean ok = FALSE;
    if (err)
        fprintf(stderr, "[webhelper] snapshot failed: %s\n", err->message);
    if (surf) {
        ok = cairo_surface_write_to_png(surf, ctx->path) == CAIRO_STATUS_SUCCESS;
        cairo_surface_destroy(surf);
    }
    g_dbus_method_invocation_return_value(ctx->inv, g_variant_new("(b)", ok));
    g_object_unref(ctx->inv);
    g_free(ctx->path);
    g_free(ctx);
}

/* ---------- PrintToPdf（GTK "Print to File" 后端，reader htmltopdf 场景） ---------- */
typedef struct {
    GDBusMethodInvocation *inv;
    gchar *outpath;
    gchar *err;
    gboolean done;
    gboolean structured;    /* PrintToPdfEx 用 (b s t d) 结构化回包 */
    gint64  t0;
} PrintJob;

static PrintJob *g_pdf_pending = NULL;  /* 等待页面加载完成后打印的任务 */

/* 系统 GTK dev 包缺 gtkprinter.h，手动声明（符号在 libgtk-3 中） */
typedef struct _GtkPrinter GtkPrinter;
typedef gboolean (*GtkPrinterFunc)(GtkPrinter *printer, gpointer data);
extern const char *gtk_printer_get_name(GtkPrinter *p);
extern gboolean gtk_printer_accepts_pdf(GtkPrinter *p);
extern void gtk_enumerate_printers(GtkPrinterFunc func, gpointer data, GDestroyNotify destroy, gboolean wait);

static gchar *g_printer_name = NULL;   /* 第一次打印时枚举选出并缓存 */

static gboolean pick_printer_cb(GtkPrinter *p, gpointer d)
{
    if (gtk_printer_accepts_pdf(p)) {
        g_free(g_printer_name);
        g_printer_name = g_strdup(gtk_printer_get_name(p));
        return TRUE;                    /* 找到即停 */
    }
    return FALSE;
}

static void print_return(PrintJob *j, gboolean ok, const gchar *err)
{
    if (j->done) return;          /* failed 与 finished 可能都触发，只回一次 */
    j->done = TRUE;
    if (g_pdf_pending == j) g_pdf_pending = NULL;
    if (err && !j->err) j->err = g_strdup(err);
    if (j->structured) {
        guint64 size = 0;
        if (ok && j->outpath) {
            GFile *f = g_file_new_for_path(j->outpath);
            GFileInfo *fi = g_file_query_info(f, G_FILE_ATTRIBUTE_STANDARD_SIZE, 0, NULL, NULL);
            if (fi) { size = g_file_info_get_size(fi); g_object_unref(fi); }
            g_object_unref(f);
        }
        gdouble ms = (g_get_monotonic_time() - j->t0) / 1000.0;
        g_dbus_method_invocation_return_value(j->inv,
            g_variant_new("(bstd)", ok, j->err ? j->err : "", size, ms));
    } else {
        g_dbus_method_invocation_return_value(j->inv, g_variant_new("(b)", ok));
    }
    g_object_unref(j->inv);
    g_free(j->outpath);
    g_free(j->err);
    g_free(j);
}

static void on_print_finished(WebKitPrintOperation *op, gpointer ud)
{
    print_return((PrintJob *)ud, TRUE, NULL);
}

static gboolean on_print_failed(WebKitPrintOperation *op, GError *err, gpointer ud)
{
    fprintf(stderr, "[webhelper] print failed: %s (code %d)\n", err ? err->message : "?", err ? err->code : -1);
    print_return((PrintJob *)ud, FALSE, err ? err->message : "print failed");
    return TRUE;
}

static void start_pdf_print(PrintJob *j)
{
    GtkPrintSettings *s = gtk_print_settings_new();
    /* 打印机名随 locale 变化（"Print to File"/"打印到文件"），禁止硬编码：枚举选第一个支持 PDF 的 */
    if (!g_printer_name)
        gtk_enumerate_printers(pick_printer_cb, NULL, NULL, TRUE);
    if (g_printer_name) {
        fprintf(stderr, "[webhelper] using printer '%s'\n", g_printer_name);
        gtk_print_settings_set_printer(s, g_printer_name);
    }
    gtk_print_settings_set(s, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");
    gchar *ouri = g_strdup_printf("file://%s", j->outpath);
    gtk_print_settings_set(s, GTK_PRINT_SETTINGS_OUTPUT_URI, ouri);
    g_free(ouri);
    WebKitPrintOperation *op = webkit_print_operation_new(g_webview);
    webkit_print_operation_set_print_settings(op, s);
    g_signal_connect(op, "finished", G_CALLBACK(on_print_finished), j);
    g_signal_connect(op, "failed", G_CALLBACK(on_print_failed), j);
    webkit_print_operation_print(op);
    g_object_unref(s);
}

static gboolean pdf_task_timeout(gpointer ud)
{
    if (g_pdf_pending == (PrintJob *)ud) {
        g_pdf_pending = NULL;
        print_return((PrintJob *)ud, FALSE, "load timeout (20s)");
    }
    return G_SOURCE_REMOVE;
}

/* ---------- 页面加载信号：业务页排障与加载同步的关键通道 ---------- */
static void on_load_changed(WebKitWebView *w, WebKitLoadEvent ev, gpointer ud)
{
    if (ev != WEBKIT_LOAD_FINISHED) return;
    const gchar *uri = webkit_web_view_get_uri(w);
    emit_sig("LoadFinished", g_variant_new("(ss)", uri ? uri : "", "ok"));
    if (g_pdf_pending) {
        PrintJob *j = g_pdf_pending;
        g_pdf_pending = NULL;
        start_pdf_print(j);
    }
}

static gboolean on_load_failed(WebKitWebView *w, WebKitLoadEvent ev, const gchar *failing_uri,
                               GError *err, gpointer ud)
{
    emit_sig("LoadFinished", g_variant_new("(ss)", failing_uri ? failing_uri : "",
                                           err ? err->message : "load failed"));
    if (g_pdf_pending) {
        PrintJob *j = g_pdf_pending;
        g_pdf_pending = NULL;
        print_return(j, FALSE, err ? err->message : "load failed");
    }
    return FALSE;                 /* 返回 FALSE 让默认错误页照常显示 */
}

/* ---------- WebChannel 等价物：script message → D-Bus 信号 ---------- */
/* 提取 hook 消息的 level/text（格式自控：{"type":"console","level":"X","text":"Y"}）。
 * 注：UI 进程禁止建独立 JSCContext——会与 WebKit 内部 VM 冲突，导致
 * evaluate_javascript 永不返回（实测踩坑）。解析信号 JSON 一律用纯 C。 */
static void parse_console_json(const gchar *s, gchar **level, gchar **text)
{
    const gchar *p = strstr(s, "\"level\":\"");
    if (p) {
        p += 9;
        const gchar *e = strchr(p, '"');
        if (e) *level = g_strndup(p, e - p);
    }
    p = strstr(s, "\"text\":\"");
    if (p) {
        p += 8;
        GString *b = g_string_new("");
        while (*p && *p != '"') {
            if (*p == '\\' && p[1]) {
                p++;
                switch (*p) {
                case 'n': g_string_append_c(b, '\n'); break;
                case 't': g_string_append_c(b, '\t'); break;
                case 'r': case 'b': case 'f': break;
                default: g_string_append_c(b, *p); break;   /* 含 \" \\ 及 \uXXXX 原样保留 */
                }
                p++;
            } else {
                g_string_append_c(b, *p++);
            }
        }
        *text = g_string_free(b, FALSE);
    }
}

/* WebKitGTK 2.54 签名：WebKitJavascriptResult（非旧文档的 JSCValue） */
static void on_script_msg(WebKitUserContentManager *m, WebKitJavascriptResult *result, gpointer ud)
{
    JSCValue *value = result ? webkit_javascript_result_get_js_value(result) : NULL;
    gchar *s = value ? jsc_value_to_string(value) : g_strdup("");
    /* console 钩子消息（JS 侧注入）→ 转发为 ConsoleMessage 信号，与其余消息分流 */
    if (s && strncmp(s, "{\"type\":\"console\"", 17) == 0) {
        gchar *level = NULL, *text = NULL;
        parse_console_json(s, &level, &text);
        fprintf(stderr, "[wh:console] %s: %s\n", level ? level : "?", text ? text : "");
        emit_sig("ConsoleMessage", g_variant_new("(ss)", level ? level : "?", text ? text : ""));
        g_free(level); g_free(text);
    } else {
        emit_sig("ScriptMessage", g_variant_new("(ss)", (const gchar *)ud, s ? s : ""));
    }
    g_free(s);
}

/* ---------- 焦点（IM 链路）：GTK grab_focus + X input focus ---------- */
/* FocusWeb / web 区点击共用：grab_focus 触发 GTK focus-in → IM 上下文激活（fcitx/ibus），
 * XSetInputFocus 把 X 层输入焦点交给 helper 窗口。 */
static void focus_web_internal(void)
{
    /* 本窗是 kwin 受管真窗（浮层模式）：走标准激活路径（_NET_ACTIVE_WINDOW +
     * WM_TAKE_FOCUS），再由 GTK 把焦点交给 webview 并激活其 IM 上下文。
     * 直接 XSetInputFocus 会被 kwin 立即回写（窗口非 active client 时）。 */
    gtk_window_present(GTK_WINDOW(g_win));
    gtk_widget_grab_focus(GTK_WIDGET(g_webview));
    emit_sig("WebFocused", NULL);
}

static gboolean on_web_button(GtkWidget *w, GdkEventButton *e, gpointer ud)
{
    focus_web_internal();
    return FALSE;
}

/* ---------- 右键菜单 / 缩放 / 拖放（voice-note 集成新增） ---------- */
/* 右键：抑制 WebKit 默认菜单，宿主用 JS 内省 + 既有 QML 菜单流程自行弹出。
 * 坐标转 webview 本地坐标；ctx 位掩码对齐 WebKitHitTestResultContext：
 * document=2, link=4, image=8, media=16, editable=32, scrollbar=64, selection=128 */
#define WH_CTX_DOCUMENT 2
#define WH_CTX_LINK 4
#define WH_CTX_IMAGE 8
#define WH_CTX_MEDIA 16
#define WH_CTX_EDITABLE 32
#define WH_CTX_SCROLLBAR 64
#define WH_CTX_SELECTION 128

static gboolean emit_ctx_menu_idle(gpointer ud)
{
    GdkEventButton *e = (GdkEventButton *)ud;
    WebKitHitTestResult *htr = (WebKitHitTestResult *)g_object_get_data(G_OBJECT(g_webview), "wh-last-htr");
    guint ctx = 0;
    if (htr) {
        guint flags = 0;
        g_object_get(htr, "context", &flags, NULL);
        if (flags & WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT) ctx |= WH_CTX_DOCUMENT;
        if (flags & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK) ctx |= WH_CTX_LINK;
        if (flags & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE) ctx |= WH_CTX_IMAGE;
        if (flags & WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA) ctx |= WH_CTX_MEDIA;
        if (flags & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE) ctx |= WH_CTX_EDITABLE;
        if (flags & WEBKIT_HIT_TEST_RESULT_CONTEXT_SCROLLBAR) ctx |= WH_CTX_SCROLLBAR;
        if (flags & WEBKIT_HIT_TEST_RESULT_CONTEXT_SELECTION) ctx |= WH_CTX_SELECTION;
        gboolean content_editable = FALSE;
        g_object_get(htr, "context-is-content-editable", &content_editable, NULL);
        if (content_editable) ctx |= WH_CTX_EDITABLE;
    }
    /* 延迟到 DOM contextmenu 事件处理器跑完后再发（Summernote 页面在 DOM 事件里
     * 先发 popupMenu 携带菜单类型，D-Bus 按序到达，宿主处理时数据已就绪） */
    emit_sig("ContextMenu", g_variant_new("(iii)",
             (gint)(e ? lround(e->x) : 0), (gint)(e ? lround(e->y) : 0), (gint)ctx));
    g_free(e);
    return G_SOURCE_REMOVE;
}

static gboolean on_context_menu(WebKitWebView *w, WebKitContextMenu *menu,
                                GdkEventButton *event, WebKitHitTestResult *htr,
                                gpointer ud)
{
    if (htr)
        g_object_set_data_full(G_OBJECT(g_webview), "wh-last-htr",
                               g_object_ref(htr), (GDestroyNotify)g_object_unref);
    GdkEventButton *copy = g_new0(GdkEventButton, 1);
    if (event) *copy = *event;
    g_idle_add_full(G_PRIORITY_DEFAULT, emit_ctx_menu_idle, copy, NULL);
    return TRUE; /* 阻止默认菜单 */
}

static void on_zoom_level_changed(GObject *obj, GParamSpec *pspec, gpointer ud)
{
    gdouble level = webkit_web_view_get_zoom_level(g_webview);
    emit_sig("ZoomChanged", g_variant_new("(d)", level));
}

/* 拖放：文件(uri-list) → ScriptMessage {"type":"dnd","x":..,"y":..,"urls":[..]}
 * 文本(text/plain|html) → ScriptMessage {"type":"dnd","x":..,"y":..,"text":".."}
 * 文本透传给页面由宿主 evalJS 插入；文件由宿主走既有插入图片流水线。 */
static const GtkTargetEntry dnd_targets[] = {
    { (gchar *)"text/uri-list", 0, 0 },
    { (gchar *)"text/plain", 0, 1 },
    { (gchar *)"text/html", 0, 2 },
};

static gchar *json_escape_and_quote(const gchar *s)
{
    GString *b = g_string_new("\"");
    for (const guchar *p = (const guchar *)(s ? s : ""); *p; p++) {
        switch (*p) {
        case '"': g_string_append(b, "\\\""); break;
        case '\\': g_string_append(b, "\\\\"); break;
        case '\n': g_string_append(b, "\\n"); break;
        case '\r': g_string_append(b, "\\r"); break;
        case '\t': g_string_append(b, "\\t"); break;
        default:
            if (*p < 0x20) g_string_append_printf(b, "\\u%04x", *p);
            else g_string_append_c(b, *p);
        }
    }
    g_string_append_c(b, '"');
    return g_string_free(b, FALSE);
}

static void on_drag_data_received(GtkWidget *w, GdkDragContext *dc,
                                  gint x, gint y, GtkSelectionData *sel,
                                  guint info, guint t, gpointer ud)
{
    const guchar *raw = gtk_selection_data_get_data(sel);
    gint fmt = gtk_selection_data_get_format(sel);
    gint len = gtk_selection_data_get_length(sel);
    if (!raw || len <= 0 || fmt != 8) {
        gtk_drag_finish(dc, FALSE, FALSE, t);
        return;
    }
    gchar *payload = NULL;
    if (info == 0) { /* uri-list */
        gchar **uris = g_uri_list_extract_uris((const gchar *)raw);
        GString *arr = g_string_new("[");
        for (gchar **u = uris; u && *u; u++) {
            if (u != uris) g_string_append_c(arr, ',');
            gchar *q = json_escape_and_quote(*u);
            g_string_append(arr, q);
            g_free(q);
        }
        g_string_append_c(arr, ']');
        payload = g_strdup_printf("{\"type\":\"dnd\",\"x\":%d,\"y\":%d,\"urls\":%s}",
                                  x, y, arr->str);
        g_string_free(arr, TRUE);
        g_strfreev(uris);
    } else { /* text/plain 或 text/html */
        gchar *text = g_strndup((const gchar *)raw, (gsize)len);
        gchar *q = json_escape_and_quote(text);
        payload = g_strdup_printf("{\"type\":\"dnd\",\"x\":%d,\"y\":%d,\"text\":%s}",
                                  x, y, q);
        g_free(text);
        g_free(q);
    }
    emit_sig("ScriptMessage", g_variant_new("(ss)", "bridge", payload));
    g_free(payload);
    gtk_drag_finish(dc, TRUE, FALSE, t);
}

static gboolean on_drag_drop(GtkWidget *w, GdkDragContext *dc, gint x, gint y,
                             guint t, gpointer ud)
{
    GdkAtom target = gtk_drag_dest_find_target(w, dc, NULL);
    if (target == GDK_NONE)
        return FALSE;
    gtk_drag_get_data(w, dc, target, t);
    return TRUE;
}

static gboolean on_drag_motion(GtkWidget *w, GdkDragContext *dc, gint x, gint y,
                               guint t, gpointer ud)
{
    GdkAtom target = gtk_drag_dest_find_target(w, dc, NULL);
    if (target == GDK_NONE)
        return FALSE;
    gdk_drag_status(dc, GDK_ACTION_COPY, t);
    return TRUE;
}

/* ---------- D-Bus 方法分发 ---------- */
static void method_call(GDBusConnection *conn, const gchar *sender, const gchar *obj,
                        const gchar *iface, const gchar *method, GVariant *params,
                        GDBusMethodInvocation *inv, gpointer ud)
{
    if (!strcmp(method, "EvalJS")) {
        const gchar *script;
        g_variant_get(params, "(&s)", &script);
        webkit_web_view_evaluate_javascript(g_webview, script, -1, NULL, NULL, NULL,
                                            eval_done, g_object_ref(inv));
    } else if (!strcmp(method, "LoadURI")) {
        const gchar *uri;
        g_variant_get(params, "(&s)", &uri);
        webkit_web_view_load_uri(g_webview, uri);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(method, "LoadHTML")) {
        const gchar *html, *base;
        g_variant_get(params, "(&s&s)", &html, &base);
        webkit_web_view_load_html(g_webview, html, base && *base ? base : NULL);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(method, "EmbedNotify")) {
        /* 浮层模式（实测结论：kwin_x11 不允许 reparent 进来的非受管窗持有 X 焦点，
         * 键盘/IM 全部失效；helper 必须是真实受管顶层窗）：
         *   - COMBO 类型：无任务栏项、无装饰
         *   - transient-for 宿主 toplevel：kwin 保持本窗覆盖在宿主之上
         *   - 几何由宿主以全局坐标持续同步（SetGeometry） */
        guint64 host_xid = 0;
        if (params)
            g_variant_get(params, "(t)", &host_xid);
        gtk_window_set_type_hint(GTK_WINDOW(g_win), GDK_WINDOW_TYPE_HINT_COMBO);
        Display *d = gdk_x11_get_default_xdisplay();
        Window xid = helper_xid();
        if (d && xid != None && host_xid)
            XSetTransientForHint(d, xid, (Window)host_xid);
        gtk_widget_show_all(g_win);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(method, "SetGeometry")) {
        /* 几何一律由宿主驱动（与 Qt 布局模型一致）；helper 不提供自助改尺寸方法 */
        gint x, y, w, h;
        g_variant_get(params, "(iiii)", &x, &y, &w, &h);
        Display *d = gdk_x11_get_default_xdisplay();
        Window xid = helper_xid();
        if (d && xid != None)
            XMoveResizeWindow(d, xid, x, y, w < 1 ? 1 : w, h < 1 ? 1 : h);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(method, "PrintToPdf")) {
        const gchar *path;
        g_variant_get(params, "(&s)", &path);
        PrintJob *j = g_new0(PrintJob, 1);
        j->inv = g_object_ref(inv);
        j->outpath = g_strdup(path);
        j->t0 = g_get_monotonic_time();
        start_pdf_print(j);
    } else if (!strcmp(method, "PrintToPdfEx")) {
        /* 任务式打印：uri 非空则先加载待 LoadFinished 再打印；回包含错误/字节数/耗时 */
        const gchar *uri, *path;
        g_variant_get(params, "(&s&s)", &uri, &path);
        PrintJob *j = g_new0(PrintJob, 1);
        j->inv = g_object_ref(inv);
        j->outpath = g_strdup(path);
        j->structured = TRUE;
        j->t0 = g_get_monotonic_time();
        if (uri && *uri) {
            g_pdf_pending = j;
            webkit_web_view_load_uri(g_webview, uri);
            g_timeout_add_seconds(20, pdf_task_timeout, j);
        } else {
            start_pdf_print(j);
        }
    } else if (!strcmp(method, "Snapshot")) {
        const gchar *path;
        g_variant_get(params, "(&s)", &path);
        SnapCtx *ctx = g_new0(SnapCtx, 1);
        ctx->inv = g_object_ref(inv);
        ctx->path = g_strdup(path);
        webkit_web_view_get_snapshot(g_webview, WEBKIT_SNAPSHOT_REGION_VISIBLE,
                                     WEBKIT_SNAPSHOT_OPTIONS_NONE, NULL,
                                     snap_done, ctx);
    } else if (!strcmp(method, "FocusWeb")) {
        focus_web_internal();
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(method, "ExecuteEditCommand")) {
        /* WebKit 编辑命令：无 "Editor" 前缀（Cut/Copy/Paste/SelectAll/Undo/Redo…） */
        const gchar *command;
        g_variant_get(params, "(&s)", &command);
        webkit_web_view_execute_editing_command(g_webview, command);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(method, "SetZoom")) {
        gdouble level = 1.0;
        g_variant_get(params, "(d)", &level);
        webkit_web_view_set_zoom_level(g_webview, level);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(method, "SetBackgroundColor")) {
        /* 页面加载前的底色跟随主题（避免浅色主题下闪深色） */
        const gchar *color;
        g_variant_get(params, "(&s)", &color);
        GdkRGBA rgba;
        if (gdk_rgba_parse(&rgba, color))
            webkit_web_view_set_background_color(g_webview, &rgba);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else {
        g_dbus_method_invocation_return_dbus_error(inv,
            IFACE_NAME ".Error.UnknownMethod", method);
    }
}

/* ---------- SIGTERM/SIGINT 优雅退出（崩溃恢复自测也依赖干净退出） ---------- */
static gboolean on_unix_signal(gpointer ud)
{
    fprintf(stderr, "[webhelper] got terminate signal, quitting\n");
    gtk_main_quit();
    return G_SOURCE_REMOVE;
}

/* ---------- 崩溃恢复自测钩子：WEBHELPER_SELF_CRASH_MS=N → N ms 后 abort() ---------- */
static gboolean self_crash_cb(gpointer ud)
{
    fprintf(stderr, "[webhelper] SELF-CRASH triggered (test hook)\n");
    fflush(stderr);
    abort();
    return G_SOURCE_REMOVE;
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    g_set_prgname("webhelper");

    const gchar *busname = NULL;
    gboolean debug_mode = FALSE, shim = FALSE;
    int dbg_w = 800, dbg_h = 500;
    const gchar *dbg_uri = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--bus-name") && i + 1 < argc)
            busname = argv[++i];
        else if (!strcmp(argv[i], "--debug")) {
            debug_mode = TRUE;
            if (i + 1 < argc && argv[i + 1][0] != '-')
                dbg_uri = argv[++i];
        } else if (!strcmp(argv[i], "--shim"))
            shim = TRUE;
        else if (!strcmp(argv[i], "--width") && i + 1 < argc)
            dbg_w = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--height") && i + 1 < argc)
            dbg_h = atoi(argv[++i]);
    }
    if (!busname)
        busname = debug_mode ? "org.deepin.WebHelper.debug" : "org.deepin.WebHelper.default";
    if (g_getenv("WEBHELPER_SHIM"))
        shim = TRUE;

    fprintf(stderr, "[webhelper] starting pid=%d bus=%s debug=%d shim=%d GTK_IM_MODULE=%s QT_IM_MODULE=%s\n",
            getpid(), busname, debug_mode, shim,
            g_getenv("GTK_IM_MODULE") ? g_getenv("GTK_IM_MODULE") : "(unset)",
            g_getenv("QT_IM_MODULE") ? g_getenv("QT_IM_MODULE") : "(unset)");

    XSetErrorHandler(wh_xerror_handler);

    WebKitWebContext *ctx = webkit_web_context_new();
    webkit_web_context_register_uri_scheme(ctx, "mdimg", mdimg_cb, NULL, NULL);

    WebKitUserContentManager *ucm = webkit_user_content_manager_new();
    webkit_user_content_manager_register_script_message_handler(ucm, "bridge");
    g_signal_connect(ucm, "script-message-received::bridge", G_CALLBACK(on_script_msg), (gpointer)"bridge");

    g_webview = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                              "web-context", ctx,
                                              "user-content-manager", ucm, NULL));
    /* voice-note 等以 file:// 加载本地页 + file:// 资源（图片/音频）的页面需要 */
    WebKitSettings *settings = webkit_web_view_get_settings(g_webview);
    webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
    webkit_settings_set_enable_developer_extras(settings, FALSE);
    /* 嵌入模式用 TOPLEVEL 仅 realize 不 map（实测 GTK_WINDOW_POPUP 的 override-redirect
     * 窗口会被 kwin_x11 强制回收 X 输入焦点，键盘事件无法进入 web，IM 失效）。
     * 宿主先 ReparentWindow 进容器再调 EmbedNotify，由 GTK 自己 show——映射时父窗
     * 已是容器（非 root 子窗），kwin 不感知，无装饰、不注册 client。 */
    g_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(g_win), dbg_w, dbg_h);
    gtk_container_add(GTK_CONTAINER(g_win), GTK_WIDGET(g_webview));
    g_signal_connect(g_webview, "button-press-event", G_CALLBACK(on_web_button), NULL);
    g_signal_connect(g_webview, "load-changed", G_CALLBACK(on_load_changed), NULL);
    g_signal_connect(g_webview, "load-failed", G_CALLBACK(on_load_failed), NULL);
    g_signal_connect(g_webview, "context-menu", G_CALLBACK(on_context_menu), NULL);
    g_signal_connect(g_webview, "notify::zoom-level", G_CALLBACK(on_zoom_level_changed), NULL);
    /* 拖放接管：覆盖 WebKit 默认目的（uri-list 默认会被当作文本插入可编辑区） */
    gtk_drag_dest_set(GTK_WIDGET(g_webview), GTK_DEST_DEFAULT_DROP,
                      dnd_targets, G_N_ELEMENTS(dnd_targets), GDK_ACTION_COPY);
    g_signal_connect(g_webview, "drag-motion", G_CALLBACK(on_drag_motion), NULL);
    g_signal_connect(g_webview, "drag-drop", G_CALLBACK(on_drag_drop), NULL);
    g_signal_connect(g_webview, "drag-data-received", G_CALLBACK(on_drag_data_received), NULL);

    /* 脚本 A（恒注入）：console 钩子 —— WebKitGTK 2.54 已移除 console-message 信号，
     * 改用 JS 侧劫持 console.* + window.onerror/unhandledrejection，经 script 通道回传 */
    const gchar *console_hook =
        "(function(){"
        " var send=function(lv,args){try{var p=[];"
        "  for(var i=0;i<args.length;i++){var a=args[i];"
        "   p.push(typeof a==='string'?a:JSON.stringify(a));}"
        "  var h=window.webkit&&window.webkit.messageHandlers&&window.webkit.messageHandlers.bridge;"
        "  h&&h.postMessage(JSON.stringify({type:'console',level:lv,text:p.join(' ').slice(0,600)}));"
        " }catch(e){}};"
        " ['log','info','warn','error','debug'].forEach(function(lv){"
        "  var orig=console[lv]?console[lv].bind(console):function(){};"
        "  console[lv]=function(){send(lv==='warn'?'warning':(lv==='info'?'message':lv),arguments);"
        "   orig.apply(null,arguments);};});"
        " window.addEventListener('error',function(e){"
        "  send('error',['[window.onerror] '+(e.message||'')+' @'+(e.filename||'')+':'+(e.lineno||0)]);});"
        " window.addEventListener('unhandledrejection',function(e){"
        "  send('error',['[unhandledrejection] '+(e.reason&&e.reason.message||e.reason)]);});"
        "})();";
    WebKitUserScript *us = g_getenv("WEBHELPER_NOHOOK") ? NULL : webkit_user_script_new(console_hook,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
    if (us) { webkit_user_content_manager_add_script(ucm, us); webkit_user_script_unref(us); }

    /* 脚本 B（恒注入）：QtWebChannel 等价 transport —— 页内 qwebchannel.js 的双向桥：
     *   JS→宿主：transport.send(msg) → bridge script 消息 → D-Bus ScriptMessage
     *   宿主→JS：宿主 EvalJS 调 __wcDeliverChannel(base64) → transport.onmessage
     * 真正的 QWebChannel 服务端仍在 UI 进程（TiptapChannelBridge 等 QObject），
     * base64 中转避免 JSON 字面量转义与多字节字符问题 */
    const gchar *transportjs =
        "(function(){"
        " var H=window.webkit&&window.webkit.messageHandlers&&window.webkit.messageHandlers.bridge;"
        " if(!H)return;"
        " var transport={onmessage:null,send:function(m){H.postMessage(String(m));}};"
        " window.__wcDeliverChannel=function(b64){"
        "  var json=decodeURIComponent(escape(atob(b64)));"
        "  if(transport.onmessage){try{transport.onmessage({data:json});}catch(e){}}"
        " };"
        " window.qt={webChannelTransport:transport};"
        "})();";
    WebKitUserScript *us_transport = webkit_user_script_new(transportjs,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
    webkit_user_content_manager_add_script(ucm, us_transport);
    webkit_user_script_unref(us_transport);

    /* --shim / WEBHELPER_SHIM=1：追加注入 WebChannel 兼容垫片（qt 桩 + QWebChannel 空
     * transport 容错），使依赖 WebChannel 的业务页不致启动即抛异常，便于适配诊断 */
    if (shim) {
        const gchar *shimjs =
            "(function(){"
            " if(!window.qt) window.qt={webChannelTransport:null,__shim:true};"
            " Object.defineProperty(window,'QWebChannel',{configurable:true,"
            "  set:function(real){this.__realQWC=real;},"
            "  get:function(){var real=this.__realQWC;"
            "   return function(t,cb,ui){"
            "    if(!t){setTimeout(function(){cb&&cb({objects:{}});},0);return;}"
            "    return real?real(t,cb,ui):undefined;};}});})();";
        WebKitUserScript *us2 = webkit_user_script_new(shimjs,
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
        webkit_user_content_manager_add_script(ucm, us2);
        webkit_user_script_unref(us2);
        fprintf(stderr, "[webhelper] WebChannel shim injected\n");
    }
    if (debug_mode) {
        gtk_widget_show_all(g_win);
    } else {
        gtk_widget_realize(GTK_WIDGET(g_win));
        gtk_widget_realize(GTK_WIDGET(g_webview));
    }
    if (dbg_uri && *dbg_uri)
        webkit_web_view_load_uri(g_webview, dbg_uri);

    g_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!g_bus) {
        fprintf(stderr, "[webhelper] FATAL: cannot connect to session bus\n");
        return 2;
    }
    g_bus_name = g_strdup(busname);
    /* 名称被占（残留实例/双宿主）：先查后占；先占用再检查会把自身误判为残留实例（任务 3：bus name 唯一性） */
    {
        GError *owner_err = NULL;
        GVariant *owner = g_dbus_connection_call_sync(g_bus, "org.freedesktop.DBus",
                                "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner",
                                g_variant_new("(s)", g_bus_name), G_VARIANT_TYPE("(s)"),
                                G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &owner_err);
        if (owner) {
            const gchar *o = NULL;
            g_variant_get(owner, "(&s)", &o);
            g_print("BUSY %s\n", g_bus_name);
            fflush(stdout);
            fprintf(stderr, "[webhelper] bus name '%s' owned by %s (stale instance?), exiting 3\n",
                    g_bus_name, o ? o : "?");
            return 3;
        }
        g_clear_error(&owner_err);
    }
    g_bus_own_name_on_connection(g_bus, g_bus_name, G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL);
    GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(g_introspection_xml, NULL);
    static GDBusInterfaceVTable vtable = { method_call, NULL, NULL };
    g_reg_id = g_dbus_connection_register_object(g_bus, OBJ_PATH,
                                                 info->interfaces[0], &vtable, NULL, NULL, NULL);

    g_unix_signal_add(SIGTERM, on_unix_signal, NULL);
    g_unix_signal_add(SIGINT, on_unix_signal, NULL);
    const gchar *crashms = g_getenv("WEBHELPER_SELF_CRASH_MS");
    if (crashms && atoi(crashms) > 0)
        g_timeout_add(atoi(crashms), self_crash_cb, NULL);

    /* stdout 握手：READY <busname> <xid> */
    g_print("READY %s %lu\n", g_bus_name, (unsigned long)helper_xid());
    fflush(stdout);
    fprintf(stderr, "[webhelper] ready bus=%s xid=%lu\n", g_bus_name, (unsigned long)helper_xid());

    gtk_main();
    return 0;
}
