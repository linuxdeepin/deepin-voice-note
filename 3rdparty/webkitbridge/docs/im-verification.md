<!-- SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
     SPDX-License-Identifier: GPL-3.0-or-later -->

# 嵌入态输入法（IM）协作验证报告（im-verification.md）

对应任务最高风险项：**WebKitGTK 独立进程嵌入 Qt 应用后，fcitx5/搜狗输入法是否可用**。
结论：**可用**，且发现了并修复了原型 IM 链路不通的根因。

## 1. 根因：kwin_x11 回收非受管窗的 X 输入焦点

原型采用 `ReparentWindow` 把 helper 窗口嵌进 Qt 容器。实测（kwin_x11，UOS V25）：

| 方案 | X 焦点能否落在 web 窗 | 键盘/IM |
|---|---|---|
| `GTK_WINDOW_POPUP`（override-redirect）+ Reparent | 否，~10ms 内回写 | 全部失效 |
| `GTK_WINDOW_TOPLEVEL`（非 OR）+ Reparent | 否，`xdotool windowfocus` 也立即回写 | 全部失效 |
| **浮层真窗**（受管 TOPLEVEL + COMBO + transient-for 宿主） | **是，`_NET_ACTIVE_WINDOW`=helper 且稳定** | **全链路可用** |

kwin 的行为：X 输入焦点必须落在"当前激活的受管 client"上；reparent 进来的子窗不是
client，谁把焦点设过去都立刻被 kwin 写回激活窗（本桥 helper、独立 `xdotool` 双向验证）。
这不是本库代码问题，而是 X11 嵌入在该 WM 下的硬约束——真实受管顶层窗（如
`webhelper --debug`）则可正常持有焦点。

因此产品化嵌入模型改为**浮层真窗**（详见 `docs/bridge-api.md` §2）：
helper 顶层窗（COMBO 类型，无任务栏/无装饰）+ `XSetTransientForHint` 挂靠宿主顶层窗
（kwin 保持其覆盖宿主之上）+ 宿主全局坐标几何同步。观感与 Reparent 嵌入一致。

## 2. 输入法链路（已验证）

```
用户点击 web 区 / focusWeb()（gtk_window_present → kwin 激活真窗）
  → GDK focus-in → WebKit webview 获焦 → GTK IM module (fcitx) 激活 IC
  → 键盘事件由 X 送 helper（当前激活窗）→ fcitx5 组合/预编辑 → 提交到 web 输入框
```

自动化验证：`tools/im-verify.sh`（结果行 `[RESULT]`，退出码=失败数）：

1. 启动 demo（`build/im-test.html`：输入事件全量日志 `window.__imlog`）。
2. FocusWeb 激活 helper（防抢焦点拦截时用 `wmctrl -i -a` pager 源兜底），EvalJS 聚焦 `#q`。
3. `fcitx5-remote -o` 激活拼音 → `xdotool type "nihao"` → 组合态截图 → 空格提交。
4. EvalJS 读回提交值与事件日志。

实测结果（UOS V25 + fcitx5 + 搜狗，kwin_x11）：

```
[RESULT] PASS im-focusin                # focusin 事件到达页面
[RESULT] PASS im-composition (preedit)  # 组合态（composing=1 input）出现
[RESULT] PASS im-composition-end        # compositionend 出现
[RESULT] PASS im-commit (CJK 提交)      # 拼音上屏中文写入网页输入框（EvalJS 读回）
[RESULT] PASS im-focus-switch           # #q ↔ #textarea 焦点切换，IC 跟随
```

- 断言为**机制级**：候选词由搜狗词库/云联想决定，不固定，故不断言具体文字，
  只断言"组合事件出现 + 非 ASCII 提交值"。实测多轮均提交出中文（含 "你好"）。
- WebKitGTK 对 fcitx 直提交流程派发 `input(composing=1)` + `compositionend`，
  **不派发 `compositionstart`**，取证断言以 `composing=1` 为准（页面日志可证）。
- 多输入框焦点切换时 fcitx IC 正确跟随（focusout/focusin 序列见
  `tools/im-artifacts/im-focus-switch.log`）。
- 组合态与提交后页面快照：`tools/im-artifacts/preedit.png`、`committed.png`。

## 3. 产品内使用注意

- **用户点击触发无 FSP 问题**：后台进程调 `FocusWeb` 可能被 kwin 防抢焦点（FSP）拦截
  （`_NET_ACTIVE_WINDOW` source=application 被拒）——这是 WM 的正确行为。产品内
  FocusWeb 总是发生在用户已与宿主交互的场景（宿主是激活窗，其 transient 激活被允许）；
  自动化测试才需要 pager 源兜底。
- helper 进程需继承会话环境（`DISPLAY/GTK_IM_MODULE/QT_IM_MODULE/XMODIFIERS`），
  宿主勿清洗环境变量。
- 中文候选词浮窗由 fcitx/搜狗自绘（独立 X 窗口），不进入页面截图，属正常。

## 4. 复跑方式

```bash
./tools/im-verify.sh                  # 单独验证
./tools/selftest.sh                   # 第 5 步含 IM 验证（15 项全绿）
```
