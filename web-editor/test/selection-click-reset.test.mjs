// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createEmptyDoc } from '../src/schema/document-envelope.js'
import { createSelectionClickResetPlugin } from '../src/runtime/selection-click-reset.js'

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
  defineGlobal('MouseEvent', window.MouseEvent)
  defineGlobal('CustomEvent', window.CustomEvent)
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('getComputedStyle', window.getComputedStyle.bind(window))
  defineGlobal('requestAnimationFrame', (cb) => setTimeout(cb, 0))
  defineGlobal('cancelAnimationFrame', (id) => clearTimeout(id))
  return window
}

function createEditor() {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
  editor.commands.insertContent('hello world')
  // happy-dom 无排版能力，固定 posAtCoords 结果，只验证本插件的选区收敛逻辑。
  editor.view.posAtCoords = () => ({ pos: 4, inside: -1 })
  return editor
}

const handlers = () => createSelectionClickResetPlugin().spec.props.handleDOMEvents

function pressEvent(view, extra = {}) {
  return {
    button: 0,
    buttons: 1,
    shiftKey: false,
    altKey: false,
    metaKey: false,
    ctrlKey: false,
    clientX: 10,
    clientY: 10,
    target: view.dom,
    ...extra,
  }
}

const tick = () => new Promise((resolve) => setTimeout(resolve, 5))

function selectRange(editor, from, to) {
  editor.commands.setTextSelection({ from, to })
  assert.equal(editor.state.selection.empty, false, '前置条件：存在非空选区')
}

test('非空选区内单击（未拖动）会折叠选区，不依赖浏览器 click 事件', async () => {
  const editor = createEditor()
  selectRange(editor, 1, 6)
  const before = editor.getText()
  const dom = handlers()

  dom.mousedown(editor.view, pressEvent(editor.view))
  dom.mouseup(editor.view, pressEvent(editor.view, { buttons: 0 }))
  await tick()

  assert.equal(editor.state.selection.empty, true, '单击后选区应折叠')
  assert.equal(editor.getText(), before, '文档内容不应变化')
  editor.destroy()
})

test('拖动超过阈值时保留选区（不影响拖动选区内文字）', async () => {
  const editor = createEditor()
  selectRange(editor, 1, 6)
  const dom = handlers()

  dom.mousedown(editor.view, pressEvent(editor.view))
  dom.mousemove(editor.view, pressEvent(editor.view, { clientX: 40, clientY: 40 }))
  dom.mouseup(editor.view, pressEvent(editor.view, { buttons: 0, clientX: 40, clientY: 40 }))
  await tick()

  assert.equal(editor.state.selection.from, 1)
  assert.equal(editor.state.selection.to, 6)
  editor.destroy()
})

test('dragstart 后不折叠选区', async () => {
  const editor = createEditor()
  selectRange(editor, 1, 6)
  const dom = handlers()

  dom.mousedown(editor.view, pressEvent(editor.view))
  dom.dragstart(editor.view, pressEvent(editor.view))
  dom.mouseup(editor.view, pressEvent(editor.view, { buttons: 0 }))
  await tick()

  assert.equal(editor.state.selection.empty, false)
  editor.destroy()
})

test('Shift 扩选不折叠选区', async () => {
  const editor = createEditor()
  selectRange(editor, 1, 6)
  const dom = handlers()

  dom.mousedown(editor.view, pressEvent(editor.view, { shiftKey: true }))
  dom.mouseup(editor.view, pressEvent(editor.view, { buttons: 0, shiftKey: true }))
  await tick()

  assert.equal(editor.state.selection.empty, false)
  editor.destroy()
})

test('抬起前选区已被其他逻辑改变时不介入（图片/待办等点击行为不受影响）', async () => {
  const editor = createEditor()
  selectRange(editor, 1, 6)
  const dom = handlers()

  dom.mousedown(editor.view, pressEvent(editor.view))
  editor.commands.setTextSelection({ from: 2, to: 8 })
  dom.mouseup(editor.view, pressEvent(editor.view, { buttons: 0 }))
  await tick()

  assert.equal(editor.state.selection.from, 2)
  assert.equal(editor.state.selection.to, 8)
  editor.destroy()
})

test('编辑器 DOM 上的真实 mousedown/mouseup 也会折叠选区', async () => {
  const editor = createEditor()
  selectRange(editor, 1, 6)
  const { MouseEvent } = globalThis

  editor.view.dom.dispatchEvent(
    new MouseEvent('mousedown', { bubbles: true, cancelable: true, button: 0, buttons: 1, clientX: 10, clientY: 10 }),
  )
  editor.view.dom.dispatchEvent(
    new MouseEvent('mouseup', { bubbles: true, cancelable: true, button: 0, buttons: 0, clientX: 10, clientY: 10 }),
  )
  await tick()

  assert.equal(editor.state.selection.empty, true)
  editor.destroy()
})
