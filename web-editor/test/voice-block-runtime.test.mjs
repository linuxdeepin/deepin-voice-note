// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Voice block runtime tests: signal dispatch, playback/transcription
// runtime state isolation, theme variable writing, paste voiceId.

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { NodeSelection } from '@tiptap/pm/state'
import { Fragment, Slice } from '@tiptap/pm/model'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { setTiptapSearchQuery } from '../src/runtime/search-extension.js'
import { currentTranscriptCopyText, writeTranscriptCopyEvent, copyTranscriptTextViaBridge, updateTranscriptSelectionCache, installTranscriptCopyHandler } from '../src/runtime/transcript-copy.js'
import { createEmptyDoc, createEnvelope, serializeEnvelope, validateEnvelope } from '../src/schema/document-envelope.js'
import {
  bindTiptapChannel,
  setVoiceBridge,
  getVoiceBridge,
  subscribeVoiceEvents,
  registerSaveNormalizer,
} from '../src/runtime/tiptap-channel.js'

function defineGlobal(name, value) {
  Object.defineProperty(globalThis, name, { value, writable: true, configurable: true })
}

function setupDom() {
  const window = new Window()
  defineGlobal('window', window)
  defineGlobal('document', window.document)
  defineGlobal('navigator', window.navigator)
  defineGlobal('Node', window.Node)
  defineGlobal('Element', window.Element)
  defineGlobal('HTMLElement', window.HTMLElement)
  defineGlobal('DocumentFragment', window.DocumentFragment)
  defineGlobal('Range', window.Range)
  defineGlobal('Event', window.Event)
  defineGlobal('CustomEvent', window.CustomEvent)
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('getComputedStyle', window.getComputedStyle.bind(window))
  defineGlobal('requestAnimationFrame', (cb) => setTimeout(cb, 0))
  defineGlobal('cancelAnimationFrame', (id) => clearTimeout(id))
  return window
}

function createEditor() {
  const window = setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
  return { editor, window }
}

function createMockBridge(resourceBaseUrl = 'file:///home/user/.local/share/deepin-voice-note') {
  const handlers = {}
  const calls = {}
  function makeConnectable(name) {
    return { connect(fn) { handlers[name] = fn } }
  }
  function emit(name, ...args) {
    if (handlers[name]) handlers[name](...args)
  }
  const bridge = {
    resourceBaseUrl,
    jsEditorReady() { calls.editorReady = (calls.editorReady || 0) + 1 },
    jsContentChanged() { calls.contentChanged = (calls.contentChanged || 0) + 1 },
    jsContentSaved(envelope) { (calls.contentSaved = calls.contentSaved || []).push(envelope) },
    jsInsertImageFailed() {},
    jsInsertVoiceBlockFailed() {},
    loadEnvelopeRequested: makeConnectable('loadEnvelopeRequested'),
    requestContent: makeConnectable('requestContent'),
    insertImage: makeConnectable('insertImage'),
    insertVoiceBlock: makeConnectable('insertVoiceBlock'),
    fontListProvided: makeConnectable('fontListProvided'),
    // voice signals
    voicePlaybackStateChanged: makeConnectable('voicePlaybackStateChanged'),
    voicePlaybackPositionChanged: makeConnectable('voicePlaybackPositionChanged'),
    voicePlaybackDurationChanged: makeConnectable('voicePlaybackDurationChanged'),
    voiceFileError: makeConnectable('voiceFileError'),
    voiceToTextStarted: makeConnectable('voiceToTextStarted'),
    voiceToTextFailed: makeConnectable('voiceToTextFailed'),
    voiceToTextCompleted: makeConnectable('voiceToTextCompleted'),
    themeProvided: makeConnectable('themeProvided'),
  }
  return { bridge, calls, emit }
}


test('selection: selected voiceBlock gets legacy active visual class', () => {
  const { editor } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: { voiceId: 'select-v1', voicePath: 'voicenote/select.wav', voiceSize: 1 },
    }],
  })

  let voicePos = -1
  editor.state.doc.descendants((node, pos) => {
    if (node.type.name === 'voiceBlock' && voicePos < 0) voicePos = pos
  })
  assert.ok(voicePos >= 0, 'voiceBlock should exist')

  editor.view.dispatch(editor.state.tr.setSelection(NodeSelection.create(editor.state.doc, voicePos)))

  const voiceBox = editor.view.dom.querySelector('.voiceBox')
  assert.ok(voiceBox.classList.contains('ProseMirror-selectednode'))
  assert.ok(voiceBox.classList.contains('active'))

  editor.destroy()
})

test('transcript text: pointer selection does not start whole voiceBlock drag', () => {
  const { editor, window } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'copy-v1',
        voicePath: 'voicenote/copy.wav',
        voiceSize: 1,
        text: '可单独复制的转写文本',
        translateUnfold: true,
      },
    }],
  })

  const voiceBox = editor.view.dom.querySelector('.voiceBox')
  const translateText = editor.view.dom.querySelector('.translateText')
  assert.ok(voiceBox, 'voiceBox should render')
  assert.ok(translateText, 'translateText should render')
  assert.equal(translateText.getAttribute('draggable'), 'false')
  assert.equal(translateText.getAttribute('contenteditable'), 'true')
  assert.equal(translateText.getAttribute('aria-readonly'), 'true')

  translateText.dispatchEvent(new window.MouseEvent('mousedown', { bubbles: true, cancelable: true }))
  assert.equal(voiceBox.getAttribute('draggable'), 'false', 'wrapper drag is disabled while selecting transcript text')

  document.dispatchEvent(new window.MouseEvent('mouseup', { bubbles: true, cancelable: true }))
  assert.equal(voiceBox.getAttribute('draggable'), null, 'wrapper drag state is restored after selection gesture')

  const dragStart = new window.Event('dragstart', { bubbles: true, cancelable: true })
  translateText.dispatchEvent(dragStart)
  assert.equal(dragStart.defaultPrevented, true, 'transcript dragstart should not drag the whole voiceBlock')

  editor.destroy()
})


test('transcript text: editable island is guarded as read-only but allows copy shortcut', () => {
  const { editor, window } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'readonly-v1',
        voicePath: 'voicenote/readonly.wav',
        voiceSize: 1,
        text: '只读但可复制',
        translateUnfold: true,
      },
    }],
  })

  const voiceBox = editor.view.dom.querySelector('.voiceBox')
  const translateText = editor.view.dom.querySelector('.translateText')
  assert.equal(voiceBox.getAttribute('contenteditable'), 'true', 'voiceBlock root must not be a non-editable ancestor of transcript selection')
  assert.equal(translateText.getAttribute('contenteditable'), 'true')
  assert.equal(translateText.getAttribute('aria-readonly'), 'true')

  const wrapperBeforeInput = new window.InputEvent('beforeinput', { bubbles: true, cancelable: true, inputType: 'insertText', data: 'x' })
  voiceBox.dispatchEvent(wrapperBeforeInput)
  assert.equal(wrapperBeforeInput.defaultPrevented, true, 'voiceBlock editable wrapper is guarded as read-only')

  const beforeInput = new window.InputEvent('beforeinput', { bubbles: true, cancelable: true, inputType: 'insertText', data: 'x' })
  translateText.dispatchEvent(beforeInput)
  assert.equal(beforeInput.defaultPrevented, true, 'typing into transcript must be blocked')

  const normalKey = new window.KeyboardEvent('keydown', { key: 'x', bubbles: true, cancelable: true })
  translateText.dispatchEvent(normalKey)
  assert.equal(normalKey.defaultPrevented, true, 'printable key must be blocked')

  const copyKey = new window.KeyboardEvent('keydown', { key: 'c', ctrlKey: true, bubbles: true, cancelable: true })
  translateText.dispatchEvent(copyKey)
  assert.equal(copyKey.defaultPrevented, false, 'copy shortcut must remain native')

  const selectAll = new window.KeyboardEvent('keydown', { key: 'a', ctrlKey: true, bubbles: true, cancelable: true })
  translateText.dispatchEvent(selectAll)
  assert.equal(selectAll.defaultPrevented, true, 'Ctrl+A is handled inside transcript')
  assert.equal(window.getSelection().toString(), '只读但可复制')

  const paste = new window.Event('paste', { bubbles: true, cancelable: true })
  translateText.dispatchEvent(paste)
  assert.equal(paste.defaultPrevented, true, 'paste into transcript must be blocked')

  editor.destroy()
})


test('transcript copy: context helper returns DOM selection inside voiceBlock atom', () => {
  const { editor } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'copy-dom-v1',
        voicePath: 'voicenote/copy-dom.wav',
        voiceSize: 1,
        text: '可单独复制的转写文本',
        translateUnfold: true,
      },
    }],
  })

  const translateText = editor.view.dom.querySelector('.translateText')
  assert.ok(translateText, 'translateText should render')

  const textNode = translateText.firstChild
  const range = document.createRange()
  range.setStart(textNode, 1)
  range.setEnd(textNode, 6)
  const selection = window.getSelection()
  selection.removeAllRanges()
  selection.addRange(range)
  window.__dvnTiptapContextTranscript = translateText

  assert.equal(currentTranscriptCopyText(), '单独复制的')

  const copied = {}
  const event = new Event('copy', { bubbles: true, cancelable: true })
  Object.defineProperty(event, 'clipboardData', {
    value: { setData(type, value) { copied[type] = value } },
    configurable: true,
  })
  assert.equal(writeTranscriptCopyEvent(event), true)
  assert.equal(event.defaultPrevented, true)
  assert.equal(copied['text/plain'], '单独复制的')
  assert.equal(copied['text/html'], '单独复制的')

  editor.destroy()
  window.__dvnTiptapContextTranscript = null
})

test('transcript copy: search-highlighted transcript copies plain text only', () => {
  const { editor } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'copy-search-v1',
        voicePath: 'voicenote/copy-search.wav',
        voiceSize: 1,
        text: '复制高亮后的转写文本',
        translateUnfold: false,
      },
    }],
  })

  assert.equal(setTiptapSearchQuery(editor, '高亮'), true)
  const translateText = editor.view.dom.querySelector('.translateText')
  assert.ok(translateText, 'translateText should render')
  assert.ok(translateText.querySelector('.dvn-search-match'), 'search hit should be rendered inside transcript')

  const range = document.createRange()
  range.selectNodeContents(translateText)
  const selection = window.getSelection()
  selection.removeAllRanges()
  selection.addRange(range)
  window.__dvnTiptapContextTranscript = translateText

  assert.equal(currentTranscriptCopyText(), '复制高亮后的转写文本')

  const bridgeCalls = []
  assert.equal(copyTranscriptTextViaBridge({ jsCopyPlainTextToClipboard: (text) => bridgeCalls.push(text) }), true)
  assert.deepEqual(bridgeCalls, ['复制高亮后的转写文本'])

  editor.destroy()
  window.__dvnTiptapContextTranscript = null
})


test('transcript copy: context menu can use cached text after Qt collapses selection', () => {
  const { editor } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'copy-cache-v1',
        voicePath: 'voicenote/copy-cache.wav',
        voiceSize: 1,
        text: '右键菜单打开前已经选中的转写文本',
        translateUnfold: true,
      },
    }],
  })

  const translateText = editor.view.dom.querySelector('.translateText')
  const textNode = translateText.firstChild
  const selectedRange = document.createRange()
  selectedRange.setStart(textNode, 7)
  selectedRange.setEnd(textNode, 12)
  const selection = window.getSelection()
  selection.removeAllRanges()
  selection.addRange(selectedRange)
  assert.equal(updateTranscriptSelectionCache(), true)

  // Simulate QtWebEngine/native menu changing the live DOM selection before
  // the QML action callback executes.  The explicit context transcript should
  // still recover the last valid selected text for that same transcript only.
  const collapsed = document.createRange()
  collapsed.setStart(textNode, 0)
  collapsed.setEnd(textNode, 0)
  selection.removeAllRanges()
  selection.addRange(collapsed)
  window.__dvnTiptapContextTranscript = translateText

  assert.equal(currentTranscriptCopyText({ useContext: true, allowCachedContext: true }), '已经选中的')
  assert.equal(currentTranscriptCopyText(), '')

  editor.destroy()
  window.__dvnTiptapContextTranscript = null
})


test('transcript copy: context menu falls back to whole transcript without selection', () => {
  const { editor } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'copy-whole-v1',
        voicePath: 'voicenote/copy-whole.wav',
        voiceSize: 1,
        text: '没有选区时复制整段转写',
        translateUnfold: true,
      },
    }],
  })

  const translateText = editor.view.dom.querySelector('.translateText')
  const selection = window.getSelection()
  selection.removeAllRanges()
  window.__dvnTiptapContextTranscript = translateText

  assert.equal(currentTranscriptCopyText({ useContext: true, allowWholeContext: true }), '没有选区时复制整段转写')
  assert.equal(currentTranscriptCopyText(), '')

  const bridgeCalls = []
  assert.equal(copyTranscriptTextViaBridge({ jsCopyPlainTextToClipboard: (text) => bridgeCalls.push(text) }), true)
  assert.deepEqual(bridgeCalls, ['没有选区时复制整段转写'])

  editor.destroy()
  window.__dvnTiptapContextTranscript = null
})


test('transcript copy: contextmenu listener records transcript target before QML probe', () => {
  const { editor, window } = createEditor()
  const uninstall = installTranscriptCopyHandler()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'copy-context-event-v1',
        voicePath: 'voicenote/copy-context-event.wav',
        voiceSize: 1,
        text: '右键事件缓存整段转写',
        translateUnfold: true,
      },
    }],
  })

  const translateText = editor.view.dom.querySelector('.translateText')
  translateText.dispatchEvent(new window.MouseEvent('contextmenu', { bubbles: true, cancelable: true }))

  assert.equal(window.__dvnTiptapContextTranscript, translateText)
  assert.equal(currentTranscriptCopyText({ useContext: true, allowWholeContext: true }), '右键事件缓存整段转写')

  uninstall()
  editor.destroy()
  window.__dvnTiptapContextTranscript = null
})


test('transcript copy: shortcut can use recent cached selection without context', () => {
  const { editor } = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'copy-shortcut-cache-v1',
        voicePath: 'voicenote/copy-shortcut-cache.wav',
        voiceSize: 1,
        text: '快捷键复制缓存转写',
        translateUnfold: true,
      },
    }],
  })

  const translateText = editor.view.dom.querySelector('.translateText')
  const textNode = translateText.firstChild
  const range = document.createRange()
  range.setStart(textNode, 3)
  range.setEnd(textNode, 7)
  const selection = window.getSelection()
  selection.removeAllRanges()
  selection.addRange(range)
  assert.equal(updateTranscriptSelectionCache(), true)

  // Simulate the global QML Shortcut path where no context-menu element was
  // recorded and the live DOM selection is no longer usable by callback time.
  selection.removeAllRanges()
  window.__dvnTiptapContextTranscript = null

  assert.equal(currentTranscriptCopyText({ allowCachedRecent: true }), '复制缓存')
  assert.equal(currentTranscriptCopyText(), '')

  editor.destroy()
})

// ---------------------------------------------------------------------------
// 信号分发：subscribeVoiceEvents → dispatchVoice
// ---------------------------------------------------------------------------

test('subscribeVoiceEvents: receives dispatched playback state change', () => {
  setupDom()
  const received = []
  const unsub = subscribeVoiceEvents('voice-1', {
    onPlaybackStateChanged(state) { received.push(state) },
  })

  // Dispatch directly via bridge signal → setVoiceBridge connection → dispatchVoice
  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('voicePlaybackStateChanged', 'voice-1', 0)
  emit('voicePlaybackStateChanged', 'voice-1', 2)
  assert.deepEqual(received, [0, 2])
  unsub()
})

test('subscribeVoiceEvents: receives dispatched position and duration', () => {
  setupDom()
  const positions = []
  const durations = []
  const unsub = subscribeVoiceEvents('voice-2', {
    onPositionChanged(ms) { positions.push(ms) },
    onDurationChanged(ms) { durations.push(ms) },
  })

  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('voicePlaybackPositionChanged', 'voice-2', 3000)
  emit('voicePlaybackDurationChanged', 'voice-2', 12000)
  assert.deepEqual(positions, [3000])
  assert.deepEqual(durations, [12000])
  unsub()
})

test('subscribeVoiceEvents: receives dispatched to-text events', () => {
  setupDom()
  const events = []
  const unsub = subscribeVoiceEvents('voice-3', {
    onToTextStarted() { events.push('started') },
    onToTextFailed() { events.push('failed') },
    onToTextCompleted(text) { events.push('completed:' + text) },
  })

  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('voiceToTextStarted', 'voice-3')
  emit('voiceToTextCompleted', 'voice-3', '转写文本')
  emit('voiceToTextFailed', 'voice-3')
  assert.deepEqual(events, ['started', 'completed:转写文本', 'failed'])
  unsub()
})

test('subscribeVoiceEvents: file error dispatched', () => {
  setupDom()
  let fileError = false
  const unsub = subscribeVoiceEvents('voice-4', {
    onFileError() { fileError = true },
  })

  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('voiceFileError', 'voice-4')
  assert.ok(fileError)
  unsub()
})

test('subscribeVoiceEvents: unsubscribe stops receiving', () => {
  setupDom()
  const received = []
  const unsub = subscribeVoiceEvents('voice-5', {
    onPlaybackStateChanged(state) { received.push(state) },
  })

  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('voicePlaybackStateChanged', 'voice-5', 0)
  unsub()
  emit('voicePlaybackStateChanged', 'voice-5', 2)
  assert.deepEqual(received, [0])
})

// ---------------------------------------------------------------------------
// 主题变量写入：themeProvided → CSS variables
// ---------------------------------------------------------------------------

test('themeProvided writes CSS variables on document root', () => {
  setupDom()
  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('themeProvided', 'dark', '#007AFF', '#999999', '#090A17')
  assert.equal(document.documentElement.style.getPropertyValue('--highlightColor'), '#007AFF')
  assert.equal(document.documentElement.style.getPropertyValue('--color'), '#999999')
  assert.equal(document.documentElement.style.getPropertyValue('--backgroundColor'), '#090A17')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-active-bg'), 'rgba(0, 122, 255, 0.5)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-transcript-selection-bg'), 'rgba(0, 122, 255, 0.4)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-active-selection-bg'), 'rgba(0, 122, 255, 0.6)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-selection-fg'), '#ffffff')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-editor-fg'), 'rgba(192, 198, 212, 1)')
  assert.equal(document.documentElement.dataset.dvnTheme, 'dark')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-editor-bg'), '#242424')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-color-chip-border'), 'rgba(255, 255, 255, 0.18)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-color-transparent-line'), 'rgba(255, 255, 255, 0.32)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-voice-bg'), 'rgba(255, 255, 255, 0.05)')
})

test('themeProvided: light theme variables', () => {
  setupDom()
  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('themeProvided', 'light', '#0058DE', '#cccccc', '#FBFCFD')
  assert.equal(document.documentElement.style.getPropertyValue('--highlightColor'), '#0058DE')
  assert.equal(document.documentElement.style.getPropertyValue('--backgroundColor'), '#FBFCFD')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-active-bg'), 'rgba(0, 88, 222, 0.5)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-editor-fg'), 'rgba(0, 0, 0, 1)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-transcript-selection-bg'), 'rgba(0, 88, 222, 0.4)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-selection-fg'), '#ffffff')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-color-chip-border'), 'rgba(0, 0, 0, 0.20)')
  assert.equal(document.documentElement.style.getPropertyValue('--dvn-color-transparent-line'), 'rgba(0, 0, 0, 0.25)')
})

// ---------------------------------------------------------------------------
// 运行态隔离：保存往返不含运行态
// ---------------------------------------------------------------------------

test('save roundtrip: voiceBlock persists relative voicePath, no runtime state', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = (cb) => cb({ objects: { tiptapChannel: bridge } })

  await bindTiptapChannel(editor, factory)

  // 插入一个 voiceBlock（相对路径）
  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'v-save-test',
      voicePath: 'voicenote/20260717-100000.mp3',
      voiceSize: 120000,
      createTime: '2026-07-17 10:00:00',
      title: '录音',
      text: null,
      translateUnfold: true,
    },
  })

  emit('requestContent')

  assert.ok(calls.contentSaved)
  const saved = JSON.parse(calls.contentSaved[0])
  const voiceNode = saved.content.content.find(n => n.type === 'voiceBlock')
  assert.ok(voiceNode)
  assert.equal(voiceNode.attrs.voicePath, 'voicenote/20260717-100000.mp3')
  assert.ok(!Object.hasOwn(voiceNode.attrs, 'playing'))
  assert.ok(!Object.hasOwn(voiceNode.attrs, 'paused'))
  assert.ok(!Object.hasOwn(voiceNode.attrs, 'progress'))
  assert.ok(!Object.hasOwn(voiceNode.attrs, 'translating'))
  editor.destroy()
})

test('save normalizer: strips absolute voicePath to relative', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = (cb) => cb({ objects: { tiptapChannel: bridge } })

  await bindTiptapChannel(editor, factory)

  // 插入一个 voiceBlock（模拟误混入绝对路径，防御性归一）
  const base = bridge.resourceBaseUrl.replace(/\/+$/, '')
  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'v-abs-test',
      voicePath: base + '/voicenote/20260717-100000.mp3',
      voiceSize: 5000,
      createTime: '2026-07-17 10:00:00',
      title: '测试',
      text: null,
      translateUnfold: true,
    },
  })

  emit('requestContent')

  assert.ok(calls.contentSaved)
  const saved = JSON.parse(calls.contentSaved[0])
  const voiceNode = saved.content.content.find(n => n.type === 'voiceBlock')
  assert.ok(voiceNode)
  assert.equal(voiceNode.attrs.voicePath, 'voicenote/20260717-100000.mp3')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 粘贴生成新 voiceId
// ---------------------------------------------------------------------------

test('paste: duplicate voiceBlock gets new voiceId', () => {
  const { editor } = createEditor()

  // 插入原始 voiceBlock
  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'original-id',
      voicePath: 'voicenote/test.mp3',
      voiceSize: 3000,
      createTime: '2026-07-17 10:00:00',
      title: '原始',
      text: null,
      translateUnfold: true,
    },
  })

  // 模拟粘贴：用相同 voiceId 插入另一个 voiceBlock（模拟复制粘贴）
  // 使用 transaction 并标记 paste meta
  const tr = editor.state.tr
  tr.insert(editor.state.doc.content.size, editor.state.schema.nodes.voiceBlock.create({
    voiceId: 'original-id',
    voicePath: 'voicenote/test.mp3',
    voiceSize: 3000,
    createTime: '2026-07-17 10:00:00',
    title: '副本',
    text: null,
    translateUnfold: true,
  }))
  tr.setMeta('paste', true)
  editor.view.dispatch(tr)

  // paste 插件应该把第二个（重复的）voiceBlock 的 voiceId 换成新的
  const voiceNodes = []
  editor.state.doc.descendants(node => {
    if (node.type.name === 'voiceBlock') voiceNodes.push(node)
  })
  assert.equal(voiceNodes.length, 2)
  assert.equal(voiceNodes[0].attrs.voiceId, 'original-id')
  assert.notEqual(voiceNodes[1].attrs.voiceId, 'original-id')
  assert.ok(voiceNodes[1].attrs.voiceId.length > 0)
  editor.destroy()
})


function createVoicePasteSlice(editor, attrs = {}) {
  const voiceNode = editor.state.schema.nodes.voiceBlock.create({
    voiceId: 'copied-id',
    voicePath: 'voicenote/copied.mp3',
    voiceSize: 3000,
    createTime: '2026-07-17 10:00:00',
    title: '复制语音',
    text: '复制时应清除的转写文本',
    translateUnfold: false,
    ...attrs,
  })
  return new Slice(Fragment.from(voiceNode), 0, 0)
}

function pasteVoiceSlice(editor, slice) {
  const handlePaste = editor.view.someProp('handlePaste', f => f)
  assert.ok(typeof handlePaste === 'function', 'handlePaste should be registered')
  let prevented = false
  const handled = handlePaste(editor.view, { preventDefault() { prevented = true } }, slice)
  assert.equal(handled, true)
  assert.equal(prevented, true)
}

test('paste: copied voiceBlock keeps legacy spacing and moves cursor after pasted block', () => {
  const { editor } = createEditor()

  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'voiceBlock', attrs: { voiceId: 'original-id', voicePath: 'voicenote/original.mp3', voiceSize: 1000 } },
      { type: 'paragraph' },
    ],
  })
  editor.commands.setTextSelection(2)

  pasteVoiceSlice(editor, createVoicePasteSlice(editor))

  const json = editor.getJSON()
  assert.deepEqual(json.content.map(n => n.type), ['voiceBlock', 'paragraph', 'voiceBlock', 'paragraph'])
  assert.equal(json.content[0].attrs.voiceId, 'original-id')
  assert.notEqual(json.content[2].attrs.voiceId, 'copied-id')
  assert.equal(json.content[2].attrs.text, null)
  assert.equal(json.content[2].attrs.translateUnfold, true)
  assert.equal(editor.state.selection.from, 5)
  assert.equal(editor.state.selection.to, 5)
  editor.destroy()
})

test('paste: copied voiceBlock appends after selected voiceBlock instead of replacing it', () => {
  const { editor } = createEditor()

  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'voiceBlock', attrs: { voiceId: 'original-id', voicePath: 'voicenote/original.mp3', voiceSize: 1000 } },
      { type: 'paragraph' },
    ],
  })
  editor.view.dispatch(editor.state.tr.setSelection(NodeSelection.create(editor.state.doc, 0)))

  pasteVoiceSlice(editor, createVoicePasteSlice(editor))

  const json = editor.getJSON()
  assert.deepEqual(json.content.map(n => n.type), ['voiceBlock', 'paragraph', 'voiceBlock', 'paragraph'])
  assert.equal(json.content[0].attrs.voiceId, 'original-id')
  assert.notEqual(json.content[2].attrs.voiceId, 'copied-id')
  assert.equal(editor.state.selection.from, 5)
  assert.equal(editor.state.selection.to, 5)
  editor.destroy()
})


test('paste: custom clipboard voice json inserts voiceBlock with legacy spacing', () => {
  const { editor } = createEditor()
  const handlePaste = editor.view.someProp('handlePaste', f => f)
  assert.ok(typeof handlePaste === 'function', 'handlePaste should be registered')

  const clipboardJson = JSON.stringify({
    type: 2,
    voiceId: 'clipboard-id',
    voicePath: 'voicenote/clipboard.mp3',
    voiceSize: 4321,
    title: '剪贴板语音',
    text: '复制时带来的转写应在粘贴时清除',
    translateUnfold: false,
  })
  let prevented = false
  const handled = handlePaste(editor.view, {
    preventDefault() { prevented = true },
    clipboardData: {
      getData(type) {
        return type === 'application/x-deepin-voice-note-voice-block' ? clipboardJson : ''
      },
    },
  }, new Slice(Fragment.empty, 0, 0))

  assert.equal(handled, true)
  assert.equal(prevented, true)
  const json = editor.getJSON()
  assert.deepEqual(json.content.map(n => n.type), ['paragraph', 'voiceBlock', 'paragraph'])
  assert.notEqual(json.content[1].attrs.voiceId, 'clipboard-id')
  assert.equal(json.content[1].attrs.voicePath, 'voicenote/clipboard.mp3')
  assert.equal(json.content[1].attrs.voiceSize, 4321)
  assert.equal(json.content[1].attrs.text, null)
  assert.equal(json.content[1].attrs.translateUnfold, true)
  assert.equal(editor.state.selection.from, 4)
  editor.destroy()
})

// ---------------------------------------------------------------------------
// Schema: playing/paused/progress 不在持久化属性中
// ---------------------------------------------------------------------------

test('schema: voiceBlock with playing/paused/progress attrs are not persisted', () => {
  // 验证 addAttributes 不含运行态属性
  const { editor } = createEditor()
  const voiceType = editor.schema.nodes.voiceBlock
  const attrNames = Object.keys(voiceType.spec.attrs)
  assert.ok(!attrNames.includes('playing'))
  assert.ok(!attrNames.includes('paused'))
  assert.ok(!attrNames.includes('progress'))
  assert.ok(!attrNames.includes('translating'))
  assert.ok(!attrNames.includes('seeking'))
  // 确认 7 个持久化属性存在
  for (const expected of ['voiceId', 'voicePath', 'voiceSize', 'createTime', 'title', 'text', 'translateUnfold']) {
    assert.ok(attrNames.includes(expected), `missing persistent attr: ${expected}`)
  }
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 文件缺失标记（D7）：voiceFileError → .unplayable
// ---------------------------------------------------------------------------

test('file error: NodeView marks unplayable via bridge signal', () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'voice-file-err',
      voicePath: 'voicenote/missing.mp3',
      voiceSize: 5000,
      createTime: '2026-07-17 10:00:00',
      title: '缺失文件',
      text: '已有转写',
      translateUnfold: true,
    },
  })

  // 发送文件错误信号
  emit('voiceFileError', 'voice-file-err')

  // 验证 NodeView DOM 上的 .unplayable 类
  const voiceDom = editor.view.dom.querySelector('.voiceInfoBox')
  assert.ok(voiceDom, 'voiceInfoBox should exist in DOM')
  const playback = voiceDom.querySelector('.voicePlayback')
  assert.ok(playback, 'voicePlayback should exist')
  assert.ok(playback.classList.contains('unplayable'), 'should have .unplayable class')
  // 文件缺失后保留 text（D7）
  const translateText = voiceDom.querySelector('.translateText')
  assert.ok(translateText, 'translateText should still exist')
  assert.equal(translateText.textContent, '已有转写')
  editor.destroy()
})

test('file error: unplayable persists across save roundtrip', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  setVoiceBridge(bridge)
  const factory = (cb) => cb({ objects: { tiptapChannel: bridge } })
  await bindTiptapChannel(editor, factory)

  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'voice-persist-test',
      voicePath: 'voicenote/test.mp3',
      voiceSize: 5000,
      createTime: '2026-07-17 10:00:00',
      title: '测试',
      text: '转写结果',
      translateUnfold: true,
    },
  })

  // 标记文件错误
  emit('voiceFileError', 'voice-persist-test')

  // 保存往返：unplayable 是运行态，不应进入持久化
  emit('requestContent')
  assert.ok(calls.contentSaved)
  const saved = JSON.parse(calls.contentSaved[0])
  const voiceNode = saved.content.content.find(n => n.type === 'voiceBlock')
  assert.ok(voiceNode)
  assert.ok(!Object.hasOwn(voiceNode.attrs, 'unplayable'))
  assert.ok(!Object.hasOwn(voiceNode.attrs, 'playing'))
  assert.equal(voiceNode.attrs.text, '转写结果')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// Schema: 拒绝 translating 运行态持久化
// ---------------------------------------------------------------------------

test('schema: voiceBlock with translating attr is rejected by validateEnvelope', () => {
  // already imported at top
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'v-translating-test',
        voicePath: 'voicenote/test.mp3',
        voiceSize: 5000,
        createTime: '2026-07-17 10:00:00',
        title: '测试',
        text: null,
        translateUnfold: true,
        translating: true,
      },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, false)
  assert.ok(result.errors.some(e => e.code === 'runtime-state-persisted'))
})

// ---------------------------------------------------------------------------
// 转写完成写回 attrs.text
// ---------------------------------------------------------------------------

test('to-text completed: writes text to voiceBlock attrs', () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'voice-writeback-test',
      voicePath: 'voicenote/writeback.mp3',
      voiceSize: 5000,
      createTime: '2026-07-17 10:00:00',
      title: '写回测试',
      text: null,
      translateUnfold: true,
    },
  })

  // 模拟转写开始
  emit('voiceToTextStarted', 'voice-writeback-test')
  // 模拟转写完成
  emit('voiceToTextCompleted', 'voice-writeback-test', '写回的转写文本')

  // 验证 attrs.text 已写回
  const voiceNode = editor.state.doc.content.content.find(n => n.type.name === 'voiceBlock')
  assert.ok(voiceNode)
  assert.equal(voiceNode.attrs.text, '写回的转写文本')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 复制清态：transformCopied 清 text、生成新 voiceId、重置 translateUnfold
// ---------------------------------------------------------------------------

test('drag: internal voiceBlock drag preserves text and voiceId in copied slice', () => {
  const { editor, window } = createEditor()
  const { bridge } = createMockBridge()
  setVoiceBridge(bridge)

  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'voice-drag-test',
      voicePath: 'voicenote/drag.mp3',
      voiceSize: 5000,
      createTime: '2026-07-17 10:00:00',
      title: '拖拽测试',
      text: '拖拽后应保留的转写文本',
      translateUnfold: false,
    },
  })

  let voicePos = -1
  editor.state.doc.descendants((node, pos) => {
    if (node.type.name === 'voiceBlock' && voicePos < 0) voicePos = pos
  })
  assert.ok(voicePos >= 0, 'voiceBlock should exist')

  const node = editor.state.doc.nodeAt(voicePos)
  const slice = editor.state.doc.slice(voicePos, voicePos + node.nodeSize)
  const voiceBox = editor.view.dom.querySelector('.voiceBox')
  assert.ok(voiceBox, 'voiceBox should render')

  voiceBox.dispatchEvent(new window.Event('dragstart', { bubbles: true, cancelable: true }))

  const transformCopied = editor.view.someProp('transformCopied', f => f)
  const transformed = transformCopied(slice)
  assert.equal(transformed, slice, 'internal drag should keep original slice')
  assert.equal(transformed.content.firstChild.attrs.text, '拖拽后应保留的转写文本')
  assert.equal(transformed.content.firstChild.attrs.voiceId, 'voice-drag-test')
  assert.equal(transformed.content.firstChild.attrs.translateUnfold, false)

  editor.view.dom.dispatchEvent(new window.Event('dragend', { bubbles: true, cancelable: true }))

  const copiedAfterDragEnd = transformCopied(slice)
  assert.notEqual(copiedAfterDragEnd, slice, 'after dragend, normal copy should clear voice metadata again')
  assert.equal(copiedAfterDragEnd.content.firstChild.attrs.text, null)
  assert.notEqual(copiedAfterDragEnd.content.firstChild.attrs.voiceId, 'voice-drag-test')

  editor.destroy()
})

test('copy: transformCopied clears text and generates new voiceId for voiceBlock', () => {
  const { editor } = createEditor()
  const { bridge } = createMockBridge()
  setVoiceBridge(bridge)

  editor.commands.insertContent({
    type: 'voiceBlock',
    attrs: {
      voiceId: 'voice-copy-test',
      voicePath: 'voicenote/test.mp3',
      voiceSize: 5000,
      createTime: '2026-07-17 10:00:00',
      title: '复制测试',
      text: '需要清除的转写文本',
      translateUnfold: false,
    },
  })

  // 获取文档中 voiceBlock 节点位置的 slice
  const { doc, selection } = editor.state
  let voicePos = -1
  doc.descendants((node, pos) => {
    if (node.type.name === 'voiceBlock' && voicePos < 0) voicePos = pos
  })
  assert.ok(voicePos >= 0, 'voiceBlock should exist')

  const node = doc.nodeAt(voicePos)
  const slice = editor.state.doc.slice(voicePos, voicePos + node.nodeSize)

  // 获取 transformCopied 插件 prop
  const transformCopied = editor.view.someProp('transformCopied', f => f)
  assert.ok(typeof transformCopied === 'function', 'transformCopied should be registered')

  const transformed = transformCopied(slice)
  const transformedNode = transformed.content.firstChild
  assert.ok(transformedNode, 'transformed slice should contain a node')
  assert.equal(transformedNode.attrs.text, null, 'text should be cleared')
  assert.notEqual(transformedNode.attrs.voiceId, 'voice-copy-test', 'voiceId should be regenerated')
  assert.notEqual(transformedNode.attrs.voiceId, '', 'new voiceId should be non-empty')
  assert.equal(transformedNode.attrs.translateUnfold, true, 'translateUnfold should be reset to true')

  // 原文档不应被修改
  const originalNode = editor.state.doc.nodeAt(voicePos)
  assert.equal(originalNode.attrs.text, '需要清除的转写文本', 'original text should be unchanged')
  assert.equal(originalNode.attrs.voiceId, 'voice-copy-test', 'original voiceId should be unchanged')

  editor.destroy()
})

test('copy: transformCopied does not modify non-voiceBlock content', () => {
  const { editor } = createEditor()
  const { bridge } = createMockBridge()
  setVoiceBridge(bridge)

  editor.commands.insertContent({ type: 'paragraph', content: [{ type: 'text', text: '普通文本' }] })

  const { doc } = editor.state
  let paraPos = -1
  doc.descendants((node, pos) => {
    if (node.type.name === 'paragraph' && paraPos < 0) paraPos = pos
  })
  const node = doc.nodeAt(paraPos)
  const slice = doc.slice(paraPos, paraPos + node.nodeSize)

  const transformCopied = editor.view.someProp('transformCopied', f => f)
  const transformed = transformCopied(slice)
  assert.equal(transformed, slice, 'non-voiceBlock slice should be returned as-is')

  editor.destroy()
})
