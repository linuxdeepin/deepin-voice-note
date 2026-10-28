<!-- SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
     SPDX-License-Identifier: GPL-3.0-or-later -->

# WebKitGTK 桥公共库 — API 参考（bridge-api.md）

宿主侧唯一接入点：`webkitbridge::WebKitBridgeClient`（`host/webkitbridgeclient.h`）。
接入方无需直接接触 D-Bus、X11、进程管理。

## 1. 快速上手

```cpp
#include "webkitbridgeclient.h"
using webkitbridge::WebKitBridgeClient;
using webkitbridge::BridgeOptions;

WebKitBridgeClient c({QStringLiteral("deepin-editor")});   // appId 用于 bus name 与残留清理
c.attach(ui->webContainer);                // 占位容器 widget（可为空布局）
QString err;
if (!c.start(&err)) { /* err 含失败原因 */ }

connect(&c, &WebKitBridgeClient::loadFinished, this,
        [](const QString &uri, const QString &status) { /* status=="ok" 或错误串 */ });

c.loadUri(QStringLiteral("file:///path/page.html"));
c.focusWeb();                              // 焦点移交 web 区（输入法链路）
```

## 2. 嵌入模型（重要）

**浮层真窗**：helper 的 WebKit 窗口是 kwin 受管的独立顶层窗（`_NET_WM_WINDOW_TYPE_COMBO`
+ `XSetTransientForHint(宿主顶层窗)`），悬浮在容器正上方；宿主把容器的全局坐标与尺寸
持续同步给 helper（`SetGeometry`，容器 Move/Resize 事件驱动，自动乘 devicePixelRatio）。

> 为什么不用 XReparentWindow 嵌入：实测 kwin_x11 对**非受管窗口**（含非
> override-redirect 的 reparent 子窗）会立即回收 X 输入焦点，键盘事件与输入法全部失效；
> 原型即因此 IM 验证不通过。浮层真窗走标准 WM 焦点路径，输入法全链路可用（见
> `docs/im-verification.md`）。观感上与嵌入一致：无任务栏项、无装饰、随容器几何浮动。

对 UI 的含义：
- 容器必须实际显示（`mapToGlobal` 才有意义）；容器移动/缩放会自动跟随。
- 点击 web 区域时 kwin 自然激活 helper（输入法、滚动、右键都走 helper 进程）；
  点击 Qt 区域时焦点回宿主，无需任何额外代码。

## 3. D-Bus 接口（webhelper 侧，仅供理解，勿直连）

- bus name：`org.deepin.WebHelper.<appId>[.<instance>]`（非法字符转 `_`，数字开头补前缀）
- object path：`/org/deepin/WebHelper`；interface：`org.deepin.WebHelper.Bridge`
- 方法：`EvalJS(s→s)` `LoadURI(s)` `LoadHTML(ss)` `SetGeometry(iiii)` `PrintToPdf(s→bs)`
  `PrintToPdfEx(s→sbstd)` `Snapshot(s→b)` `FocusWeb()` `EmbedNotify(t)`
- 信号：`ScriptMessage(ss)` `WebFocused()` `ConsoleMessage(ss)` `LoadFinished(ss)`

`EvalJS` 约定：结果为 `undefined` 时回字符串 `"undefined"`（**非错误**）；JS 异常时
D-Bus error（宿主侧回调 err 非空）。UI 进程**禁止**创建独立 `JSCContext` 解析结果——
解析一律在宿主侧用 `QJsonDocument` / `QJsonValue`，helper 侧用纯 C JSON（双端不得引入
第二个 JSC VM，这是与 QtWebEngine 行为对齐的红线）。

## 4. 环境变量

| 变量 | 作用 |
|---|---|
| `WEBHELPER_SHIM=1` | helper 注入 WebChannel 兼容垫片（window.qt.webChannelTransport） |
| `BRIDGE_DEMO_PAGE=<file>` | demo 启动后自动加载的页面（默认 build/test.html） |
| `BRIDGE_AUTOTEST=1` | demo 跑全链路自动验收（结果行以 `[RESULT]` 输出到 stderr） |
| `BRIDGE_AUTOTEST=crash` | 崩溃恢复自测（配合 `BRIDGE_SELF_CRASH_MS=N`） |
| `WEBHELPER_SELF_CRASH_MS=N` | helper 运行 N ms 后自杀（测试用；宿主经 `BRIDGE_SELF_CRASH_MS` 透传） |

## 5. 自测与验证

```bash
./tools/selftest.sh          # 构建 + 全链路 + 外部焦点 + 崩溃恢复 + 输入法协作（15 项）
./tools/im-verify.sh         # 单独跑输入法协作验证（需图形会话 + fcitx5 + wmctrl）
python3 tools/dbusprobe.py --app <appId> call '1+1'   # 外部进程直连桥调试
```

`selftest.sh` 结果行以 `[RESULT]` 输出，退出码 = 失败项数，可直接接 CI。

## 6. 已知边界

- X11 会话专用（kwin_x11 / 深度桌面验证通过）；Wayland 未适配。
- 崩溃重启退避 1s→2s→4s→8s→16s，稳定运行 60s 后重置计数；重启后自动重嵌入并重载最近页面。
- helper 日志中的 `GLib-GObject-CRITICAL g_value_set_boxed`、`dbind AT-SPI` 警告与
  `XError code=173 GLXBadWindow`（嵌入期）为已知无害噪音。
- 相对 URI 在 `PrintToPdfEx` 中会失败，必须传完整 URI。
