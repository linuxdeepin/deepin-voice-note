// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 异步资源插入光标管理：模拟工具栏文件选择、拖拽坐标与回插前恢复。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { createEmptyDoc } from '../src/schema/document-envelope.js'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createResourceInsertionSelection } from '../src/runtime/resource-insertion-selection.js'
import { insertImageWithLegacyFlow } from '../src/runtime/tiptap-channel.js'

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

function createEditor(content = createEmptyDoc()) {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  return new Editor({
    element,
    extensions: createTiptapExtensions(),
    content,
  })
}

test('resource insertion selection restores toolbar async insertion position once', () => {
  const editor = createEditor({ type: 'doc', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'abc' }] }] })
  const manager = createResourceInsertionSelection(editor)

  editor.commands.setTextSelection(2)
  assert.equal(manager.capture(), true)
  editor.commands.setTextSelection(4)

  assert.equal(manager.restoreAndConsume(), true)
  assert.equal(insertImageWithLegacyFlow(editor, { src: 'file:///base/images/a.png', relPath: 'images/a.png' }), true)

  const nodes = editor.getJSON().content[0].content
  assert.equal(nodes[0].text, 'a')
  assert.equal(nodes[1].type, 'image')
  assert.equal(nodes[2].text, 'bc')
  assert.equal(manager.restoreAndConsume(), false, 'bookmark is consumed after the first resource')

  manager.destroy()
  editor.destroy()
})

test('resource insertion selection maps through document edits while dialog is open', () => {
  const editor = createEditor({ type: 'doc', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'abc' }] }] })
  const manager = createResourceInsertionSelection(editor)

  editor.commands.setTextSelection(3)
  assert.equal(manager.capture(), true)
  editor.commands.insertContentAt(1, 'X')
  editor.commands.setTextSelection(5)

  assert.equal(manager.restoreAndConsume(), true)
  assert.equal(editor.state.selection.from, 4)

  manager.destroy()
  editor.destroy()
})

test('resource insertion selection captures drag/drop client coordinates', () => {
  const editor = createEditor({ type: 'doc', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'abc' }] }] })
  const manager = createResourceInsertionSelection(editor)
  const originalPosAtCoords = editor.view.posAtCoords.bind(editor.view)

  editor.view.posAtCoords = () => ({ pos: 2 })
  assert.equal(manager.captureAtClientPoint(10, 20), true)
  editor.commands.setTextSelection(4)

  assert.equal(manager.restoreAndConsume(), true)
  assert.equal(editor.state.selection.from, 2)

  editor.view.posAtCoords = originalPosAtCoords
  manager.destroy()
  editor.destroy()
})

test('resource insertion selection preserves drop side on bare image rows', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'image', attrs: { src: 'file:///x/a.png', relPath: 'images/a.png', alt: '', title: null } }] },
      { type: 'paragraph', content: [{ type: 'text', text: 'tail' }] },
    ],
  })
  const manager = createResourceInsertionSelection(editor)
  const img = editor.view.dom.querySelector('img[data-rel-path]')
  assert.ok(img, 'image should be rendered')
  const paragraph = img.closest('p')
  assert.ok(paragraph, 'image paragraph should be rendered')
  editor.view.dom.getBoundingClientRect = () => ({ left: 0, right: 300, top: 0, bottom: 200, width: 300, height: 200 })
  paragraph.getBoundingClientRect = () => ({ left: 0, right: 300, top: 10, bottom: 90, width: 300, height: 80 })
  img.getBoundingClientRect = () => ({ left: 40, right: 140, top: 20, bottom: 80, width: 100, height: 60 })

  assert.equal(manager.captureAtClientPoint(45, 40), true)
  editor.commands.setTextSelection(7)
  assert.equal(manager.restoreAndConsume(), true)
  assert.equal(editor.state.selection.from, 1, 'drop on the left half of image row should restore before-image caret')

  assert.equal(manager.captureAtClientPoint(180, 40), true)
  editor.commands.setTextSelection(7)
  assert.equal(manager.restoreAndConsume(), true)
  assert.equal(editor.state.selection.from, 2, 'drop on the right side of image row should restore after-image caret')

  assert.equal(manager.captureAtClientPoint(45, 85), true)
  editor.commands.setTextSelection(7)
  assert.equal(manager.restoreAndConsume(), true)
  assert.equal(editor.state.selection.from, 2, 'drop in the runtime trailing area below an image should insert after that image')

  editor.view.dom.style.paddingLeft = '30px'
  editor.view.dom.style.paddingRight = '20px'
  editor.view.dom.getBoundingClientRect = () => ({ left: 0, right: 320, top: 0, bottom: 200, width: 320, height: 200 })
  paragraph.getBoundingClientRect = () => ({ left: 30, right: 300, top: 10, bottom: 90, width: 270, height: 80 })
  img.getBoundingClientRect = () => ({ left: 30, right: 300, top: 20, bottom: 80, width: 270, height: 60 })
  assert.equal(manager.captureAtClientPoint(315, 40), true)
  editor.commands.setTextSelection(7)
  assert.equal(manager.restoreAndConsume(), true)
  assert.equal(editor.state.selection.from, 2, 'drop in editor right padding of a full-width image row should restore after-image caret')


  manager.destroy()
  editor.destroy()
})
