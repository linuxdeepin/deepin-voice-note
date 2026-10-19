// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createEmptyDoc } from '../src/schema/document-envelope.js'
import {
  currentBlockPlainText,
  normalizeCopyEvent,
  normalizeCutEvent,
  selectionPlainText,
  stripPmSliceFromHtml,
} from '../src/runtime/clipboard-copy.js'

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
}

function createEditor(content = { type: 'doc', content: [{ type: 'paragraph' }] }) {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content,
  })
  return editor
}

function paragraphDoc(...texts) {
  return {
    type: 'doc',
    content: texts.map((text) => (
      text === ''
        ? { type: 'paragraph' }
        : { type: 'paragraph', content: [{ type: 'text', text }] }
    )),
  }
}

// 模拟 copy/cut 事件的 DataTransfer：只实现被归一逻辑用到的读写接口
function createEvent(editor, { html = '', plain = '', target } = {}) {
  const store = new Map()
  if (html) store.set('text/html', html)
  if (plain) store.set('text/plain', plain)
  const state = { prevented: 0 }
  const event = {
    target: target || editor.view.dom,
    clipboardData: {
      getData: (type) => store.get(type) || '',
      setData: (type, value) => store.set(type, value),
    },
    preventDefault() {
      state.prevented += 1
    },
  }
  return {
    event,
    data: store,
    get prevented() {
      return state.prevented
    },
  }
}

const SLICE_HTML = '<p data-pm-slice="0 0 []">abc</p>'

test('stripPmSliceFromHtml 去掉 data-pm-slice、保留其它 HTML', () => {
  assert.equal(stripPmSliceFromHtml(SLICE_HTML), '<p>abc</p>')
  assert.equal(stripPmSliceFromHtml('<ul data-pm-slice="1 1 []"><li>a</li></ul>'), '<ul><li>a</li></ul>')
  assert.equal(stripPmSliceFromHtml('<p>abc</p>'), '<p>abc</p>')
  assert.equal(stripPmSliceFromHtml(''), '')
  assert.equal(stripPmSliceFromHtml(null), '')
})

test('有文本选区时：纯文本来自文档、HTML 去掉 slice 标记', () => {
  const editor = createEditor(paragraphDoc('abc', 'def'))
  editor.commands.selectAll()
  const harness = createEvent(editor, { html: '<p data-pm-slice="0 0 []">abc</p><p>def</p>' })

  assert.equal(normalizeCopyEvent(harness.event, editor), true)
  assert.equal(harness.data.get('text/plain'), 'abc\ndef')
  assert.equal(harness.data.get('text/html'), '<p>abc</p><p>def</p>')
  assert.equal(harness.prevented, 1)
  assert.equal(selectionPlainText(editor.state), 'abc\ndef')
})

test('无 data-pm-slice 时 HTML 原样保留（外部复制格式不丢）', () => {
  const editor = createEditor(paragraphDoc('abc'))
  editor.commands.selectAll()
  const harness = createEvent(editor, { html: '<p><strong>abc</strong></p>' })

  assert.equal(normalizeCopyEvent(harness.event, editor), true)
  assert.equal(harness.data.get('text/html'), '<p><strong>abc</strong></p>')
  assert.equal(harness.data.get('text/plain'), 'abc')
})

test('光标无选区时：复制光标所在块的文本（Ctrl+C）', () => {
  const editor = createEditor(paragraphDoc('abc', 'def'))
  editor.commands.setTextSelection(2)
  const harness = createEvent(editor)

  assert.equal(normalizeCopyEvent(harness.event, editor), true)
  assert.equal(harness.data.get('text/plain'), 'abc')
  assert.equal(harness.data.get('text/html'), undefined)
  assert.equal(harness.prevented, 1)
  assert.equal(currentBlockPlainText(editor.state), 'abc')
})

test('光标在空段落时：调用宿主清空剪贴板（清掉上一次残留）', () => {
  const editor = createEditor(paragraphDoc(''))
  const harness = createEvent(editor)
  let cleared = 0

  assert.equal(normalizeCopyEvent(harness.event, editor, { clearClipboard: () => { cleared += 1 } }), true)
  assert.equal(cleared, 1)
  assert.equal(harness.prevented, 1)
  // 不用空字符串覆盖剪贴板：Chromium 会忽略空数据，残留内容反而留在剪贴板里
  assert.equal(harness.data.get('text/plain'), undefined)
})

test('列表项里无选区时：只复制当前项文本', () => {
  const editor = createEditor({
    type: 'doc',
    content: [{
      type: 'bulletList',
      content: [
        { type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: '第一项' }] }] },
        { type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: '第二项' }] }] },
      ],
    }],
  })
  // 光标放进第二项的段落
  let secondItemPos = 0
  editor.state.doc.descendants((node, pos) => {
    if (node.isText && node.text === '第二项') secondItemPos = pos + 1
    return true
  })
  editor.commands.setTextSelection(secondItemPos)
  const harness = createEvent(editor)

  assert.equal(normalizeCopyEvent(harness.event, editor), true)
  assert.equal(harness.data.get('text/plain'), '第二项')
})

test('剪切（Ctrl+X）空选区不做整行复制', () => {
  const editor = createEditor(paragraphDoc('abc'))
  editor.commands.setTextSelection(2)
  const harness = createEvent(editor)

  assert.equal(normalizeCutEvent(harness.event, editor), false)
  assert.equal(harness.data.get('text/plain'), undefined)
  assert.equal(harness.prevented, 0)
})

test('剪切有文本选区时与复制同样归一（ProseMirror 已写入剪贴板、选区已被删除）', () => {
  const editor = createEditor(paragraphDoc('cd'))
  // 真实剪切事件到达页面级监听时，ProseMirror 已经删除了选区，state 里取不到文本
  editor.commands.setTextSelection(1)
  const harness = createEvent(editor, {
    html: '<p data-pm-slice="1 1 []">cd</p>',
    plain: 'cd',
  })

  assert.equal(normalizeCutEvent(harness.event, editor), true)
  assert.equal(harness.data.get('text/plain'), 'cd')
  assert.equal(harness.data.get('text/html'), '<p>cd</p>')
})

test('语音转写文本的复制交给专用通道，不被归一覆盖', () => {
  const editor = createEditor(paragraphDoc('abc'))
  editor.commands.selectAll()
  const transcript = document.createElement('div')
  transcript.className = 'translateText'
  editor.view.dom.appendChild(transcript)
  const harness = createEvent(editor, { html: SLICE_HTML, target: transcript })

  assert.equal(normalizeCopyEvent(harness.event, editor), false)
  assert.equal(harness.data.get('text/plain'), undefined)
  assert.equal(harness.prevented, 0)
})

test('编辑器之外的复制不受影响', () => {
  const editor = createEditor(paragraphDoc('abc'))
  editor.commands.selectAll()
  const outside = document.createElement('div')
  document.body.appendChild(outside)
  const harness = createEvent(editor, { html: SLICE_HTML, target: outside })

  assert.equal(normalizeCopyEvent(harness.event, editor), false)
  assert.equal(harness.prevented, 0)
})

test('非文本块中的光标不做整行复制', () => {
  const state = {
    doc: { textBetween: () => '' },
    selection: {
      empty: true,
      from: 1,
      to: 1,
      $from: { parent: { isTextblock: false }, start: () => 0, end: () => 0 },
    },
  }
  const editor = { state, view: { dom: { contains: () => true } } }
  const harness = createEvent(editor, { target: 'inside' })

  assert.equal(currentBlockPlainText(state), null)
  assert.equal(normalizeCopyEvent(harness.event, editor), false)
  assert.equal(harness.prevented, 0)
})

test('没有 clipboardData 时不动剪贴板', () => {
  const editor = createEditor(paragraphDoc('abc'))
  editor.commands.selectAll()

  assert.equal(normalizeCopyEvent({ target: editor.view.dom }, editor), false)
  assert.equal(normalizeCopyEvent(undefined, editor), false)
})
