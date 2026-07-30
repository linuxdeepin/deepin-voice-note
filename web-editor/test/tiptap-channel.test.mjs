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
  validateEnvelope,
  isSafeImageSrc,
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

function createMockBridge(resourceBaseUrl = 'file:///home/user/.local/share/deepin-voice-note') {
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
    fontListProvided: makeConnectable('fontListProvided'),
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
// 测试：字体列表下发
// ---------------------------------------------------------------------------

test('bindTiptapChannel: fontListProvided invokes options.onFontList', async () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  const received = []
  await bindTiptapChannel(editor, factory, {
    onFontList: (fonts, defaultFont) => received.push({ fonts, defaultFont }),
  })

  emit('fontListProvided', ['Arial', 'Noto Sans CJK SC'], 'Noto Sans CJK SC')

  assert.equal(received.length, 1)
  assert.deepEqual(received[0].fonts, ['Arial', 'Noto Sans CJK SC'])
  assert.equal(received[0].defaultFont, 'Noto Sans CJK SC')
  editor.destroy()
})

test('bindTiptapChannel: fontListProvided does not interfere with other events', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory, { onFontList: () => {} })

  // 下发字体列表不应触发内容变化/保存/加载等既有事件
  emit('fontListProvided', ['Arial'], 'Arial')
  assert.equal(calls.contentChanged || 0, 0)
  assert.equal(calls.contentSaved || 0, 0)

  // 既有事件仍正常工作
  emit('requestContent')
  assert.ok(calls.contentSaved)
  assert.equal(calls.contentSaved.length, 1)
  editor.destroy()
})

test('bindTiptapChannel: works without onFontList option', async () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)
  // 无 onFontList 时下发不应抛错
  assert.doesNotThrow(() => emit('fontListProvided', ['Arial'], 'Arial'))
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

// ---------------------------------------------------------------------------
// 测试：保存归一 / 加载解析框架（image 绝对↔相对）
// ---------------------------------------------------------------------------

const ABSOLUTE_IMAGE_SRC = 'file:///usr/share/deepin-voice-note/web/images/photo.png'
const RELATIVE_IMAGE_PATH = 'images/photo.png'

function insertImageNode(editor, src, relPath) {
  editor.commands.insertContent({
    type: 'image',
    attrs: { src, relPath, alt: '', title: null },
  })
}

test('save normalize: absolute image src replaced by relative relPath on save', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 插入一张绝对 src + 相对 relPath 的图片（编辑区显示态）
  insertImageNode(editor, ABSOLUTE_IMAGE_SRC, RELATIVE_IMAGE_PATH)

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save should produce a contentSaved envelope')
  const envelope = JSON.parse(calls.contentSaved[0])
  const imageNode = envelope.content.content.find((n) => n.type === 'image')
  assert.ok(imageNode, 'image node should be in saved envelope')
  assert.equal(imageNode.attrs.src, RELATIVE_IMAGE_PATH, 'saved src must be relative relPath')
  assert.ok(!imageNode.attrs.src.includes('file://'), 'saved src must not contain file://')
  assert.equal(imageNode.attrs.relPath, RELATIVE_IMAGE_PATH, 'relPath preserved')
  editor.destroy()
})

test('load resolve: relative image src expanded to absolute file:// for display', async () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 下发一个只含相对路径 src 的 envelope（持久态）
  const persisted = {
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: RELATIVE_IMAGE_PATH, relPath: RELATIVE_IMAGE_PATH, alt: '', title: null },
    }],
  }
  emit('loadEnvelopeRequested', JSON.stringify(createEnvelope(persisted)))

  const json = editor.getJSON()
  const imageNode = json.content.find((n) => n.type === 'image')
  assert.ok(imageNode, 'image node should be loaded into editor')
  assert.ok(imageNode.attrs.src.startsWith('file:///'), 'display src must be absolute file://')
  assert.ok(imageNode.attrs.src.includes(RELATIVE_IMAGE_PATH), 'display src must contain the relative path')
  assert.equal(imageNode.attrs.relPath, RELATIVE_IMAGE_PATH, 'relPath unchanged by load resolve')
  editor.destroy()
})

test('save normalize: normalized image envelope passes validateEnvelope', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)
  insertImageNode(editor, ABSOLUTE_IMAGE_SRC, RELATIVE_IMAGE_PATH)

  emit('requestContent')

  assert.ok(calls.contentSaved, 'envelope should be saved (validation passed)')
  const result = validateEnvelope(JSON.parse(calls.contentSaved[0]))
  assert.equal(result.ok, true, JSON.stringify(result.errors))
  editor.destroy()
})

test('save normalize: image with absolute file:// src and no relPath is sanitized (keeps rest)', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 模拟丢失 relPath 的旧数据：仅绝对 file:// src，无 relPath
  insertImageNode(editor, ABSOLUTE_IMAGE_SRC, null)

  emit('requestContent')

  // 校验失败的图片节点被净化后仍回告宿主，非图片内容不静默丢失
  assert.ok(calls.contentSaved, 'unsafe image node should be sanitized, save should still report back')
  const envelope = JSON.parse(calls.contentSaved[0])
  const imageNode = envelope.content.content.find((n) => n.type === 'image')
  assert.equal(imageNode, undefined, 'unsafe image node should be removed from saved envelope')
  editor.destroy()
})

test('save → load → save roundtrip keeps image path relative', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)
  insertImageNode(editor, ABSOLUTE_IMAGE_SRC, RELATIVE_IMAGE_PATH)

  // 第一次保存：绝对 src 归一为相对
  emit('requestContent')
  const firstEnvelope = JSON.parse(calls.contentSaved[0])
  const firstImage = firstEnvelope.content.content.find((n) => n.type === 'image')
  assert.equal(firstImage.attrs.src, RELATIVE_IMAGE_PATH)

  // 加载第一次保存的 envelope（相对 src 解析为绝对显示）
  emit('loadEnvelopeRequested', JSON.stringify(firstEnvelope))
  const loadedImage = editor.getJSON().content.find((n) => n.type === 'image')
  assert.ok(loadedImage.attrs.src.startsWith('file:///'))

  // 再次保存：绝对 src 再次归一为相对
  calls.contentSaved.length = 0
  emit('requestContent')
  const secondEnvelope = JSON.parse(calls.contentSaved[0])
  const secondImage = secondEnvelope.content.content.find((n) => n.type === 'image')
  assert.equal(secondImage.attrs.src, RELATIVE_IMAGE_PATH, 'second save must also be relative')
  assert.deepEqual(firstEnvelope, secondEnvelope, 'save output must be stable across roundtrips')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// CR rework tests: AppData path, empty relPath, relPath validation, sanitize
// ---------------------------------------------------------------------------

test('load resolve: resolved image src must fall in AppData images dir, not WEB_PATH', async () => {
  const { editor } = createEditor()
  const { bridge, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  const persisted = {
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: RELATIVE_IMAGE_PATH, relPath: RELATIVE_IMAGE_PATH, alt: '', title: null },
    }],
  }
  emit('loadEnvelopeRequested', JSON.stringify(createEnvelope(persisted)))

  const imageNode = editor.getJSON().content.find((n) => n.type === 'image')
  // 解析后的绝对 src 必须落在 AppData images 目录，不得指向 WEB_PATH（web 资源安装目录）
  assert.ok(imageNode.attrs.src.startsWith('file:///'), 'resolved src must be absolute file://')
  assert.ok(imageNode.attrs.src.includes('/images/'), 'resolved src must contain images/ dir')
  assert.ok(!imageNode.attrs.src.includes('/web/images/'), 'resolved src must NOT be under WEB_PATH (web install dir)')
  assert.ok(imageNode.attrs.src.includes('deepin-voice-note/images/'), 'resolved src must be under AppData root')
  editor.destroy()
})

test('save normalize: empty-string relPath falls back to src (#3)', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // relPath 为空串时应回退到 src，不应置空导致校验失败
  editor.commands.insertContent({
    type: 'image',
    attrs: { src: RELATIVE_IMAGE_PATH, relPath: '', alt: '', title: null },
  })

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save should succeed when relPath is empty (falls back to src)')
  const envelope = JSON.parse(calls.contentSaved[0])
  const imageNode = envelope.content.content.find((n) => n.type === 'image')
  assert.ok(imageNode, 'image should survive when relPath is empty string')
  assert.equal(imageNode.attrs.src, RELATIVE_IMAGE_PATH, 'src should fall back from empty relPath to src')
  editor.destroy()
})

test('validateEnvelope: rejects remote relPath (#4)', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/a.png', relPath: 'https://evil.com/a.png', alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, false)
  assert.ok(result.errors.some((e) => e.code === 'unsafe-image-relpath'))
})

test('validateEnvelope: rejects absolute file:// relPath (#4)', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/a.png', relPath: 'file:///etc/passwd', alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, false)
  assert.ok(result.errors.some((e) => e.code === 'unsafe-image-relpath'))
})

test('validateEnvelope: accepts null relPath (#4 backward compat)', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/a.png', relPath: null, alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, true, JSON.stringify(result.errors))
})

test('validateEnvelope: accepts valid images/ relPath (#4)', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/photo.png', relPath: 'images/photo.png', alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, true, JSON.stringify(result.errors))
})

test('save sanitize: preserves non-image content when image is unsafe (#2)', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 先插入文本，再插入一张无 relPath 的 file:// 图片
  editor.commands.insertContent({ type: 'text', text: 'important text' })
  insertImageNode(editor, ABSOLUTE_IMAGE_SRC, null)

  emit('requestContent')

  // 非法图片被净化，但文本内容保留，仍回告宿主
  assert.ok(calls.contentSaved, 'save should still report back after sanitizing')
  const envelope = JSON.parse(calls.contentSaved[0])
  const hasText = JSON.stringify(envelope.content).includes('important text')
  assert.ok(hasText, 'non-image content must be preserved after sanitize')
  const imageNode = envelope.content.content.find((n) => n.type === 'image')
  assert.equal(imageNode, undefined, 'unsafe image should be removed')
  editor.destroy()
})

test('isSafeImageSrc is exported and usable externally', () => {
  assert.equal(isSafeImageSrc('images/a.png'), true)
  assert.equal(isSafeImageSrc('file:///x/a.png'), false)
  assert.equal(isSafeImageSrc('https://x.com/a.png'), true)
})

// ---------------------------------------------------------------------------
// CR rework round 2: recursive sanitize, remote relPath sanitize, traversal
// ---------------------------------------------------------------------------

test('sanitize removes unsafe image nested in list item (#1 recursion)', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 嵌套结构：listItem 内含一张无 relPath 的 file:// 图片 + 文本
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'bulletList',
      content: [{
        type: 'listItem',
        content: [{
          type: 'paragraph',
          content: [
            { type: 'text', text: 'keep me' },
            { type: 'image', attrs: { src: ABSOLUTE_IMAGE_SRC, relPath: null, alt: '', title: null } },
          ],
        }],
      }],
    }],
  })

  emit('requestContent')

  // 嵌套非法图片被净化后仍回告，非图片内容保留
  assert.ok(calls.contentSaved, 'save should report back after recursive sanitize')
  const envelope = JSON.parse(calls.contentSaved[0])
  const serialized = JSON.stringify(envelope.content)
  assert.ok(serialized.includes('keep me'), 'nested non-image content must survive sanitize')
  assert.ok(!serialized.includes('photo.png'), 'nested unsafe image must be removed')
  editor.destroy()
})

test('sanitize removes image with remote relPath (#2)', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 归一后 src = relPath = 远程 URL，isSafeImageSrc(http) 返回 true 但 relPath 校验失败
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'paragraph',
      content: [
        { type: 'text', text: 'text survives' },
        { type: 'image', attrs: { src: 'https://evil.com/x.png', relPath: 'https://evil.com/x.png', alt: '', title: null } },
      ],
    }],
  })

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save should report back after sanitizing remote relPath')
  const envelope = JSON.parse(calls.contentSaved[0])
  const serialized = JSON.stringify(envelope.content)
  assert.ok(serialized.includes('text survives'), 'non-image content must survive')
  assert.ok(!serialized.includes('evil.com'), 'image with remote relPath must be sanitized out')
  editor.destroy()
})

test('validateEnvelope rejects relPath with .. traversal (#3)', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/a.png', relPath: 'images/../../etc/passwd', alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, false)
  assert.ok(result.errors.some((e) => e.code === 'unsafe-image-relpath'))
})

test('validateEnvelope rejects relPath images/.. (#3)', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/a.png', relPath: 'images/..', alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, false)
  assert.ok(result.errors.some((e) => e.code === 'unsafe-image-relpath'))
})

test('validateEnvelope accepts relPath images/foo/bar.png (no traversal, #3)', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/sub/a.png', relPath: 'images/sub/a.png', alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, true, JSON.stringify(result.errors))
})

test('validateEnvelope accepts relPath images/... (literal dots, not traversal)', () => {
  // 'images/...png' 含 '...' 不是 '..' 段，不构成穿越
  const envelope = createEnvelope({
    type: 'doc',
    content: [{
      type: 'image',
      attrs: { src: 'images/...png', relPath: 'images/...png', alt: '', title: null },
    }],
  })
  const result = validateEnvelope(envelope)
  assert.equal(result.ok, true, JSON.stringify(result.errors))
})

test('sanitize preserves non-image content alongside unsafe image (#2)', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'para one' }] },
      { type: 'image', attrs: { src: ABSOLUTE_IMAGE_SRC, relPath: null, alt: '', title: null } },
      { type: 'paragraph', content: [{ type: 'text', text: 'para two' }] },
    ],
  })

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save should report back')
  const serialized = JSON.stringify(JSON.parse(calls.contentSaved[0]).content)
  assert.ok(serialized.includes('para one') && serialized.includes('para two'), 'surrounding paragraphs survive')
  assert.ok(!serialized.includes('photo.png'), 'unsafe image removed')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// CR rework round 3: empty container after sanitize (空容器边界)
// ---------------------------------------------------------------------------

test('sanitize drops empty blockquote left by sole unsafe image, save reports back', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // blockquote 内仅含一张非法图片（file:// 无 relPath），净化后 blockquote 变空
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'blockquote',
      content: [{ type: 'image', attrs: { src: ABSOLUTE_IMAGE_SRC, relPath: null, alt: '', title: null } }],
    }],
  })

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save must report back even when a container becomes empty')
  const serialized = JSON.stringify(JSON.parse(calls.contentSaved[0]).content)
  assert.ok(!serialized.includes('photo.png'), 'unsafe image must be removed')
  assert.ok(!serialized.includes('blockquote'), 'empty blockquote must be removed')
  editor.destroy()
})

test('sanitize cascades: bulletList>listItem with only unsafe image removed', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'bulletList',
      content: [{
        type: 'listItem',
        content: [{ type: 'image', attrs: { src: ABSOLUTE_IMAGE_SRC, relPath: null, alt: '', title: null } }],
      }],
    }],
  })

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save must report back after cascading container removal')
  const serialized = JSON.stringify(JSON.parse(calls.contentSaved[0]).content)
  assert.ok(!serialized.includes('photo.png'), 'unsafe image removed')
  assert.ok(!serialized.includes('bulletList') && !serialized.includes('listItem'),
    'empty list containers must be removed via cascade')
  editor.destroy()
})

test('sanitize keeps blockquote text when image alongside is unsafe', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'blockquote',
      content: [{
        type: 'paragraph',
        content: [
          { type: 'text', text: 'survives' },
          { type: 'image', attrs: { src: ABSOLUTE_IMAGE_SRC, relPath: null, alt: '', title: null } },
        ],
      }],
    }],
  })

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save must report back')
  const serialized = JSON.stringify(JSON.parse(calls.contentSaved[0]).content)
  assert.ok(serialized.includes('survives'), 'text in same container must survive')
  assert.ok(!serialized.includes('photo.png'), 'unsafe image removed')
  assert.ok(serialized.includes('blockquote'), 'blockquote with remaining text kept')
  editor.destroy()
})

test('sanitize fills empty doc root with single empty paragraph', async () => {
  const { editor } = createEditor()
  const { bridge, calls, emit } = createMockBridge()
  const factory = createMockChannelFactory(bridge)

  await bindTiptapChannel(editor, factory)

  // 仅含一张非法图片，净化后 doc 根变空 → 补单空段落
  editor.commands.setContent({
    type: 'doc',
    content: [{ type: 'image', attrs: { src: ABSOLUTE_IMAGE_SRC, relPath: null, alt: '', title: null } }],
  })

  emit('requestContent')

  assert.ok(calls.contentSaved, 'save must report back when doc root becomes empty')
  const content = JSON.parse(calls.contentSaved[0]).content
  assert.deepEqual(content, { type: 'doc', content: [{ type: 'paragraph' }] },
    'empty doc root must be filled with a single empty paragraph')
  editor.destroy()
})
