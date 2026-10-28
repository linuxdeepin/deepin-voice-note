<!-- SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
     SPDX-License-Identifier: GPL-3.0-or-later -->

# WebKitGTK 桥公共库

把 WebKitGTK（独立进程 + D-Bus 桥）封装为 Qt 应用可直接复用的公共库，用于替代
QtWebEngine 的轻量化场景。技术方案与实验依据见
`../WebKitGTK桥接替代QtWebEngine-实验验证报告.md`。

## 结构

```
helper/webhelper.c          独立 WebKitGTK 进程（纯 C）：D-Bus 桥、mdimg scheme、console 钩子
host/webkitbridgeclient.*   宿主侧 Qt6 封装（唯一接入点）：进程管理/嵌入/焦点/崩溃恢复
demo/demo-host.cpp          自动验收演示宿主（[RESULT] 结果行）
demo/test.html              全链路验收页；demo/im-test.html  输入法验证页
tools/dbusprobe.py          外部直连桥调试工具（call/eval/load/pdfex/snap/focus/value）
tools/selftest.sh           一键验收：构建+全链路+外部焦点+崩溃恢复+输入法（15 项）
tools/im-verify.sh          输入法协作专项验证
docs/bridge-api.md          API 参考；docs/im-verification.md  IM 验证报告
```

## 构建与验收

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./tools/selftest.sh          # 退出码 = 失败项数；需 X11 图形会话（IM 步骤需 fcitx5）
```

## 接入示例

```cpp
using webkitbridge::WebKitBridgeClient;
WebKitBridgeClient c({QStringLiteral("your-appid")});
c.attach(ui->webContainer);
QString err; c.start(&err);
c.loadUri("file:///path/page.html");
c.focusWeb();
```

## 关键设计（与原型的差异）

- **嵌入模型：浮层真窗**。实测 kwin_x11 会立即回收 Reparent 嵌入窗（含非
  override-redirect）的 X 输入焦点，键盘/输入法全失效；改为受管顶层窗（COMBO +
  transient-for 宿主）悬浮于容器上方，几何全局坐标同步——观感一致，焦点/IM 走标准
  WM 路径。详见 `docs/im-verification.md`。
- **输入法已验证可用**：fcitx5/搜狗组合态→提交→焦点跟随全链路 PASS（原型的 IM
  高风险项闭环）。
- EvalJS `undefined` → 字符串 `"undefined"`（非错误）；UI 进程禁止创建独立
  JSCContext（JSON 解析宿主用 QJsonDocument、helper 用纯 C）。
- 崩溃恢复：退避重启（1s→16s，稳定 60s 重置），重嵌入并重载最近页面。
- bus name 唯一性：残留 helper 清理 + BUSY 退出码 3 重试。

## 验收现状（selftest 15/15）

自定义 scheme、EvalJS×100（avg≈0.23ms）、几何同步、ScriptMessage 反向通道、
console 捕获、undefined 容忍、PrintToPdfEx（≈93080 bytes/26ms）、Snapshot、
FocusWeb 焦点路由、外部焦点核查、崩溃恢复×4、输入法协作×5。

## 已知边界

X11 专用（kwin_x11 验证）；Wayland 未适配。其余见 `docs/bridge-api.md` §6。
