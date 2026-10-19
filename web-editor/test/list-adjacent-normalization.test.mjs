// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 相邻同类型列表只在分界两侧处于同一视觉层级时归一为一个列表。
// 这防止“局部转换后同一父级、同一缩进的有序列表各自从 1 开始”，
// 同时又不能把视觉上属于父子关系的两个列表错误合并。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import {
  joinAdjacentSameTypeLists,
  nestAdjacentVisualLists,
  toggleActiveListFormatting,
} from '../src/runtime/list-behavior.js'
import { orderedListMarkerNumbers } from '../src/runtime/normal-list-markers.js'

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

function item(text, dvnIndentLevel = 0) {
  return {
    type: 'listItem',
    attrs: { dvnIndentLevel },
    content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
  }
}

function ordered(items, start = 1) {
  return { type: 'orderedList', attrs: { start }, content: items }
}

function createEditor(content) {
  const window = setupDom()
  const element = window.document.createElement('div')
  window.document.body.appendChild(element)
  return new Editor({ element, extensions: createTiptapExtensions(), content })
}

function triggerDocumentNormalization(editor) {
  const tr = editor.state.tr.insertText('X', 1)
  editor.view.dispatch(tr)
}

test('同父级、同视觉层级的相邻有序列表在文档变更后合并并连续编号', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      ordered([item('A'), item('B')]),
      ordered([item('C'), item('D')]),
    ],
  })

  triggerDocumentNormalization(editor)

  const doc = editor.state.doc
  assert.equal(doc.childCount, 1, '两个相邻有序列表必须合并成一个列表块')
  assert.equal(doc.firstChild.type.name, 'orderedList')
  assert.deepEqual(
    doc.firstChild.content.content.map((node) => node.textContent),
    ['X', 'A', 'B', 'C', 'D'],
  )
  editor.destroy()
})

test('普通文档变更不能擅自把分界处更深列表挂回，避免取消列表中途被归一', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      ordered([item('A', 0)]),
      ordered([item('B', 1), item('C', 1)]),
    ],
  })

  assert.equal(editor.state.doc.childCount, 2, '前置结构应有两个列表块')
  assert.equal(editor.state.doc.child(0).lastChild.attrs.dvnIndentLevel, 0)
  assert.equal(editor.state.doc.child(1).firstChild.attrs.dvnIndentLevel, 1)

  triggerDocumentNormalization(editor)

  const doc = editor.state.doc
  assert.equal(doc.childCount, 2, '普通变更只允许合并同视觉层级列表，不能擅自改变树结构')
  assert.equal(doc.child(0).lastChild.attrs.dvnIndentLevel, 0)
  assert.equal(doc.child(1).firstChild.attrs.dvnIndentLevel, 1)
  editor.destroy()
})

test('局部取消后重应用：子列表挂回原项，外层 1..4 连续且子列表独立 1..2', () => {
  const childItem = (text) => item(text)
  const editor = createEditor({
    type: 'doc',
    content: [
      ordered([
        item('A'),
        {
          type: 'listItem',
          attrs: { dvnIndentLevel: 0 },
          content: [
            { type: 'paragraph', content: [{ type: 'text', text: 'B' }] },
            ordered([childItem('B1'), childItem('B2')]),
          ],
        },
        item('C'),
        item('D'),
      ]),
    ],
  })

  let cursor = null
  editor.state.doc.descendants((node, pos) => {
    if (cursor != null) return false
    if (node.isText && node.text === 'B') {
      cursor = pos + node.nodeSize
      return false
    }
    return true
  })
  assert.notEqual(cursor, null)
  editor.commands.setTextSelection(cursor)

  assert.equal(toggleActiveListFormatting(editor, 'orderedList'), true, '第一次点击应取消 B 的列表')
  assert.equal(editor.state.doc.child(1).type.name, 'paragraph')
  assert.equal(toggleActiveListFormatting(editor, 'orderedList'), true, '第二次点击应重新应用列表')

  const doc = editor.state.doc
  assert.equal(doc.childCount, 1, '外层列表必须重新归并成一个顶级列表')
  const outer = doc.firstChild
  assert.equal(outer.type.name, 'orderedList')
  assert.deepEqual(outer.content.content.map((node) => node.firstChild.textContent), ['A', 'B', 'C', 'D'])
  assert.deepEqual(orderedListMarkerNumbers(outer), [1, 2, 3, 4], '外层编号必须连续')
  const nested = outer.child(1).lastChild
  assert.equal(nested.type.name, 'orderedList')
  assert.deepEqual(nested.content.content.map((node) => node.firstChild.textContent), ['B1', 'B2'])
  assert.deepEqual(orderedListMarkerNumbers(nested), [1, 2], '子列表编号必须独立从 1 开始')
  assert.equal(nested.child(0).attrs.dvnIndentLevel, 0, '结构下沉后必须减掉视觉额外缩进')
  assert.equal(nested.child(1).attrs.dvnIndentLevel, 0)
  assert.equal(editor.state.selection.$from.parent.textContent, 'B', '重应用后光标必须仍在 B')
  editor.destroy()
})

test('显式结构归一：更深相邻列表挂回前一项后再合并外层列表', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      ordered([item('A'), item('B')]),
      ordered([item('B1', 1), item('B2', 1)]),
      ordered([item('C'), item('D')]),
    ],
  })
  const tr = editor.state.tr
  nestAdjacentVisualLists(tr)
  joinAdjacentSameTypeLists(tr)
  editor.view.dispatch(tr)

  const outer = editor.state.doc.firstChild
  assert.equal(editor.state.doc.childCount, 1)
  assert.deepEqual(outer.content.content.map((node) => node.firstChild.textContent), ['A', 'B', 'C', 'D'])
  assert.deepEqual(orderedListMarkerNumbers(outer), [1, 2, 3, 4])
  assert.deepEqual(orderedListMarkerNumbers(outer.child(1).lastChild), [1, 2])
  editor.destroy()
})
