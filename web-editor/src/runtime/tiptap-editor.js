// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// TTP-022: Tiptap 运行时入口脚本。
// 结构分层：
//   - 编辑器初始化模块（createTiptapEditor）：022 建立并长期保留
//   - 通道绑定模块（bindTiptapTempChannel）：022 临时最小通道，TTP-023 将替换

import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from './tiptap-extensions.js'
import {
  createEmptyDoc,
  createEnvelope,
  serializeEnvelope,
} from '../schema/document-envelope.js'

// ---------------------------------------------------------------------------
// 编辑器初始化模块（TTP-022，TTP-023 不替换此模块）
// ---------------------------------------------------------------------------

function createTiptapEditor(element) {
  return new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
}

// ---------------------------------------------------------------------------
// 通道绑定模块（TTP-022 临时最小通道，TTP-023 将替换为正式通道）
// ---------------------------------------------------------------------------

function bindTiptapTempChannel(editor) {
  // QWebChannel 由 qwebchannel.js 提供（Qt WebEngine 全局）
  new QWebChannel(qt.webChannelTransport, function (channel) {
    const bridge = channel.objects.tiptapTemp

    // Editor 初始化完成 → 通知 C++
    bridge.jsEditorReady()

    // C++ → JS：加载 envelope
    bridge.callLoadEnvelope.connect(function (json) {
      let envelope
      try {
        envelope = JSON.parse(json)
      } catch {
        envelope = createEnvelope(createEmptyDoc())
      }
      const content = envelope?.content || createEmptyDoc()
      editor.commands.setContent(content)
    })

    // C++ → JS：请求保存
    bridge.callRequestContent.connect(function () {
      const envelope = createEnvelope(editor.getJSON())
      bridge.jsContentSaved(serializeEnvelope(envelope))
    })
  })
}

// ---------------------------------------------------------------------------
// 启动
// ---------------------------------------------------------------------------

const editor = createTiptapEditor(document.getElementById('app'))
bindTiptapTempChannel(editor)
