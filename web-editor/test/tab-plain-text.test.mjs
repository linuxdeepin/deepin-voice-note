// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tab 的纯文本语义：
// 1. 普通段落行首、空段落：插入 4 个 NBSP；
// 2. 任意文本选区：删除选区后插入 4 个 NBSP；
// 3. 列表内空光标：仍执行列表缩进。
import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { AllSelection } from '@tiptap/pm/state'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'

const PLAIN_TAB = '\u00a0'.repeat(4)

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
  return { type: 'paragraph', content: text ? [{ type: 'text', text }] : [] }
}

function listItem(text, children = [], attrs = {}) {
  return {
    type: 'listItem',
    attrs: { dvnIndentLevel: 0, ...attrs },
    content: [paragraph(text), ...children],
  }
}

function taskItem(text, children = [], attrs = {}) {
  return {
    type: 'taskItem',
    attrs: { checked: false, dvnIndentLevel: 0, ...attrs },
    content: [paragraph(text), ...children],
  }
}

function list(type, items) {
  return type === 'orderedList'
    ? { type, attrs: { start: 1 }, content: items }
    : { type, content: items }
}

function createEditor(window, content) {
  const element = window.document.createElement('div')
  window.document.body.appendChild(element)
  return new Editor({ element, extensions: createTiptapExtensions(), content })
}

function findTextRange(editor, text) {
  let range = null
  editor.state.doc.descendants((node, pos) => {
    if (range) return false
    if (node.isText && node.text === text) {
      range = { from: pos, to: pos + node.text.length }
      return false
    }
    return true
  })
  if (!range) throw new Error(`text not found: ${text}`)
  return range
}

function pressTab(editor, window) {
  editor.isFocused = true
  const event = new window.KeyboardEvent('keydown', {
    key: 'Tab',
    bubbles: true,
    cancelable: true,
  })
  editor.view.dom.dispatchEvent(event)
  return event
}

test('普通段落行首按 Tab 插入 4 个 NBSP，光标移动到空格之后', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [paragraph('普通文本')],
  })
  const range = findTextRange(editor, '普通文本')
  editor.chain().focus().setTextSelection(range.from).run()

  const event = pressTab(editor, window)
  assert.equal(event.defaultPrevented, true, 'Tab 必须被编辑器接管，不能移走焦点')
  assert.equal(editor.state.doc.textContent, `${PLAIN_TAB}普通文本`)
  assert.equal(editor.state.doc.firstChild.type.name, 'paragraph', '不得把普通段落转成列表')
  assert.equal(editor.state.selection.empty, true)
  assert.equal(editor.state.selection.from, range.from + PLAIN_TAB.length)
  editor.destroy()
})

test('空段落按 Tab 插入 4 个 NBSP，形成纯文本缩进', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [paragraph('')],
  })
  editor.chain().focus().setTextSelection(1).run()

  const event = pressTab(editor, window)
  assert.equal(event.defaultPrevented, true)
  assert.equal(editor.state.doc.textContent, PLAIN_TAB)
  assert.equal(editor.state.doc.firstChild.type.name, 'paragraph')
  assert.equal(editor.state.selection.from, 1 + PLAIN_TAB.length)
  editor.destroy()
})

test('普通段落行中按 Tab 不插入缩进，保留默认焦点行为', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [paragraph('普通文本')],
  })
  const range = findTextRange(editor, '普通文本')
  editor.chain().focus().setTextSelection(range.from + 2).run()
  const before = editor.getJSON()

  const event = pressTab(editor, window)
  assert.equal(event.defaultPrevented, false)
  assert.deepEqual(editor.getJSON(), before)
  editor.destroy()
})

test('列表内空光标按 Tab 仍执行列表缩进，不插入 NBSP', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('第一项'),
      listItem('第二项'),
    ])],
  })
  const range = findTextRange(editor, '第二项')
  editor.chain().focus().setTextSelection(range.from).run()

  const event = pressTab(editor, window)
  const first = editor.getJSON().content[0].content[0]
  const nestedList = first.content.find((child) => child.type === 'bulletList')
  assert.equal(event.defaultPrevented, true)
  assert.equal(editor.state.doc.textContent.includes(PLAIN_TAB), false)
  assert.ok(nestedList, '第二项应缩进到第一项下面')
  assert.equal(nestedList.content[0].content[0].content[0].text, '第二项')
  editor.destroy()
})

const LIST_CASES = [
  {
    name: '无序列表',
    listType: 'bulletList',
    makeItem: (text, children, attrs) => listItem(text, children, attrs),
  },
  {
    name: '有序列表',
    listType: 'orderedList',
    makeItem: (text, children, attrs) => listItem(text, children, attrs),
  },
  {
    name: '待办列表',
    listType: 'taskList',
    makeItem: (text, children, attrs) => taskItem(text, children, attrs),
  },
]

for (const spec of LIST_CASES) {
  test(`${spec.name}选中文本按 Tab 替换为 4 个 NBSP，不改变列表状态`, () => {
    const window = setupDom()
    const target = 'A-B-C-D'
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(spec.listType, [
        spec.makeItem('父项', [
          list(spec.listType, [spec.makeItem(target, [], { checked: true })]),
        ]),
      ])],
    })
    const before = editor.getJSON()
    const range = findTextRange(editor, target)
    editor.chain().focus().setTextSelection({ from: range.from + 2, to: range.from + 4 }).run()

    const event = pressTab(editor, window)
    const after = editor.getJSON()
    const nestedItem = after.content[0].content[0].content[1].content[0]
    assert.equal(event.defaultPrevented, true)
    assert.equal(nestedItem.content[0].content[0].text, `A-${PLAIN_TAB}C-D`)
    assert.equal(nestedItem.type, spec.listType === 'taskList' ? 'taskItem' : 'listItem')
    assert.equal(nestedItem.attrs.checked, spec.listType === 'taskList' ? true : undefined)
    assert.equal(after.content[0].type, spec.listType, '列表类型不能变化')
    assert.equal(editor.state.selection.empty, true)

    editor.commands.undo()
    assert.deepEqual(editor.getJSON(), before, '一次撤销必须恢复原文和列表状态')
    editor.destroy()
  })
}

test('Ctrl+A 全选后按 Tab 仍替换为纯文本空格，不执行列表缩进', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('taskList', [
      taskItem('父项', [list('taskList', [taskItem('子项', [], { checked: true })])], { checked: true }),
    ])],
  })
  editor.chain().focus().selectAll().run()
  assert.equal(editor.state.selection instanceof AllSelection, true)

  const event = pressTab(editor, window)
  assert.equal(event.defaultPrevented, true)
  assert.equal(editor.state.doc.textContent, PLAIN_TAB)
  assert.equal(editor.state.selection.empty, true)
  editor.destroy()
})
