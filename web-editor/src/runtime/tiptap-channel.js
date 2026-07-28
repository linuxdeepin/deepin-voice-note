// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 正式 QWebChannel 绑定模块（channel.objects.tiptapChannel）。
// 绑定五类基础事件，替换临时通道。
// 命名稳定契约：后续能力迁移只增不改、不重命名。
// 已知限制：适配层 resolveResourceUrl 将相对路径解析为 file:// URL，
// 但 document-envelope.js 的 isSafeImageSrc 仅允许 http/https scheme。
// 本 PR 保存往返未调用 validateEnvelope，不构成即时 bug；
// 后续接入校验时需放宽安全 scheme 白名单以含 file://。

import {
  createEmptyDoc,
  createEnvelope,
  serializeEnvelope,
} from '../schema/document-envelope.js'
import { parseImageInfo, parseVoiceInfo } from './tiptap-adapter.js'

const CONTENT_CHANGE_DEBOUNCE_MS = 200

/**
 * 创建防抖函数：在指定延迟后调用 fn，仅最后一次调用生效。
 * @param {Function} fn
 * @param {number} delay
 * @returns {{ trigger: () => void, cancel: () => void, flush: () => void }}
 */
export function createDebounce(fn, delay) {
  let timer = null
  let pending = false

  return {
    trigger() {
      pending = true
      if (timer) clearTimeout(timer)
      timer = setTimeout(() => {
        timer = null
        if (pending) {
          pending = false
          fn()
        }
      }, delay)
    },
    cancel() {
      pending = false
      if (timer) {
        clearTimeout(timer)
        timer = null
      }
    },
    flush() {
      if (timer) {
        clearTimeout(timer)
        timer = null
      }
      if (pending) {
        pending = false
        fn()
      }
    },
  }
}

/**
 * 绑定正式 QWebChannel 通道（channel.objects.tiptapChannel）。
 * @param {Editor} editor — Tiptap Editor 实例
 * @param {object} [channelFactory] — 可选，测试注入；默认使用全局 QWebChannel
 * @returns {Promise<object>} resolve 为 bridge 对象
 */
export function bindTiptapChannel(editor, channelFactory) {
  return new Promise((resolve, reject) => {
    const createChannel = channelFactory || ((cb) => {
      new QWebChannel(qt.webChannelTransport, cb)  // eslint-disable-line no-undef
    })

    createChannel(function (channel) {
      const bridge = channel.objects.tiptapChannel
      if (!bridge) {
        reject(new Error('tiptapChannel not found on QWebChannel'))
        return
      }

      // 获取宿主资源根路径（适配层）
      const resourceBaseUrl = bridge.resourceBaseUrl || ''

      // --- 事件 1：加载就绪 ---
      // Editor 初始化完成 → 通知 C++
      bridge.jsEditorReady()

      // C++→JS：加载 envelope（loadEnvelopeRequested signal）
      bridge.loadEnvelopeRequested.connect(function (json) {
        let envelope
        try {
          envelope = JSON.parse(json)
        } catch {
          envelope = createEnvelope(createEmptyDoc())
        }
        const content = envelope?.content || createEmptyDoc()
        editor.commands.setContent(content)
      })

      // --- 事件 2：内容变化（节流后回告，不携带摘要） ---
      const debounce = createDebounce(() => {
        bridge.jsContentChanged()
      }, CONTENT_CHANGE_DEBOUNCE_MS)

      editor.on('update', () => {
        debounce.trigger()
      })

      // --- 事件 3：保存往返 ---
      // C++→JS：请求保存 → editor.getJSON() → envelope 包装 → 回告
      bridge.requestContent.connect(function () {
        const envelope = createEnvelope(editor.getJSON())
        bridge.jsContentSaved(serializeEnvelope(envelope))
      })

      // --- 事件 4：插入图片 ---
      bridge.insertImage.connect(function (imageInfoJson) {
        const result = parseImageInfo(imageInfoJson, resourceBaseUrl)
        if (result.ok) {
          const inserted = editor.chain().focus().insertContent({
            type: 'image',
            attrs: result.attrs,
          }).run()
          if (!inserted) {
            bridge.jsInsertImageFailed('editor rejected image node insertion')
          }
        } else {
          bridge.jsInsertImageFailed(result.reason)
        }
      })

      // --- 事件 5：插入语音块 ---
      bridge.insertVoiceBlock.connect(function (voiceInfoJson) {
        const result = parseVoiceInfo(voiceInfoJson, resourceBaseUrl)
        if (result.ok) {
          const inserted = editor.chain().focus().insertContent({
            type: 'voiceBlock',
            attrs: result.attrs,
          }).run()
          if (!inserted) {
            bridge.jsInsertVoiceBlockFailed('editor rejected voiceBlock node insertion')
          }
        } else {
          bridge.jsInsertVoiceBlockFailed(result.reason)
        }
      })

      resolve(bridge)
    })
  })
}
