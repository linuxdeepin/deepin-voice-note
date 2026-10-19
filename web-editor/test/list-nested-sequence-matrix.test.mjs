// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 列表嵌套“操作序列”矩阵。
//
// 单步矩阵只能证明每个动作单独可执行，不能证明 A -> B -> A 的结构可恢复。
// 这里专门枚举无序/有序/待办的同类型与跨类型嵌套，对每个列表项执行：
//   1. 光标：Shift+Tab -> Tab
//   2. 能实际改变层级的 Tab -> Shift+Tab
// 文本选区按 Tab 属于“替换为纯文本空格”的独立语义，由 tab-plain-text.test.mjs 覆盖。
// 并同时校验文本顺序、视觉层级、真实结构、列表项类型、待办状态和选区。
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

function listItem(text, children = [], extraAttrs = {}) {
  return {
    type: 'listItem',
    attrs: { dvnIndentLevel: 0, ...extraAttrs },
    content: [paragraph(text), ...children],
  }
}

function taskItem(text, children = [], extraAttrs = {}) {
  return {
    type: 'taskItem',
    attrs: { checked: true, dvnIndentLevel: 0, ...extraAttrs },
    content: [paragraph(text), ...children],
  }
}

function list(type, items) {
  return type === 'orderedList'
    ? { type, attrs: { start: 1 }, content: items }
    : { type, content: items }
}

function makeItem(listType, text, children = [], extraAttrs = {}) {
  return listType === 'taskList'
    ? taskItem(text, children, extraAttrs)
    : listItem(text, children, extraAttrs)
}

function nestedDoc(types, labels, options = {}) {
  const [outerType, middleType, innerType] = types
  const [outer, middle, inner] = labels
  const innerItem = makeItem(innerType, inner, [], options.innerAttrs)
  const middleItems = [
    makeItem(middleType, middle, [list(innerType, [innerItem])], options.middleAttrs),
    makeItem(middleType, `${middle}-后项`),
  ]
  const outerItems = [
    makeItem(outerType, outer, [list(middleType, middleItems)]),
    makeItem(outerType, `${outer}-后项`),
  ]
  return { type: 'doc', content: [list(outerType, outerItems)] }
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

function pressTab(editor, window, shift) {
  editor.isFocused = true
  const event = new window.KeyboardEvent('keydown', {
    key: 'Tab',
    shiftKey: shift,
    bubbles: true,
    cancelable: true,
  })
  editor.view.dom.dispatchEvent(event)
  return event.defaultPrevented
}

function listItemEntries(doc) {
  const entries = []
  const walk = (node, structuralDepth) => {
    const nodeType = node.type.name
    const isItem = nodeType === 'listItem' || nodeType === 'taskItem'
    if (isItem) {
      const ownTextblock = node.firstChild
      entries.push({
        text: ownTextblock?.textContent || '',
        visualDepth: Math.min(3, structuralDepth + (Number(node.attrs?.dvnIndentLevel) || 0)),
        structuralDepth,
        indentLevel: Number(node.attrs?.dvnIndentLevel) || 0,
        itemType: nodeType,
        checked: node.attrs?.checked,
      })
    }
    for (let index = 0; index < node.childCount; index += 1) {
      walk(node.child(index), isItem ? structuralDepth + 1 : structuralDepth)
    }
  }
  walk(doc, 1)
  return entries
}

function snapshot(editor) {
  return listItemEntries(editor.state.doc)
}

function entrySignature(entries) {
  return entries.map((entry) => (
    `${entry.text}:${entry.visualDepth}/${entry.structuralDepth}/${entry.indentLevel}/${entry.itemType}/${entry.checked}`
  )).join('|')
}

function visualSignature(entries) {
  return entries.map((entry) => (
    `${entry.text}:${entry.visualDepth}/${entry.itemType}/${entry.checked}`
  )).join('|')
}

function currentItemText(editor) {
  const { $from } = editor.state.selection
  for (let depth = $from.depth; depth > 0; depth -= 1) {
    const node = $from.node(depth)
    if (node.type.name === 'listItem' || node.type.name === 'taskItem') {
      return node.firstChild?.textContent || ''
    }
  }
  return null
}

function selectedItemTexts(editor) {
  const { from, to, empty } = editor.state.selection
  if (empty) return []
  const selected = []
  editor.state.doc.descendants((node, pos) => {
    if (node.type.name !== 'listItem' && node.type.name !== 'taskItem') return true
    const textblock = node.firstChild
    if (!textblock) return true
    const start = pos + 2
    const end = start + textblock.content.size
    if (from <= end && to >= start) selected.push(textblock.textContent)
    return true
  })
  return selected
}

function fixtureEntries(content) {
  const window = setupDom()
  const editor = createEditor(window, content)
  const entries = snapshot(editor)
  editor.destroy()
  return entries
}

function runSequence(window, content, targetText, mode, shifts) {
  const editor = createEditor(window, content)
  const range = findTextRange(editor, targetText)
  const selection = mode === 'caret'
    ? { from: range.from, to: range.from }
    : range
  editor.chain().focus().setTextSelection(selection).run()

  const beforeEntries = snapshot(editor)
  const beforeSignature = entrySignature(beforeEntries)
  const beforeSelected = selectedItemTexts(editor)
  const firstHandled = pressTab(editor, window, shifts[0])
  const afterFirstEntries = snapshot(editor)
  const afterFirstSignature = entrySignature(afterFirstEntries)
  const secondHandled = pressTab(editor, window, shifts[1])
  const afterEntries = snapshot(editor)
  const current = currentItemText(editor)
  const selected = selectedItemTexts(editor)
  editor.destroy()

  return {
    beforeEntries,
    beforeSignature,
    beforeSelected,
    firstHandled,
    afterFirstEntries,
    afterFirstSignature,
    secondHandled,
    afterEntries,
    afterSignature: entrySignature(afterEntries),
    current,
    selected,
  }
}

const SAME_TYPE_FIXTURES = ['bulletList', 'orderedList', 'taskList'].map((type) => ({
  name: `same/${type}`,
  content: nestedDoc([type, type, type], [`${type}-一级`, `${type}-二级`, `${type}-三级`]),
}))

const CROSS_TWO_LEVEL_FIXTURES = []
const CROSS_THREE_LEVEL_FIXTURES = []
for (const outerType of ['bulletList', 'orderedList', 'taskList']) {
  for (const middleType of ['bulletList', 'orderedList', 'taskList']) {
    if (middleType === outerType) continue
    CROSS_TWO_LEVEL_FIXTURES.push({
      name: `cross2/${outerType}>${middleType}`,
      content: {
        type: 'doc',
        content: [list(outerType, [
          makeItem(outerType, `${outerType}-一级`, [list(middleType, [
            makeItem(middleType, `${middleType}-二级A`),
            makeItem(middleType, `${middleType}-二级B`, [list(outerType, [
              makeItem(outerType, `${outerType}-三级B`),
            ])]),
          ])]),
          makeItem(outerType, `${outerType}-后项`),
        ])],
      },
    })
    for (const innerType of ['bulletList', 'orderedList', 'taskList']) {
      if (innerType === outerType || innerType === middleType) continue
      CROSS_THREE_LEVEL_FIXTURES.push({
        name: `cross3/${outerType}>${middleType}>${innerType}`,
        content: nestedDoc(
          [outerType, middleType, innerType],
          [`${outerType}-一级`, `${middleType}-二级`, `${innerType}-三级`],
        ),
      })
    }
  }
}

function extendedNestedDoc(types) {
  const [outerType, middleType, innerType] = types
  const outer = makeItem(outerType, `${outerType}-一级`, [list(middleType, [
    makeItem(middleType, `${middleType}-二级首项`),
    makeItem(middleType, `${middleType}-二级中间项`, [list(innerType, [
      makeItem(innerType, `${innerType}-三级首项`),
      makeItem(innerType, `${innerType}-三级中间项`),
      makeItem(innerType, `${innerType}-三级末项`),
    ])]),
    makeItem(middleType, `${middleType}-二级末项`, [list(innerType, [
      makeItem(innerType, `${innerType}-三级尾项子项`),
    ])]),
  ])])
  return {
    type: 'doc',
    content: [list(outerType, [
      makeItem(outerType, `${outerType}-一级前项`),
      outer,
      makeItem(outerType, `${outerType}-一级后项`, [list(middleType, [
        makeItem(middleType, `${middleType}-一级后项子项`),
      ])]),
    ])],
  }
}

const EXTENDED_FIXTURES = []
for (const outerType of ['bulletList', 'orderedList', 'taskList']) {
  for (const middleType of ['bulletList', 'orderedList', 'taskList']) {
    if (middleType === outerType) continue
    for (const innerType of ['bulletList', 'orderedList', 'taskList']) {
      if (innerType === outerType || innerType === middleType) continue
      EXTENDED_FIXTURES.push({
        name: `extended/${outerType}>${middleType}>${innerType}`,
        content: extendedNestedDoc([outerType, middleType, innerType]),
      })
    }
  }
}
for (const type of ['bulletList', 'orderedList', 'taskList']) {
  EXTENDED_FIXTURES.push({
    name: `extended/same/${type}`,
    content: extendedNestedDoc([type, type, type]),
  })
}

const FIXTURES = [
  ...SAME_TYPE_FIXTURES,
  ...CROSS_TWO_LEVEL_FIXTURES,
  ...CROSS_THREE_LEVEL_FIXTURES,
  ...EXTENDED_FIXTURES,
]

test('嵌套列表 Shift+Tab -> Tab：所有层级和三种列表排列必须逐项恢复', () => {
  const problems = []
  for (const fixture of FIXTURES) {
    const entries = fixtureEntries(fixture.content)
    for (const entry of entries) {
      if (entry.visualDepth <= 1) continue
      const window = setupDom()
      const result = runSequence(window, fixture.content, entry.text, 'caret', [true, false])
      const label = `[${fixture.name}/${entry.text}/caret]`
      if (!result.firstHandled) problems.push(`${label} Shift+Tab 未处理`)
      if (!result.secondHandled) problems.push(`${label} Tab 未处理`)
      if (result.afterSignature !== result.beforeSignature) {
        problems.push(`${label} 往返后结构未恢复\n  前: ${result.beforeSignature}\n  后: ${result.afterSignature}`)
      }
      if (result.current !== entry.text) {
        problems.push(`${label} 光标跑到了 ${result.current}`)
      }
      if (result.selected.length) {
        problems.push(`${label} 光标被扩成范围选区`)
      }
    }
  }
  assert.equal(problems.length, 0, `Shift+Tab -> Tab 序列矩阵失败 ${problems.length} 例：\n${problems.join('\n')}`)
})

test('嵌套列表 Tab -> Shift+Tab：当前项恢复，无子树项结构也必须完全恢复', () => {
  const problems = []
  for (const fixture of FIXTURES) {
    const entries = fixtureEntries(fixture.content)
    for (let entryIndex = 0; entryIndex < entries.length; entryIndex += 1) {
      const entry = entries[entryIndex]
      if (entry.visualDepth >= 3) continue
      const hadDescendants = hasDescendantItems(entries, entryIndex)
      const window = setupDom()
      const result = runSequence(window, fixture.content, entry.text, 'caret', [false, true])
      const label = `[${fixture.name}/${entry.text}/caret]`
      // 一级首项等没有前置兄弟项时，Tab 语义允许原地不动，不纳入往返恢复断言。
      if (!result.firstHandled || result.afterFirstSignature === result.beforeSignature) continue
      if (!result.secondHandled) problems.push(`${label} Shift+Tab 未处理`)

      const targetBefore = result.beforeEntries.find((item) => item.text === entry.text)
      const targetAfter = result.afterEntries.find((item) => item.text === entry.text)
      if (targetAfter?.visualDepth !== targetBefore?.visualDepth) {
        problems.push(
          `${label} 当前项视觉层级未恢复: ${targetBefore?.visualDepth} -> ${targetAfter?.visualDepth}`,
        )
      }
      if (!hadDescendants && result.afterSignature !== result.beforeSignature) {
        problems.push(`${label} 无子树项往返后结构未恢复\n  前: ${result.beforeSignature}\n  后: ${result.afterSignature}`)
      }
      if (result.current !== entry.text) {
        problems.push(`${label} 光标跑到了 ${result.current}`)
      }
      if (result.selected.length) {
        problems.push(`${label} 光标被扩成范围选区`)
      }
    }
  }
  assert.equal(problems.length, 0, `Tab -> Shift+Tab 序列矩阵失败 ${problems.length} 例：\n${problems.join('\n')}`)
})

function hasDescendantItems(entries, index) {
  const depth = entries[index].structuralDepth
  for (let cursor = index + 1; cursor < entries.length; cursor += 1) {
    if (entries[cursor].structuralDepth <= depth) return false
    return true
  }
  return false
}
