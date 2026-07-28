// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tiptap 运行时入口脚本。
// 结构分层：
//   - 编辑器初始化模块（createTiptapEditor）：建立并长期保留
//   - 通道绑定模块（bindTiptapChannel）：正式 QWebChannel 接口

import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from './tiptap-extensions.js'
import { createEmptyDoc } from '../schema/document-envelope.js'
import { bindTiptapChannel } from './tiptap-channel.js'

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
bindTiptapChannel(editor)
