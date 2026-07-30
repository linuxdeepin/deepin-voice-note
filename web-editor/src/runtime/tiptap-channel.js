// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 正式 QWebChannel 绑定模块（channel.objects.tiptapChannel）。
// 绑定五类基础事件，替换临时通道。
// 命名稳定契约：后续能力迁移只增不改、不重命名。
//
// 保存往返：editor.getJSON() 经 walkNormalize 归一（image 绝对显示 src → 相对 relPath）
// → createEnvelope → validateEnvelope → 通过则 jsContentSaved，失败记日志跳过保存。
// 归一后图片 src 为无 scheme 的相对路径，isSafeImageSrc 直接放行，无需放宽白名单。
// 加载往返：loadEnvelopeRequested 经 walkResolve 解析（image 相对 relPath → 绝对 file://）
// → setContent 供显示，不改持久内容。

import {
  createEmptyDoc,
  createEnvelope,
  serializeEnvelope,
  validateEnvelope,
  isSafeImageSrc,
  isSafeImageRelPath,
} from '../schema/document-envelope.js'
import { parseImageInfo, parseVoiceInfo, resolveResourceUrl } from './tiptap-adapter.js'

const CONTENT_CHANGE_DEBOUNCE_MS = 200

// 保存归一 / 加载解析注册表：按节点类型分发，递归遍历深拷贝，不 mutate 编辑器状态。
// 新增资源节点类型只需注册对应处理函数即可扩展，无需改动本框架主体。
const saveNormalizers = new Map()
const loadResolvers = new Map()

export function registerSaveNormalizer(nodeType, fn) {
  saveNormalizers.set(nodeType, fn)
}

export function registerLoadResolver(nodeType, fn) {
  loadResolvers.set(nodeType, fn)
}

function cloneValue(value) {
  return JSON.parse(JSON.stringify(value))
}

// 递归遍历文档树，深拷贝后对 table 中注册的节点类型应用处理函数。
// 处理函数返回需覆盖的 attrs 子集（合并进 clone.attrs），其余属性原样保留。
function walkApply(node, table) {
  if (!node || typeof node !== 'object') return node
  const clone = cloneValue(node)
  if (node.type && table.has(node.type)) {
    const override = table.get(node.type)(node)
    if (override && typeof override === 'object') {
      clone.attrs = { ...(clone.attrs || {}), ...override }
    }
  }
  if (Array.isArray(node.content)) {
    clone.content = node.content.map((child) => walkApply(child, table))
  }
  if (Array.isArray(node.marks)) {
    clone.marks = node.marks.map((mark) => walkApply(mark, table))
  }
  return clone
}

function walkNormalize(doc, table) {
  return walkApply(doc, table)
}

function walkResolve(doc, table) {
  return walkApply(doc, table)
}

// 递归删除校验失败的图片节点（含嵌套于列表项/引用/表格单元格者），保留其余内容，
// 避免单张非法图片导致整次保存静默丢失。判定条件同时满足 src 安全与 relPath 安全。
function sanitizeDoc(doc) {
  const clone = cloneValue(doc)
  sanitizeNode(clone, clone.type === 'doc')
  return clone
}

// schema 要求非空内容的容器节点类型：净化后若变空会被 nodeFromJSON().check() 拒绝，
// 需整体移除（由父节点 filter 掉）以避免空容器导致整次保存被跳过、非图片内容丢失。
const EMPTY_REJECTING_CONTAINERS = new Set([
  'blockquote',
  'bulletList',
  'orderedList',
  'listItem',
  'taskList',
  'taskItem',
])

function sanitizeNode(node, isDocRoot = false) {
  if (!node || typeof node !== 'object') return true
  if (Array.isArray(node.content)) {
    // 先剔除非法图片，再递归净化子节点；递归返回 false 表示该子节点是变空的容器需移除，
    // 父节点据此 filter 掉，自然处理嵌套级联（listItem 空 → bulletList 空 → 一并移除）。
    node.content = node.content
      .filter((child) => isSafeNode(child))
      .filter((child) => sanitizeNode(child, false))
    if (node.content.length === 0) {
      if (isDocRoot) {
        node.content = [{ type: 'paragraph' }]
      } else if (EMPTY_REJECTING_CONTAINERS.has(node.type)) {
        return false
      }
    }
  }
  return true
}

function isSafeNode(node) {
  if (node?.type !== 'image') return true
  const src = node.attrs?.src
  const relPath = node.attrs?.relPath
  return typeof src === 'string' && src.length > 0
    && isSafeImageSrc(src) && isSafeImageRelPath(relPath)
}

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
 * @param {object} [options] — 可选，{ onFontList(fonts, defaultFont) }
 * @returns {Promise<object>} resolve 为 bridge 对象
 */
export function bindTiptapChannel(editor, channelFactory, options = {}) {
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

      // 注册 image 保存归一：丢弃绝对显示 src，落库只存相对 relPath
      registerSaveNormalizer('image', (node) => ({
        src: node.attrs?.relPath || node.attrs?.src,
      }))

      // 注册 image 加载解析：相对 relPath → 绝对 file:// 显示 URL
      registerLoadResolver('image', (node) => ({
        src: resolveResourceUrl(resourceBaseUrl, node.attrs?.relPath || node.attrs?.src),
      }))

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
        const resolved = walkResolve(content, loadResolvers)
        editor.commands.setContent(resolved)
      })

      // C++→JS：宿主下发字体列表（fontListProvided signal）
      if (bridge.fontListProvided) {
        bridge.fontListProvided.connect(function (fonts, defaultFont) {
          options.onFontList?.(fonts, defaultFont)
        })
      }

      // --- 事件 2：内容变化（节流后回告，不携带摘要） ---
      const debounce = createDebounce(() => {
        bridge.jsContentChanged()
      }, CONTENT_CHANGE_DEBOUNCE_MS)

      editor.on('update', () => {
        debounce.trigger()
      })

      // --- 事件 3：保存往返 ---
      // C++→JS：请求保存 → editor.getJSON() → 归一 → envelope 包装 → 校验
      // 校验失败时净化非法图片节点（删除后重试），仍回告宿主避免静默丢失非图片内容
      bridge.requestContent.connect(function () {
        const normalized = walkNormalize(editor.getJSON(), saveNormalizers)
        const envelope = createEnvelope(normalized)
        const result = validateEnvelope(envelope)
        if (result.ok) {
          bridge.jsContentSaved(serializeEnvelope(envelope))
        } else {
          const sanitized = sanitizeDoc(normalized)
          const retryResult = validateEnvelope(createEnvelope(sanitized))
          if (retryResult.ok) {
            console.warn('[tiptap] save validation failed, sanitized invalid image nodes:', result.errors)
            bridge.jsContentSaved(serializeEnvelope(createEnvelope(sanitized)))
          } else {
            console.error('[tiptap] save validation failed even after sanitize, skip save:', retryResult.errors)
          }
        }
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
