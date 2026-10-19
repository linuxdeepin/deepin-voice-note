// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 范围 Shift+Tab 会按分组逐项改动文档。处理靠后的分组时可能重排其祖先列表，
// 使靠前分组先前缓存的位置失效；后续必须按块身份重新定位，不能拿旧 pos 去
// resolve，否则会出现 Position out of range 并让快捷键直接报错。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { liftActiveListItem } from '../src/runtime/list-behavior.js'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'

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
  defineGlobal('KeyboardEvent', window.KeyboardEvent)
  defineGlobal('CustomEvent', window.CustomEvent)
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('getComputedStyle', window.getComputedStyle.bind(window))
  defineGlobal('requestAnimationFrame', (cb) => setTimeout(cb, 0))
  defineGlobal('cancelAnimationFrame', (id) => clearTimeout(id))
  return window
}

function paragraph(text) {
  return { type: 'paragraph', attrs: { dvnIndentLevel: 0 }, content: [{ type: 'text', text }] }
}

function item(type, text) {
  return {
    type,
    attrs: type === 'taskItem' ? { dvnIndentLevel: 0, checked: false } : { dvnIndentLevel: 0 },
    content: [paragraph(text)],
  }
}

function list(type, texts, itemType) {
  return {
    type,
    attrs: type === 'orderedList' ? { start: 1, type: null } : {},
    content: texts.map((text) => item(itemType, text)),
  }
}

function textRange(editor, text) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (found || !node.isText || node.text !== text) return found == null
    found = { from: pos, to: pos + node.text.length }
    return false
  })
  assert.ok(found, `text not found: ${text}`)
  return found
}

function textOrder(doc) {
  const order = []
  doc.descendants((node) => {
    if (node.isTextblock && !node.type.name.startsWith('list')) {
      order.push(node.textContent)
      return false
    }
    return true
  })
  return order
}

test('混合多个列表块的范围 Shift+Tab 分组处理后不会使用失效坐标', () => {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: {
      type: 'doc',
      content: [
        list('taskList', ['A', 'B'], 'taskItem'),
        list('bulletList', ['C', 'D', 'E'], 'listItem'),
        list('orderedList', ['F'], 'listItem'),
      ],
    },
  })

  const from = textRange(editor, 'B').from
  const to = textRange(editor, 'F').to
  editor.commands.setTextSelection({ from, to })
  assert.equal(liftActiveListItem(editor), true)
  assert.deepEqual(textOrder(editor.state.doc).sort(), ['A', 'B', 'C', 'D', 'E', 'F'])
  assert.doesNotThrow(() => editor.state.doc.resolve(editor.state.selection.from))
  editor.destroy()
})
