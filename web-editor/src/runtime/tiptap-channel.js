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
import { TextSelection } from '@tiptap/pm/state'
import { replaceEditorContentWithoutHistory } from './undo-redo.js'
import { setTiptapSearchQuery, clearTiptapSearch } from './search-extension.js'

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
const themeConnectedBridges = new WeakSet()

function connectThemeSignal(bridge) {
  if (!bridge || themeConnectedBridges.has(bridge)) return
  if (bridge.themeProvided?.connect) {
    bridge.themeProvided.connect(function (theme, highlightColor, disableHighlightColor, backgroundColor) {
      applyTheme(theme, highlightColor, disableHighlightColor, backgroundColor)
    })
    themeConnectedBridges.add(bridge)
  }
}

/**
 * 注入 voice 桥对象，连接 C++→JS 的 voice 信号并按 voiceId 分发。
 * 在 tiptap-editor.js 的 .then(bridge) 回调中调用。
 * @param {object} bridge — QWebChannel bridge 对象
 */
export function setVoiceBridge(bridge) {
  voiceBridge = bridge
  connectThemeSignal(bridge)

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
  ]
  for (const sig of signals) {
    if (bridge?.[sig]?.connect) {
      connectVoiceSignal(bridge, sig)
    }
  }
}

function connectVoiceSignal(bridge, signalName) {
  bridge[signalName].connect((...args) => {
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

function imageParagraph(schema, image) {
  // Tiptap/ProseMirror already renders a non-document trailingBreak for a
  // paragraph that ends with an inline leaf node.  Persisting an extra
  // hardBreak would make two visual line breaks (<img><br><br>) and creates an
  // empty row between consecutive images.  Keep the document model identical to
  // Summernote's semantic content: one inline image in one paragraph, caret just
  // after the image.
  return schema.nodes.paragraph.create(null, image)
}

function isTrailingHardBreak(node) {
  return node?.type?.name === 'hardBreak'
}

function setCursorAfterInlineImage(tr, paragraphStart, imageNodeSize = 1) {
  return tr.setSelection(TextSelection.create(tr.doc, paragraphStart + 1 + imageNodeSize)).scrollIntoView()
}

function cursorAfterImage(doc, imagePos) {
  const imageNode = doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return null
  const afterImage = imagePos + imageNode.nodeSize
  const $after = doc.resolve(afterImage)
  if ($after.parent.inlineContent) return afterImage
  const nextNode = $after.nodeAfter
  return nextNode?.isTextblock ? afterImage + 1 : null
}

function syncCursorAfterImage(editor, imagePos, { guardUserMove = true } = {}) {
  const { state } = editor
  const imageNode = state.doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return false

  const cursorPos = cursorAfterImage(state.doc, imagePos)
  if (cursorPos == null) return false

  const { selection } = state
  const isExpectedCursor = selection.empty && selection.from === cursorPos
  if (guardUserMove && !isExpectedCursor) return false

  const tr = state.tr.setSelection(TextSelection.create(state.doc, cursorPos)).scrollIntoView()
  editor.view.dispatch(tr)
  editor.view.focus()
  return true
}

function scheduleCursorAfterImageSync(editor, imagePos) {
  const sync = () => syncCursorAfterImage(editor, imagePos)
  if (typeof window !== 'undefined' && typeof window.requestAnimationFrame === 'function') {
    window.requestAnimationFrame(() => window.requestAnimationFrame(sync))
  } else {
    setTimeout(sync, 0)
  }
}

function bareImageParagraphSelection(selection) {
  if (!selection.empty) return null
  const $from = selection.$from
  if ($from.parent.type.name !== 'paragraph') return null
  if ($from.parent.childCount < 1 || $from.parent.childCount > 2) return null
  const firstChild = $from.parent.child(0)
  if (firstChild.type.name !== 'image') return null
  if ($from.parent.childCount === 2 && !isTrailingHardBreak($from.parent.child(1))) return null

  const paragraphStart = $from.before($from.depth)
  const paragraphEnd = $from.after($from.depth)
  if ($from.parentOffset <= 0) {
    return { side: 'before', paragraphStart, paragraphEnd, image: firstChild }
  }
  if ($from.parentOffset >= firstChild.nodeSize) {
    return { side: 'after', paragraphStart, paragraphEnd, image: firstChild }
  }
  return null
}

function selectedBareImageParagraph(selection) {
  if (selection.node?.type?.name !== 'image') return null
  const $from = selection.$from
  if ($from.parent.type.name !== 'paragraph') return null
  if ($from.parent.childCount < 1 || $from.parent.childCount > 2) return null
  if ($from.parent.child(0).type.name !== 'image') return null
  if ($from.parent.childCount === 2 && !isTrailingHardBreak($from.parent.child(1))) return null
  return {
    side: 'after',
    paragraphStart: $from.before($from.depth),
    paragraphEnd: $from.after($from.depth),
    image: selection.node,
  }
}

function isParagraphImageSeparator(node) {
  return node?.type === 'hardBreak'
}

function imageOnlyParagraphImages(node) {
  if (node?.type !== 'paragraph' || !Array.isArray(node.content)) return null
  const meaningful = node.content.filter((child) => !isParagraphImageSeparator(child))
  if (meaningful.length === 0) return null
  if (!meaningful.every((child) => child?.type === 'image')) return null
  return meaningful
}

export function normalizeImageParagraphLayout(doc) {
  if (!doc || doc.type !== 'doc' || !Array.isArray(doc.content)) return doc
  let changed = false
  const content = []

  for (const node of doc.content) {
    if (node?.type === 'image') {
      changed = true
      content.push({ type: 'paragraph', content: [node] })
      continue
    }

    const images = imageOnlyParagraphImages(node)
    if (images && (images.length > 1 || images.length !== node.content.length)) {
      changed = true
      for (const image of images) {
        content.push({ ...node, content: [image] })
      }
      continue
    }

    content.push(node)
  }

  return changed ? { ...doc, content } : doc
}

export const wrapTopLevelImagesInParagraphs = normalizeImageParagraphLayout

export function insertImageWithLegacyFlow(editor, attrs) {
  // Summernote stores pasted images as <p><img>...</p> and then selects the
  // range after the img.  Keep the caret in the same paragraph after the image,
  // but start a new paragraph when repeatedly inserting images so images do not
  // collapse onto the same visual line.
  try {
    const { state } = editor
    const { selection, schema } = state
    const image = schema.nodes.image.create(attrs)
    let tr = state.tr
    let insertedImagePos = null

    const imageParagraphSelection = bareImageParagraphSelection(selection) || selectedBareImageParagraph(selection)

    if (imageParagraphSelection) {
      const insertPos = imageParagraphSelection.side === 'before'
        ? imageParagraphSelection.paragraphStart
        : imageParagraphSelection.paragraphEnd
      const paragraph = imageParagraph(schema, image)
      insertedImagePos = insertPos + 1
      tr = tr.insert(insertPos, paragraph)
      tr = setCursorAfterInlineImage(tr, insertPos, image.nodeSize)
    } else if (selection.empty && selection.$from.parent.type.name === 'paragraph' && selection.$from.parent.content.size === 0) {
      const paragraphStart = selection.$from.before(selection.$from.depth)
      const paragraphEnd = selection.$from.after(selection.$from.depth)
      const paragraph = imageParagraph(schema, image)
      insertedImagePos = paragraphStart + 1
      tr = tr.replaceWith(paragraphStart, paragraphEnd, paragraph)
      tr = setCursorAfterInlineImage(tr, paragraphStart, image.nodeSize)
    } else {
      const insertFrom = selection.from
      insertedImagePos = insertFrom
      tr = tr.replaceSelectionWith(image, false)
      tr = tr.setSelection(TextSelection.create(tr.doc, insertFrom + image.nodeSize)).scrollIntoView()
    }

    editor.view.dispatch(tr)
    syncCursorAfterImage(editor, insertedImagePos, { guardUserMove: false })
    scheduleCursorAfterImageSync(editor, insertedImagePos)
    return true
  } catch (err) {
    console.error('[tiptap] insert image failed:', err)
    return false
  }
}

function colorWithAlpha(color, alpha, fallback) {
  if (typeof color !== 'string' || color.trim().length === 0) return fallback
  const value = color.trim()
  const clampedAlpha = Math.max(0, Math.min(1, alpha))

  const shortHex = value.match(/^#([0-9a-fA-F]{3})$/)
  if (shortHex) {
    const [r, g, b] = shortHex[1].split('').map((ch) => parseInt(ch + ch, 16))
    return `rgba(${r}, ${g}, ${b}, ${clampedAlpha})`
  }

  const longHex = value.match(/^#([0-9a-fA-F]{6})$/)
  if (longHex) {
    const hex = longHex[1]
    const r = parseInt(hex.slice(0, 2), 16)
    const g = parseInt(hex.slice(2, 4), 16)
    const b = parseInt(hex.slice(4, 6), 16)
    return `rgba(${r}, ${g}, ${b}, ${clampedAlpha})`
  }

  const rgb = value.match(/^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$/i)
  if (rgb) {
    return `rgba(${rgb[1]}, ${rgb[2]}, ${rgb[3]}, ${clampedAlpha})`
  }

  const rgba = value.match(/^rgba\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(?:0|1|0?\.\d+)\s*\)$/i)
  if (rgba) {
    return `rgba(${rgba[1]}, ${rgba[2]}, ${rgba[3]}, ${clampedAlpha})`
  }

  return fallback
}


function isEmptyParagraphNode(node) {
  return !!node && node.type?.name === 'paragraph' && node.content.size === 0
}

function findInsertedVoiceBlockPosition(editor, voiceId) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (node.type?.name === 'voiceBlock' && node.attrs?.voiceId === voiceId) {
      found = { pos, node }
      return false
    }
    return true
  })
  return found
}

function resolveVoiceInsertPosition(editor) {
  const { selection } = editor.state

  // Legacy Summernote inserts a newly recorded voice after the active voice box
  // instead of replacing it.  Tiptap uses a NodeSelection for selected atom
  // nodes, so move the insertion point into the following blank paragraph when
  // one already exists; inserting the legacy triplet there preserves exactly one
  // empty line between two voice blocks.
  if (selection.node?.type?.name === 'voiceBlock') {
    const $after = selection.$to
    const nextNode = $after.nodeAfter
    if (isEmptyParagraphNode(nextNode)) {
      return selection.to + 1
    }
    return selection.to
  }

  return null
}

function moveCursorAfterVoiceBlock(editor, voiceId) {
  const found = findInsertedVoiceBlockPosition(editor, voiceId)
  if (!found) return false

  const paragraphStart = found.pos + found.node.nodeSize
  const nextNode = editor.state.doc.resolve(paragraphStart).nodeAfter
  if (!isEmptyParagraphNode(nextNode)) return false

  return editor.chain()
    .focus(paragraphStart + 1)
    .setTextSelection(paragraphStart + 1)
    .run()
}

function insertVoiceBlockWithLegacySpacing(editor, attrs) {
  const content = [
    { type: 'paragraph' },
    { type: 'voiceBlock', attrs },
    { type: 'paragraph' },
  ]
  const insertPosition = resolveVoiceInsertPosition(editor)
  const chain = editor.chain().focus()
  const inserted = insertPosition == null
    ? chain.insertContent(content).run()
    : chain.insertContentAt(insertPosition, content).run()

  if (inserted) {
    moveCursorAfterVoiceBlock(editor, attrs.voiceId)
  }

  return inserted
}

function applyTheme(theme, highlightColor, disableHighlightColor, backgroundColor) {
  const root = document.documentElement
  if (highlightColor) root.style.setProperty('--highlightColor', highlightColor)
  if (disableHighlightColor) root.style.setProperty('--color', disableHighlightColor)
  if (backgroundColor) root.style.setProperty('--backgroundColor', backgroundColor)

  const activeBaseColor = highlightColor || '#0081ff'
  root.style.setProperty('--dvn-active-bg', colorWithAlpha(activeBaseColor, 0.5, 'rgba(0, 129, 255, 0.5)'))
  root.style.setProperty('--dvn-transcript-selection-bg', colorWithAlpha(activeBaseColor, 0.4, 'rgba(0, 129, 255, 0.4)'))
  root.style.setProperty('--dvn-active-selection-bg', colorWithAlpha(activeBaseColor, 0.6, 'rgba(0, 129, 255, 0.6)'))
  root.style.setProperty('--dvn-selection-fg', '#ffffff')

  // 主题联动：工具栏 / 语音块 / 滚动条 / 取色板 / 图片自绘菜单
  const isDark = theme === 'dark'
  root.dataset.dvnTheme = isDark ? 'dark' : 'light'
  root.style.setProperty('--dvn-editor-bg', isDark ? '#242424' : '#FBFCFD')
  root.style.setProperty('--dvn-panel-bg', isDark ? '#252525' : '#ffffff')
  root.style.setProperty('--dvn-toolbar-bg', isDark ? 'rgba(42, 42, 42, 1)' : 'rgba(245, 245, 245, 1)')
  root.style.setProperty('--dvn-toolbar-border-soft', isDark ? 'rgba(0, 0, 0, 0.30)' : 'rgba(0, 0, 0, 0.04)')
  root.style.setProperty('--dvn-panel-border', isDark ? '#444444' : '#cccccc')
  root.style.setProperty('--dvn-toolbar-border', isDark ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.08)')
  root.style.setProperty('--dvn-toolbar-fg', isDark ? 'rgba(192, 198, 212, 1)' : 'rgba(63, 63, 63, 1)')
  root.style.setProperty('--dvn-editor-fg', isDark ? 'rgba(192, 198, 212, 1)' : 'rgba(0, 0, 0, 1)')
  root.style.setProperty('--dvn-title-fg', isDark ? 'rgba(192, 198, 212, 1)' : 'rgba(0, 0, 0, 0.30)')
  root.style.setProperty('--dvn-placeholder-fg', isDark ? 'rgba(109, 124, 136, 1)' : 'rgba(0, 0, 0, 0.25)')
  root.style.setProperty('--dvn-toolbar-separator', isDark ? 'rgba(255, 255, 255, 0.08)' : 'rgba(0, 0, 0, 0.08)')
  root.style.setProperty('--dvn-scrollbar-thumb', isDark ? 'rgba(255, 255, 255, 0.20)' : 'rgba(0, 0, 0, 0.30)')
  root.style.setProperty('--dvn-scrollbar-thumb-hover', isDark ? 'rgba(255, 255, 255, 0.25)' : 'rgba(0, 0, 0, 0.50)')
  root.style.setProperty('--dvn-scrollbar-thumb-active', isDark ? 'rgba(255, 255, 255, 0.30)' : 'rgba(0, 0, 0, 0.40)')
  root.style.setProperty('--dvn-hover-bg', isDark ? '#3d3d3d' : '#f0f0f0')
  root.style.setProperty('--dvn-clear-btn-bg', isDark ? '#2d2d2d' : '#fafafa')
  root.style.setProperty('--dvn-color-chip-border', isDark ? 'rgba(255, 255, 255, 0.18)' : 'rgba(0, 0, 0, 0.20)')
  root.style.setProperty('--dvn-color-transparent-line', isDark ? 'rgba(255, 255, 255, 0.32)' : 'rgba(0, 0, 0, 0.25)')
  root.style.setProperty('--dvn-active-outline', highlightColor || '#0086cc')
  // Summernote 深色主题在 #242424 画布上使用白色 5% 的语音块背景。
  root.style.setProperty('--dvn-voice-bg', isDark ? 'rgba(255, 255, 255, 0.05)' : 'rgba(0, 0, 0, 0.05)')
  root.style.setProperty('--dvn-voice-title', isDark ? 'rgba(192, 198, 212, 1)' : 'rgba(0, 26, 46, 1)')
  root.style.setProperty('--dvn-voice-text', isDark ? 'rgba(192, 198, 212, 1)' : 'rgba(65, 77, 104, 1)')
  root.style.setProperty('--dvn-voice-subtitle', isDark ? 'rgba(109, 124, 136, 1)' : 'rgba(138, 161, 180, 1)')
  root.style.setProperty('--dvn-voice-divider', isDark ? 'rgba(255, 255, 255, 0.1)' : 'rgba(0, 0, 0, 0.08)')
  const themeEvent = typeof CustomEvent === 'function'
    ? new CustomEvent('dvn-theme-applied', { detail: { theme: root.dataset.dvnTheme } })
    : null
  if (themeEvent) window.dispatchEvent(themeEvent)

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
 * @param {object} [options] — 可选，{ onFontList, beforeInsertImage, clearResourceInsertionSelection }
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

      // C++→JS：加载 envelope（loadEnvelopeRequested signal）
      bridge.loadEnvelopeRequested.connect(function (json) {
        let envelope
        try {
          envelope = JSON.parse(json)
        } catch {
          envelope = createEnvelope(createEmptyDoc())
        }
        const content = normalizeImageParagraphLayout(envelope?.content || createEmptyDoc())
        const resolved = walkResolve(content, loadResolvers)
        options.clearResourceInsertionSelection?.()
        replaceEditorContentWithoutHistory(editor, resolved)
        if (bridge.currentSearchQuery) {
          setTiptapSearchQuery(editor, bridge.currentSearchQuery)
        }
      })

      // C++→JS：搜索 query 同步（Native 搜索框 → Tiptap 运行态高亮）
      if (bridge.searchQueryChanged?.connect) {
        bridge.searchQueryChanged.connect(function (query) {
          setTiptapSearchQuery(editor, query)
        })
      }
      if (bridge.searchCleared?.connect) {
        bridge.searchCleared.connect(function () {
          clearTiptapSearch(editor)
        })
      }
      if (bridge.currentSearchQuery) {
        setTiptapSearchQuery(editor, bridge.currentSearchQuery)
      }

      // C++→JS：宿主下发字体列表（fontListProvided signal）
      if (bridge.fontListProvided) {
        bridge.fontListProvided.connect(function (fonts, defaultFont) {
          options.onFontList?.(fonts, defaultFont)
        })
      }

      // C++→JS：主题同步必须在 jsEditorReady() 之前绑定。
      // 宿主会在 editorReady 回调里立即补发当前主题，若这里后绑，
      // 首次打开暗色主题会丢失信号并停留在 HTML 默认浅色。
      connectThemeSignal(bridge)

      // --- 事件 2：内容变化（节流后回告，不携带摘要） ---
      const debounce = createDebounce(() => {
        bridge.jsContentChanged()
      }, CONTENT_CHANGE_DEBOUNCE_MS)

      editor.on('update', () => {
        debounce.trigger()
      })

      // 滚动位置上报：工具栏固定在滚动容器外，只上报正文滚动容器位置，
      // 避免 window/body 滚动导致 sticky toolbar 重绘闪烁。
      const scrollTarget = document.getElementById('content-scroll') || document.scrollingElement || document.documentElement
      scrollTarget.addEventListener('scroll', () => {
        if (bridge.jsReportScroll) {
          bridge.jsReportScroll(scrollTarget.scrollTop || 0)
        }
      }, { passive: true })

      // --- 事件 3：保存往返 ---
      bridge.requestContent.connect(function () {
        const normalized = normalizeImageParagraphLayout(walkNormalize(editor.getJSON(), saveNormalizers))
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
          options.beforeInsertImage?.()
          const inserted = insertImageWithLegacyFlow(editor, result.attrs)
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
          const inserted = insertVoiceBlockWithLegacySpacing(editor, result.attrs)
          if (!inserted) {
            bridge.jsInsertVoiceBlockFailed('editor rejected voiceBlock node insertion')
          }
        } else {
          bridge.jsInsertVoiceBlockFailed(result.reason)
        }
      })

      // 所有 C++→JS 信号先 connect 完成，再通知 C++ editorReady。
      // 否则 C++ 在 editorReady 回调里立即下发字体列表/内容时，前端可能还没绑定 signal。
      bridge.jsEditorReady()

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
