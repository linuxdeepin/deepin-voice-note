// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Voice block runtime tests: signal dispatch, playback/transcription
// runtime state isolation, theme variable writing, paste voiceId.

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
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
})

test('themeProvided: light theme variables', () => {
  setupDom()
  const { bridge, emit } = createMockBridge()
  setVoiceBridge(bridge)

  emit('themeProvided', 'light', '#0058DE', '#cccccc', '#FBFCFD')
  assert.equal(document.documentElement.style.getPropertyValue('--highlightColor'), '#0058DE')
  assert.equal(document.documentElement.style.getPropertyValue('--backgroundColor'), '#FBFCFD')
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
