// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tiptap 运行时入口脚本。
// 结构分层：
//   - 编辑器初始化模块（createTiptapEditor）：建立并长期保留
//   - 通道绑定模块（bindTiptapChannel）：正式 QWebChannel 接口
//   - 工具栏模块（createFormatToolbar）：富文本格式 + 资源插入区
//   - 图片交互模块（setupImagePaste）：粘贴落盘往返 / 远程阻止

import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from './tiptap-extensions.js'
import { createEmptyDoc } from '../schema/document-envelope.js'
import { bindTiptapChannel } from './tiptap-channel.js'
import { createFormatToolbar } from './format-toolbar.js'
import { setupImagePaste, setupImageViewAndMenu } from './image-interactions.js'

// ---------------------------------------------------------------------------
// 编辑器初始化模块（本接口不替换此模块）
// ---------------------------------------------------------------------------

function createTiptapEditor(element) {
  return new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
}

// ---------------------------------------------------------------------------
// 启动
// ---------------------------------------------------------------------------

const editor = createTiptapEditor(document.getElementById('app'))
const toolbar = createFormatToolbar(editor, document.getElementById('toolbar-host'))

bindTiptapChannel(editor, undefined, {
  onFontList: (fonts, defaultFont) => toolbar.setFontList(fonts, defaultFont),
}).then((bridge) => {
  // 工具栏图片按钮 → 宿主打开文件选择
  toolbar.setOnPickImage(() => bridge.jsRequestPickImage && bridge.jsRequestPickImage())
  // 粘贴：剪贴板图片落盘往返、远程图片阻止
  setupImagePaste(editor, bridge)
  // 双击查看原图、右键菜单（查看原图 / 删除）
  setupImageViewAndMenu(editor, bridge)
})
