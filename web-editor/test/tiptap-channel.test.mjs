// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 正式 QWebChannel 通道绑定单测（绑定/节流/往返/插入失败回告）

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import {
  createEmptyDoc,
  createEnvelope,
  serializeEnvelope,
} from '../src/schema/document-envelope.js'
import {
  bindTiptapChannel,
  createDebounce,
} from '../src/runtime/tiptap-channel.js'
import { parseImageInfo, parseVoiceInfo } from '../src/runtime/tiptap-adapter.js'

// ---------------------------------------------------------------------------
// happy-dom DOM 环境设置
// ---------------------------------------------------------------------------

function defineGlobal(name, value) {
  Object.defineProperty(globalThis, name, {
    value,
    writable: true,
    configurable: true,
  })
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

// ---------------------------------------------------------------------------
// Mock bridge：模拟 C++ TiptapChannelBridge 的 QWebChannel 对象
// ---------------------------------------------------------------------------

function createMockBridge(resourceBaseUrl = 'file:///usr/share/deepin-voice-note/web') {
  const handlers = {}
  const calls = {}

  function makeConnectable(name) {
    return {
      connect(fn) {
        handlers[name] = fn
      },
    }
  }

  function emit(name, ...args) {
    if (handlers[name]) handlers[name](...args)
  }

  const bridge = {
    resourceBaseUrl,
    // JS→C++ 回告方法（记录调用）
    jsEditorReady() {
      calls.editorReady = (calls.editorReady || 0) + 1
    },
    jsContentChanged() {
      calls.contentChanged = (calls.contentChanged || 0) + 1
    },
    jsContentSaved(envelope) {
      calls.contentSaved = calls.contentSaved || []
      calls.contentSaved.push(envelope)
    },
    jsInsertImageFailed(reason) {
      calls.insertImageFailed = calls.insertImageFailed || []
      calls.insertImageFailed.push(reason)
    },
    jsInsertVoiceBlockFailed(reason) {
      calls.insertVoiceBlockFailed = calls.insertVoiceBlockFailed || []
      calls.insertVoiceBlockFailed.push(reason)
    },
    // C++→JS signals（可 connect）
    loadEnvelopeRequested: makeConnectable('loadEnvelopeRequested'),
    requestContent: makeConnectable('requestContent'),
    insertImage: makeConnectable('insertImage'),
    insertVoiceBlock: makeConnectable('insertVoiceBlock'),
  }

  return { bridge, calls, emit }
}

function createMockChannelFactory(bridge) {
  return (cb) => {
    cb({ objects: { tiptapChannel: bridge } })
  }
}

// ---------------------------------------------------------------------------
// 测试：通道绑定
// ---------------------------------------------------------------------------

test('bindTiptapChannel: editorReady called on bind', async () => {
  const { editor } = createEditor()
  const { bridge, calls } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  assert.equal(calls.editorReady, 1)
  editor.destroy()
})

test('bindTiptapChannel: rejects when tiptapChannel not found', async () => {
  const { editor } = createEditor()
  const factory = (cb) => cb({ objects: {} })

  await assert.rejects(
    bindTiptapChannel(editor, factory),
    /tiptapChannel not found/,
  )
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 测试：加载 envelope
// ---------------------------------------------------------------------------

test('bindTiptapChannel: loadEnvelopeRequested sets editor content', async () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  const envelope = createEnvelope({
    type: 'doc',
    content: [{ type: 'paragraph', content: [{ type: 'text', text: 'loaded' }] }],
  })
  emit('loadEnvelopeRequested', serializeEnvelope(envelope))

  const json = editor.getJSON()
  assert.deepEqual(json.content[0].content[0].text, 'loaded')
  editor.destroy()
})

test('bindTiptapChannel: invalid envelope falls back to empty doc', async () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)
  emit('loadEnvelopeRequested', 'not valid json')

  const json = editor.getJSON()
  assert.deepEqual(json, { type: 'doc', content: [{ type: 'paragraph' }] })
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 测试：保存往返
// ---------------------------------------------------------------------------

test('bindTiptapChannel: requestContent → contentSaved roundtrip', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 编辑内容
  editor.commands.insertContent({ type: 'text', text: 'saved content' })

  // 触发保存请求
  emit('requestContent')

  assert.ok(calls.contentSaved)
  assert.equal(calls.contentSaved.length, 1)
  const envelope = JSON.parse(calls.contentSaved[0])
  assert.equal(envelope.format, 'tiptap')
  assert.equal(envelope.schemaVersion, 1)
  assert.equal(envelope.content.type, 'doc')
  editor.destroy()
})

test('bindTiptapChannel: empty doc save produces single empty paragraph', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 空文档状态触发保存
  emit('requestContent')

  const envelope = JSON.parse(calls.contentSaved[0])
  assert.deepEqual(envelope.content, { type: 'doc', content: [{ type: 'paragraph' }] })
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 测试：内容变化节流
// ---------------------------------------------------------------------------

test('createDebounce: only fires once after multiple triggers within delay', async () => {
  let count = 0
  const deb = createDebounce(() => { count++ }, 50)

  deb.trigger()
  deb.trigger()
  deb.trigger()

  // 等待延迟后执行
  await new Promise(r => setTimeout(r, 100))
  assert.equal(count, 1)
})

test('createDebounce: cancel prevents fire', async () => {
  let count = 0
  const deb = createDebounce(() => { count++ }, 50)

  deb.trigger()
  deb.cancel()

  await new Promise(r => setTimeout(r, 100))
  assert.equal(count, 0)
})

test('createDebounce: flush fires immediately', () => {
  let count = 0
  const deb = createDebounce(() => { count++ }, 1000)

  deb.trigger()
  deb.flush()
  assert.equal(count, 1)
})

test('bindTiptapChannel: contentChanged is debounced', async () => {
  const { editor } = createEditor()
  const { bridge, calls } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 多次编辑触发 update
  editor.commands.insertContent({ type: 'text', text: 'a' })
  editor.commands.insertContent({ type: 'text', text: 'b' })
  editor.commands.insertContent({ type: 'text', text: 'c' })

  // 立即检查：节流窗口内不应回告
  assert.equal(calls.contentChanged || 0, 0)

  // 等待节流延迟后
  await new Promise(r => setTimeout(r, 300))
  assert.equal(calls.contentChanged, 1)
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 测试：插入图片 / 语音 + 失败回告
// ---------------------------------------------------------------------------

test('bindTiptapChannel: insertImage success inserts image node', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  emit('insertImage', JSON.stringify({ relPath: 'images/test.png' }))

  const json = editor.getJSON()
  const imageNode = json.content.find(n => n.type === 'image')
  assert.ok(imageNode, 'image node should be inserted')
  assert.ok(imageNode.attrs.src.includes('images/test.png'))
  assert.equal(imageNode.attrs.relPath, 'images/test.png')
  assert.equal(calls.insertImageFailed, undefined)
  editor.destroy()
})

test('bindTiptapChannel: insertImage failed reports reason', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 无效 imageInfoJson
  emit('insertImage', 'not json')
  assert.ok(calls.insertImageFailed)
  assert.equal(calls.insertImageFailed.length, 1)

  // 缺少 relPath
  emit('insertImage', JSON.stringify({}))
  assert.equal(calls.insertImageFailed.length, 2)
  editor.destroy()
})

test('bindTiptapChannel: insertVoiceBlock success inserts voiceBlock node', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  emit('insertVoiceBlock', JSON.stringify({
    voiceId: 'v1',
    voicePath: 'voice/v1.wav',
    voiceSize: 1024,
  }))

  const json = editor.getJSON()
  const voiceNode = json.content.find(n => n.type === 'voiceBlock')
  assert.ok(voiceNode, 'voiceBlock node should be inserted')
  assert.equal(voiceNode.attrs.voiceId, 'v1')
  assert.ok(voiceNode.attrs.voicePath.includes('voice/v1.wav'))
  assert.equal(voiceNode.attrs.voiceSize, 1024)
  assert.equal(calls.insertVoiceBlockFailed, undefined)
  editor.destroy()
})

test('bindTiptapChannel: insertVoiceBlock failed reports reason', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 缺少 voiceId
  emit('insertVoiceBlock', JSON.stringify({ voicePath: 'voice/v1.wav' }))
  assert.ok(calls.insertVoiceBlockFailed)
  assert.equal(calls.insertVoiceBlockFailed.length, 1)

  // 缺少 voicePath
  emit('insertVoiceBlock', JSON.stringify({ voiceId: 'v1' }))
  assert.equal(calls.insertVoiceBlockFailed.length, 2)
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 测试：适配层
// ---------------------------------------------------------------------------

test('parseImageInfo: valid relPath resolves to absolute URL', () => {
  const result = parseImageInfo(
    JSON.stringify({ relPath: 'images/test.png' }),
    'file:///usr/share/deepin-voice-note/web',
  )
  assert.ok(result.ok)
  assert.ok(result.attrs.src.startsWith('file:///'))
  assert.ok(result.attrs.src.includes('images/test.png'))
  assert.equal(result.attrs.relPath, 'images/test.png')
})

test('parseImageInfo: absolute URL returned as-is', () => {
  const result = parseImageInfo(
    JSON.stringify({ relPath: 'data:image/png;base64,abc' }),
    'file:///base',
  )
  assert.ok(result.ok)
  assert.equal(result.attrs.src, 'data:image/png;base64,abc')
})

test('parseImageInfo: missing relPath fails', () => {
  const result = parseImageInfo('{}', 'file:///base')
  assert.ok(!result.ok)
  assert.ok(result.reason.includes('relPath'))
})

test('parseVoiceInfo: valid info resolves voicePath', () => {
  const result = parseVoiceInfo(
    JSON.stringify({ voiceId: 'v1', voicePath: 'voice/v1.wav', voiceSize: 512 }),
    'file:///base',
  )
  assert.ok(result.ok)
  assert.equal(result.attrs.voiceId, 'v1')
  assert.ok(result.attrs.voicePath.startsWith('file:///'))
  assert.equal(result.attrs.voiceSize, 512)
})

test('parseVoiceInfo: missing voiceId fails', () => {
  const result = parseVoiceInfo(
    JSON.stringify({ voicePath: 'voice/v1.wav' }),
    'file:///base',
  )
  assert.ok(!result.ok)
  assert.ok(result.reason.includes('voiceId'))
})

// ---------------------------------------------------------------------------
// 测试：编辑器拒绝插入时 .run() 返回 false → 失败回告
// ---------------------------------------------------------------------------

test('bindTiptapChannel: insertImage reports failure when editor rejects insertion', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 模拟编辑器层面拒绝插入（schema 不接受 attrs 等）
  const realChain = editor.chain.bind(editor)
  editor.chain = () => {
    const chain = realChain()
    const realRun = chain.run.bind(chain)
    chain.run = () => false
    return chain
  }

  emit('insertImage', JSON.stringify({ relPath: 'images/test.png' }))
  assert.ok(calls.insertImageFailed, 'should report failure when .run() returns false')
  assert.equal(calls.insertImageFailed.length, 1)
  assert.ok(calls.insertImageFailed[0].includes('rejected'))
  editor.destroy()
})

test('bindTiptapChannel: insertVoiceBlock reports failure when editor rejects insertion', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  const realChain = editor.chain.bind(editor)
  editor.chain = () => {
    const chain = realChain()
    const realRun = chain.run.bind(chain)
    chain.run = () => false
    return chain
  }

  emit('insertVoiceBlock', JSON.stringify({
    voiceId: 'v1',
    voicePath: 'voice/v1.wav',
    voiceSize: 1024,
  }))
  assert.ok(calls.insertVoiceBlockFailed, 'should report failure when .run() returns false')
  assert.equal(calls.insertVoiceBlockFailed.length, 1)
  assert.ok(calls.insertVoiceBlockFailed[0].includes('rejected'))
  editor.destroy()
})
