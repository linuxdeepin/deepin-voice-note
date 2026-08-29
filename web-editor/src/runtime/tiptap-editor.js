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
import { bindTiptapChannel, setVoiceBridge } from './tiptap-channel.js'
import { createFormatToolbar } from './format-toolbar.js'
import { setupImagePaste, setupImageViewAndMenu } from './image-interactions.js'
import { shouldFocusEditorOnDocumentMouseDown } from './focus-behavior.js'
import { setTiptapSearchQuery, clearTiptapSearch } from './search-extension.js'
import { currentTranscriptCopyText, copyTranscriptTextViaBridge, installTranscriptCopyHandler } from './transcript-copy.js'

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


function setupTransientScrollbar() {
  let scrollHideTimer = 0
  const showScrollbar = () => {
    document.body.classList.add('dvn-scrolling')
    if (scrollHideTimer) window.clearTimeout(scrollHideTimer)
    scrollHideTimer = window.setTimeout(() => {
      document.body.classList.remove('dvn-scrolling')
      scrollHideTimer = 0
    }, 1500)
  }
  document.body.addEventListener('scroll', showScrollbar, { passive: true })
  window.addEventListener('scroll', showScrollbar, { passive: true })
}

// ---------------------------------------------------------------------------
// 启动
// ---------------------------------------------------------------------------

setupTransientScrollbar()
installTranscriptCopyHandler()
const editor = createTiptapEditor(document.getElementById('app'))
const appElement = document.getElementById('app')
let tiptapBridge = null
function syncEditorEmptyState() {
  appElement.classList.toggle('is-empty', editor.isEmpty)
}
editor.on('create', syncEditorEmptyState)
editor.on('update', syncEditorEmptyState)
// setContent(..., { emitUpdate: false }) is used when loading a note.
// It still changes the ProseMirror transaction state, so sync here as well;
// otherwise the empty placeholder can remain visible over loaded/typed text.
editor.on('transaction', syncEditorEmptyState)
if (typeof window !== 'undefined') {
  window.__dvnTiptapEditor = editor
  window.__dvnTiptapContextTranscript = null
  window.__dvnTiptapGetContextTranscriptCopyText = function () {
    return currentTranscriptCopyText({ useContext: true, allowCachedContext: true, allowWholeContext: true, allowCachedRecent: true })
  }
  window.__dvnTiptapGetShortcutTranscriptCopyText = function () {
    return currentTranscriptCopyText({ useContext: false, allowCachedRecent: true })
  }
  window.__dvnTiptapCopyContextTranscript = function () {
    return copyTranscriptTextViaBridge(tiptapBridge)
  }
  window.__dvnTiptapSelectContextAll = function () {
    const transcript = window.__dvnTiptapContextTranscript
    if (transcript && document.contains(transcript)) {
      const range = document.createRange()
      range.selectNodeContents(transcript)
      const selection = window.getSelection()
      selection.removeAllRanges()
      selection.addRange(range)
      return true
    }
    return editor.chain().selectAll().run()
  }
  window.__dvnTiptapSelectVoiceBlockFromElement = function (element) {
    const voiceBox = element && element.closest ? element.closest('.voiceBox') : null
    if (!voiceBox || !document.contains(voiceBox)) return false
    const pos = editor.view.posAtDOM(voiceBox, 0)
    if (pos == null || pos < 0) return false
    return editor.chain().focus().setNodeSelection(pos).run()
  }
  window.__dvnTiptapSelectImageFromElement = function (element) {
    const img = element && element.closest ? element.closest('img[data-rel-path]') : null
    if (!img || !document.contains(img)) return false
    const pos = editor.view.posAtDOM(img, 0)
    if (pos == null || pos < 0) return false
    return editor.chain().focus().setNodeSelection(pos).run()
  }
  window.__dvnTiptapUndo = function () {
    return editor.chain().focus().undo().run()
  }
  window.__dvnTiptapRedo = function () {
    return editor.chain().focus().redo().run()
  }
  window.__dvnTiptapSetSearchQuery = function (query) {
    return setTiptapSearchQuery(editor, query)
  }
  window.__dvnTiptapClearSearch = function () {
    return clearTiptapSearch(editor)
  }
}
syncEditorEmptyState()
const toolbar = createFormatToolbar(editor, document.getElementById('toolbar-host'))
const titleInput = document.getElementById('note-title-input')
let pendingPickImage = false
let pendingRecordVoice = false
let currentTitleNoteId = -1
let savedTitle = ''
let titleSyncing = false

function normalizeNoteTitle(value) {
  return String(value ?? '').trim().slice(0, 24)
}

function isGeneratedNoteTitle(value) {
  return /^未命名文本\d+$/.test(String(value ?? ''))
}

function syncNoteTitleStyle(value) {
  titleInput.classList.toggle('is-custom', Boolean(value) && !isGeneratedNoteTitle(value))
}

function setNoteTitle(value) {
  titleSyncing = true
  titleInput.value = value
  syncNoteTitleStyle(value)
  titleSyncing = false
}

function loadNoteTitle(noteId, title) {
  currentTitleNoteId = Number(noteId)
  savedTitle = normalizeNoteTitle(title)
  setNoteTitle(savedTitle)
}

function commitNoteTitle() {
  if (titleSyncing || currentTitleNoteId <= 0) return
  const normalized = normalizeNoteTitle(titleInput.value)
  if (!normalized || normalized === savedTitle) {
    setNoteTitle(savedTitle)
    return
  }
  tiptapBridge?.renameCurrentNote?.(normalized)
  savedTitle = normalized
  setNoteTitle(savedTitle)
}

titleInput.addEventListener('input', () => {
  if (titleInput.value.length > 24) titleInput.value = titleInput.value.slice(0, 24)
  syncNoteTitleStyle(titleInput.value.trim())
})
titleInput.addEventListener('blur', commitNoteTitle)
titleInput.addEventListener('keydown', (event) => {
  if (event.key === 'Enter') {
    event.preventDefault()
    commitNoteTitle()
    titleInput.blur()
  }
})

// 工具栏可能先于 QWebChannel 完成绑定就已经可见。先缓存用户操作，
// 避免首次点击在 bridge 尚未就绪时被静默丢弃。
toolbar.setOnPickImage(() => {
  if (tiptapBridge?.jsRequestPickImage) {
    tiptapBridge.jsRequestPickImage()
  } else {
    pendingPickImage = true
  }
})
toolbar.setOnRecordVoice(() => {
  if (tiptapBridge?.jsRequestRecordVoice) {
    tiptapBridge.jsRequestRecordVoice()
  } else {
    pendingRecordVoice = true
  }
})

if (typeof window !== 'undefined') {
  window.__dvnTiptapSetResourceButtonsEnabled = function (voiceEnabled, imageEnabled) {
    toolbar.setResourceButtonsEnabled(Boolean(voiceEnabled), Boolean(imageEnabled))
  }
}

// 暴露聚焦接口：QML 侧把 WebEngineView 键盘焦点移入后调用，把光标放进 ProseMirror 编辑器
window._dvnTiptapFocus = function () {
  editor.commands.focus('end')
}

document.addEventListener('mousedown', function (event) {
  if (shouldFocusEditorOnDocumentMouseDown(event.target)) {
    editor.commands.focus('end')
  }
})

bindTiptapChannel(editor, undefined, {
  onFontList: (fonts, defaultFont) => toolbar.setFontList(fonts, defaultFont),
}).then((bridge) => {
  tiptapBridge = bridge
  loadNoteTitle(bridge.currentNoteId, bridge.currentNoteTitle)
  if (bridge.currentNoteChanged?.connect) {
    bridge.currentNoteChanged.connect((noteId, title) => loadNoteTitle(noteId, title))
  }
  // 注入 voice 桥，连接 voice 运行态信号分发
  setVoiceBridge(bridge)
  // bridge 就绪后补发绑定完成前缓存的首次点击。
  if (pendingPickImage) {
    pendingPickImage = false
    bridge.jsRequestPickImage?.()
  }
  if (pendingRecordVoice) {
    pendingRecordVoice = false
    bridge.jsRequestRecordVoice?.()
  }
  // 粘贴：剪贴板图片落盘往返、远程图片阻止
  setupImagePaste(editor, bridge)
  // 双击查看原图、右键菜单（查看原图 / 删除）
  setupImageViewAndMenu(editor, bridge)
})
