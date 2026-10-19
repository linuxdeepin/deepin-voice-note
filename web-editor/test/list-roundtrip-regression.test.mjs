// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

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
  return { type: 'paragraph', content: text ? [{ type: 'text', text }] : [] }
}

function item(type, text, children = [], extraAttrs = {}) {
  const itemType = type === 'taskList' ? 'taskItem' : 'listItem'
  const attrs = itemType === 'taskItem'
    ? { checked: false, dvnIndentLevel: 0, ...extraAttrs }
    : { dvnIndentLevel: 0, ...extraAttrs }
  return { type: itemType, attrs, content: [paragraph(text), ...children] }
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
      range = { from: pos, to: pos + node.nodeSize }
      return false
    }
    return true
  })
  if (!range) throw new Error(`text not found: ${text}`)
  return range
}

function pressTab(editor, window, shift) {
  editor.isFocused = true
  const event = new window.KeyboardEvent('keydown', {
    key: 'Tab',
    shiftKey: shift,
    bubbles: true,
    cancelable: true,
  })
  editor.view.dom.dispatchEvent(event)
  return event
}

function visualLevels(node, structuralDepth = 0, acc = []) {
  if (!node || typeof node !== 'object') return acc
  const isItem = node.type === 'listItem' || node.type === 'taskItem'
  const nextDepth = isItem ? structuralDepth + 1 : structuralDepth
  if (isItem) {
    const own = node.content?.find((child) => child.type === 'paragraph')
    acc.push({
      text: own?.content?.map((child) => child.text || '').join('') || '',
      level: Math.min(3, nextDepth + (Number(node.attrs?.dvnIndentLevel) || 0)),
      structuralDepth: nextDepth,
      checked: node.attrs?.checked,
    })
  }
  node.content?.forEach((child) => visualLevels(child, nextDepth, acc))
  return acc
}

function selectedText(editor) {
  return editor.state.selection.$from.parent.textContent
}

function sameTypeFixture(type) {
  return {
    type: 'doc',
    content: [list(type, [
      item(type, '一级'),
      item(type, '父项', [list(type, [
        item(type, '二级A'),
        item(type, '二级B', [list(type, [
          item(type, '三级B'),
        ])]),
      ])]),
    ])],
  }
}

for (const type of ['bulletList', 'orderedList', 'taskList']) {
  test(`${type}: Shift+Tab 后再 Tab，只恢复 A 的层级，不连带下沉 B/C`, () => {
    const window = setupDom()
    const editor = createEditor(window, sameTypeFixture(type))
    editor.chain().focus().setTextSelection(findTextRange(editor, '二级A').from).run()

    assert.equal(pressTab(editor, window, true).defaultPrevented, true)
    assert.deepEqual(
      visualLevels(editor.getJSON()).filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
        .map((entry) => [entry.text, entry.level]),
      [['一级', 1], ['二级A', 1], ['二级B', 2], ['三级B', 3]],
      'Shift+Tab 后 A 到一级，B/C 的视觉层级必须保持',
    )

    assert.equal(pressTab(editor, window, false).defaultPrevented, true)
    assert.deepEqual(
      visualLevels(editor.getJSON()).filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
        .map((entry) => [entry.text, entry.level]),
      [['一级', 1], ['二级A', 2], ['二级B', 2], ['三级B', 3]],
      'Tab 后 A 应回到二级，B/C 不能被多降一级',
    )
    assert.deepEqual(
      visualLevels(editor.getJSON()).filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
        .map((entry) => [entry.text, entry.structuralDepth]),
      [['一级', 1], ['二级A', 2], ['二级B', 2], ['三级B', 3]],
      'B 必须和 A 真正处于同一个二级列表，不能靠缩进属性伪装层级',
    )
    assert.equal(selectedText(editor), '二级A', '操作后光标应仍在 A')
    editor.destroy()
  })
}

test('A 自身已有子项：Shift+Tab 后再 Tab，A1 仍属于 A，B/C 不被误挂到 A 下', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      item('bulletList', '一级'),
      item('bulletList', '父项', [list('bulletList', [
        item('bulletList', '二级A', [list('bulletList', [
          item('bulletList', '三级A1'),
        ])]),
        item('bulletList', '二级B', [list('bulletList', [
          item('bulletList', '三级B'),
        ])]),
      ])]),
    ])],
  })
  editor.chain().focus().setTextSelection(findTextRange(editor, '二级A').from).run()

  pressTab(editor, window, true)
  pressTab(editor, window, false)

  assert.deepEqual(
    visualLevels(editor.getJSON()).filter((entry) => ['一级', '二级A', '三级A1', '二级B', '三级B'].includes(entry.text))
      .map((entry) => [entry.text, entry.level]),
    [['一级', 1], ['二级A', 2], ['三级A1', 3], ['二级B', 2], ['三级B', 3]],
  )
  assert.deepEqual(
    visualLevels(editor.getJSON()).filter((entry) => ['一级', '二级A', '三级A1', '二级B', '三级B'].includes(entry.text))
      .map((entry) => [entry.text, entry.structuralDepth]),
    [['一级', 1], ['二级A', 2], ['三级A1', 3], ['二级B', 2], ['三级B', 3]],
  )
  assert.equal(selectedText(editor), '二级A')
  editor.destroy()
})

test('跨类型嵌套：无序 > 有序，A 反缩进后仍能 Tab 回到二级且 B/C 层级不变', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      item('bulletList', '一级', [list('orderedList', [
        item('orderedList', '二级A'),
        item('orderedList', '二级B', [list('bulletList', [
          item('bulletList', '三级B'),
        ])]),
      ])]),
    ])],
  })
  editor.chain().focus().setTextSelection(findTextRange(editor, '二级A').from).run()

  assert.equal(pressTab(editor, window, true).defaultPrevented, true)
  const afterLift = editor.getJSON()
  assert.deepEqual(
    visualLevels(afterLift).filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
      .map((entry) => [entry.text, entry.level]),
    [['一级', 1], ['二级A', 1], ['二级B', 2], ['三级B', 3]],
    JSON.stringify(afterLift),
  )

  assert.equal(pressTab(editor, window, false).defaultPrevented, true)
  const afterSink = editor.getJSON()
  assert.deepEqual(
    visualLevels(afterSink).filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
      .map((entry) => [entry.text, entry.level]),
    [['一级', 1], ['二级A', 2], ['二级B', 2], ['三级B', 3]],
    JSON.stringify(afterSink),
  )
  assert.deepEqual(
    visualLevels(afterSink).filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
      .map((entry) => [entry.text, entry.structuralDepth]),
    [['一级', 1], ['二级A', 2], ['二级B', 2], ['三级B', 3]],
  )
  assert.equal(selectedText(editor), '二级A')
  editor.destroy()
})

for (const type of ['bulletList', 'orderedList', 'taskList']) {
  test(`${type}: Shift+Tab / Tab 连续往返三轮不漂移`, () => {
    const window = setupDom()
    const editor = createEditor(window, sameTypeFixture(type))
    editor.chain().focus().setTextSelection(findTextRange(editor, '二级A').from).run()

    for (let round = 0; round < 3; round += 1) {
      pressTab(editor, window, true)
      pressTab(editor, window, false)
      assert.deepEqual(
        visualLevels(editor.getJSON()).filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
          .map((entry) => [entry.text, entry.level]),
        [['一级', 1], ['二级A', 2], ['二级B', 2], ['三级B', 3]],
        `第 ${round + 1} 轮往返后层级漂移`,
      )
      assert.equal(selectedText(editor), '二级A', `第 ${round + 1} 轮后光标应仍在 A`)
    }
    editor.destroy()
  })
}

test('待办 Shift+Tab -> Tab 往返后 checked 状态完整恢复', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('taskList', [
      item('taskList', '一级'),
      item('taskList', '父项', [list('taskList', [
        item('taskList', '二级A'),
        item('taskList', '二级B', [list('taskList', [
          item('taskList', '三级B', [], { checked: true }),
        ])], { checked: true }),
      ])]),
    ])],
  })
  editor.chain().focus().setTextSelection(findTextRange(editor, '二级A').from).run()
  const beforeChecks = Object.fromEntries(
    visualLevels(editor.getJSON())
      .filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
      .map((entry) => [entry.text, entry.checked]),
  )

  pressTab(editor, window, true)
  pressTab(editor, window, false)

  const afterSinkChecks = Object.fromEntries(
    visualLevels(editor.getJSON())
      .filter((entry) => ['一级', '二级A', '二级B', '三级B'].includes(entry.text))
      .map((entry) => [entry.text, entry.checked]),
  )
  assert.deepEqual(afterSinkChecks, beforeChecks)
  editor.destroy()
})
