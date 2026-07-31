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
// 新增资源节点类型只需注册对应处理函数即可扩展。
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

// ---------------------------------------------------------------------------
// Voice 运行态桥注册表
// ---------------------------------------------------------------------------

// voiceBridge 在 setVoiceBridge 时赋值，供 NodeView 获取并调用宿主入口。
let voiceBridge = null

// voiceSubscribers: voiceId -> Set<handlers>
const voiceSubscribers = new Map()

/**
 * 注入 voice 桥对象，连接 C++→JS 的 voice 信号并按 voiceId 分发。
 * 在 tiptap-editor.js 的 .then(bridge) 回调中调用。
 * @param {object} bridge — QWebChannel bridge 对象
 */
export function setVoiceBridge(bridge) {
  voiceBridge = bridge

  // 连接 C++→JS 的 voice 信号（防御性，信号在后续提交中添加）。
  // 每个信号携带 voiceId，用于按 voiceId 分发到对应 NodeView。
  const signals = [
    'voicePlaybackStateChanged',
    'voicePlaybackPositionChanged',
    'voicePlaybackDurationChanged',
    'voiceFileError',
    'voiceToTextStarted',
    'voiceToTextFailed',
    'voiceToTextCompleted',
    'themeProvided',
  ]
  for (const sig of signals) {
    if (bridge?.[sig]?.connect) {
      connectVoiceSignal(bridge, sig)
    }
  }
}

function connectVoiceSignal(bridge, signalName) {
  bridge[signalName].connect((...args) => {
    // voiceId 是每个信号的第一个参数（themeProvided 除外）
    if (signalName === 'themeProvided') {
      applyTheme(...args)
      return
    }
    const voiceId = args[0]
    const eventMap = {
      voicePlaybackStateChanged: 'onPlaybackStateChanged',
      voicePlaybackPositionChanged: 'onPositionChanged',
      voicePlaybackDurationChanged: 'onDurationChanged',
      voiceFileError: 'onFileError',
      voiceToTextStarted: 'onToTextStarted',
      voiceToTextFailed: 'onToTextFailed',
      voiceToTextCompleted: 'onToTextCompleted',
    }
    const handlerName = eventMap[signalName]
    dispatchVoice(voiceId, handlerName, ...args.slice(1))
  })
}

/**
 * 获取当前 voice 桥对象（供 voiceBlock NodeView 调用宿主入口）。
 * @returns {object|null}
 */
export function getVoiceBridge() {
  return voiceBridge
}

/**
 * 订阅指定 voiceId 的运行态事件。返回取消订阅函数。
 * @param {string} voiceId
 * @param {object} handlers — { onPlaybackStateChanged, onPositionChanged, onDurationChanged, onFileError, onToTextStarted, onToTextFailed, onToTextCompleted }
 * @returns {() => void}
 */
export function subscribeVoiceEvents(voiceId, handlers) {
  if (!voiceSubscribers.has(voiceId)) {
    voiceSubscribers.set(voiceId, new Set())
  }
  voiceSubscribers.get(voiceId).add(handlers)
  return () => {
    const set = voiceSubscribers.get(voiceId)
    if (set) {
      set.delete(handlers)
      if (set.size === 0) voiceSubscribers.delete(voiceId)
    }
  }
}

function dispatchVoice(voiceId, handlerName, ...args) {
  const subs = voiceSubscribers.get(voiceId)
  if (subs) {
    for (const handlers of subs) {
      handlers?.[handlerName]?.(...args)
    }
  }
}

function applyTheme(theme, highlightColor, disableHighlightColor, backgroundColor) {
  const root = document.documentElement
  if (highlightColor) root.style.setProperty('--highlightColor', highlightColor)
  if (disableHighlightColor) root.style.setProperty('--color', disableHighlightColor)
  if (backgroundColor) root.style.setProperty('--backgroundColor', backgroundColor)
}

// ---------------------------------------------------------------------------
// 防抖
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// voicePath 相对化（保存归一器，防御性）
// ---------------------------------------------------------------------------

/**
 * 将 voicePath 归一为相对路径（防御性）。若已是相对路径则原样返回；
 * 若为 file:// 绝对路径，尝试剥离 resourceBaseUrl 前缀，失败时尝试提取 voicenote/ 段。
 * @param {string|null|undefined} voicePath
 * @param {string} resourceBaseUrl
 * @returns {string|null}
 */
function makeVoicePathRelative(voicePath, resourceBaseUrl) {
  if (typeof voicePath !== 'string' || voicePath.length === 0) return voicePath
  // 已是相对路径（无 scheme）——直接保留
  if (!/^[a-z]+:\/\//i.test(voicePath)) return voicePath
  // 绝对 file:// URL —— 剥离 resourceBaseUrl 前缀
  if (resourceBaseUrl) {
    const base = resourceBaseUrl.replace(/\/+$/, '')
    if (voicePath.startsWith(base + '/')) {
      return voicePath.slice(base.length + 1)
    }
  }
  // 回退：voicePath 未匹配 resourceBaseUrl 前缀时，提取约定的语音存储目录段。
  // "voicenote/" 是录音文件的相对存储根（insertVoiceItem 下发 voicePath 时即用此约定），
  // 作为防御性归一的最终兜底；主路径已由 resourceBaseUrl 剥离覆盖。
  const match = voicePath.match(/(voicenote\/.+)$/)
  return match ? match[1] : voicePath
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

      // 获取宿主资源根路径（供前端拼接 images/ 等相对路径）
      const resourceBaseUrl = bridge.resourceBaseUrl || ''

      // 注册 image 保存归一：丢弃绝对显示 src，落库只存相对 relPath
      registerSaveNormalizer('image', (node) => ({
        src: node.attrs?.relPath || node.attrs?.src,
      }))

      // 注册 voiceBlock 保存归一：voicePath 绝对→相对（防御性）
      registerSaveNormalizer('voiceBlock', (node) => ({
        voicePath: makeVoicePathRelative(node.attrs?.voicePath, resourceBaseUrl),
      }))

      // 注册 image 加载解析：相对 relPath → 绝对 file:// 显示 URL
      registerLoadResolver('image', (node) => ({
        src: resolveResourceUrl(resourceBaseUrl, node.attrs?.relPath || node.attrs?.src),
      }))
      // voiceBlock 不注册写绝对路径的 load resolver —— 运行态由 NodeView 自行解析。

      // --- 事件 1：加载就绪 ---
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
            console.warn('[tiptap] save validation failed, sanitized invalid nodes:', result.errors)
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

// 递归删除校验失败的图片节点（含嵌套于列表项/引用/表格单元格者），保留其余内容。
function sanitizeDoc(doc) {
  const clone = cloneValue(doc)
  sanitizeNode(clone, true)
  return clone
}

const EMPTY_REJECTING_CONTAINERS = new Set([
  'blockquote',
  'bulletList',
  'orderedList',
  'listItem',
  'taskList',
  'taskItem',
])

function sanitizeNode(node, isDocRoot) {
  if (!node || typeof node !== 'object') return true
  if (Array.isArray(node.content)) {
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
