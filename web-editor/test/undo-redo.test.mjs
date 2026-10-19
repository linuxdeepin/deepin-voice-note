// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { AllSelection } from '@tiptap/pm/state'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createEmptyDoc } from '../src/schema/document-envelope.js'
import { replaceEditorContentWithoutHistory } from '../src/runtime/undo-redo.js'

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

function createEditor() {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  return new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
}

test('undoRedo extension restores Tiptap document changes', () => {
  const editor = createEditor()

  editor.commands.insertContent({ type: 'text', text: 'hello' })
  assert.equal(editor.getText(), 'hello')

  assert.equal(editor.commands.undo(), true)
  assert.equal(editor.getText(), '')

  assert.equal(editor.commands.redo(), true)
  assert.equal(editor.getText(), 'hello')
  editor.destroy()
})

test('loading saved content does not create an undo event', () => {
  const editor = createEditor()
  const saved = {
    type: 'doc',
    content: [{ type: 'paragraph', content: [{ type: 'text', text: 'saved' }] }],
  }

  replaceEditorContentWithoutHistory(editor, saved)
  assert.equal(editor.getText(), 'saved')
  assert.equal(editor.commands.undo(), false)
  assert.equal(editor.getText(), 'saved')

  editor.commands.insertContent({ type: 'text', text: '!' })
  assert.equal(editor.getText(), 'saved!')
  assert.equal(editor.commands.undo(), true)
  assert.equal(editor.getText(), 'saved')
  assert.equal(editor.commands.redo(), true)
  assert.equal(editor.getText(), 'saved!')
  editor.destroy()
})

test('loading content drops the select-all selection of the previous note', () => {
  const editor = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{ type: 'paragraph', content: [{ type: 'text', text: 'first' }] }],
  })
  editor.commands.selectAll()
  assert.equal(editor.state.selection instanceof AllSelection, true)

  replaceEditorContentWithoutHistory(editor, {
    type: 'doc',
    content: [{ type: 'paragraph', content: [{ type: 'text', text: 'second' }] }],
  })

  assert.equal(editor.getText(), 'second')
  // 切换笔记后不得把上一个笔记的全选带过来：AllSelection.map() 会原样映射到新文档
  assert.equal(editor.state.selection instanceof AllSelection, false)
  assert.equal(editor.state.selection.empty, true)
  assert.equal(editor.state.selection.from, 1)
  editor.destroy()
})
