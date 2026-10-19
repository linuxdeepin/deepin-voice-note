// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 混合列表结构的 Shift+Tab -> Tab 必须精确可逆。
// 这里不能只比较每个列表项的文字和视觉层级，因为以下两种结构在逐项快照里完全相同：
//   orderedList(A, B, D)
//   orderedList(A, B) + orderedList(D)
// 因此测试直接比较完整 ProseMirror JSON，并同时校验光标仍在原列表项、原文本偏移。
import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'

const LIST_TYPES = ['bulletList', 'orderedList', 'taskList']

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
  const attrs = type === 'taskList'
    ? { checked: false, dvnIndentLevel: 0, ...extraAttrs }
    : { dvnIndentLevel: 0, ...extraAttrs }
  return {
    type: type === 'taskList' ? 'taskItem' : 'listItem',
    attrs,
    content: [paragraph(text), ...children],
  }
}

function list(type, items, extraAttrs = {}) {
  return {
    type,
    attrs: type === 'orderedList'
      ? { start: 1, ...extraAttrs }
      : { ...extraAttrs },
    content: items,
  }
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
  return event.defaultPrevented
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

function assertRoundTrip(fixture, target, offset, rounds = 1) {
  const window = setupDom()
  const editor = createEditor(window, fixture.content)
  const range = findTextRange(editor, target)
  editor.chain().focus().setTextSelection(range.from + offset).run()
  const before = editor.getJSON()
  const beforeOffset = editor.state.selection.$from.parentOffset

  for (let round = 1; round <= rounds; round += 1) {
    assert.equal(
      pressTab(editor, window, true),
      true,
      `[${fixture.name}/${target}/${offset}] 第 ${round} 轮 Shift+Tab 未处理`,
    )
    assert.equal(
      pressTab(editor, window, false),
      true,
      `[${fixture.name}/${target}/${offset}] 第 ${round} 轮 Tab 未处理`,
    )
    assert.deepEqual(
      editor.getJSON(),
      before,
      `[${fixture.name}/${target}/${offset}] 第 ${round} 轮往返后完整结构未恢复`,
    )
    assert.equal(
      currentItemText(editor),
      target,
      `[${fixture.name}/${target}/${offset}] 光标跑到了其他列表项`,
    )
    assert.equal(
      editor.state.selection.$from.parentOffset,
      beforeOffset,
      `[${fixture.name}/${target}/${offset}] 光标文本偏移未恢复`,
    )
  }
  editor.destroy()
}

function targetOffsets(text) {
  return [...new Set([0, Math.floor(text.length / 2), text.length])]
}

function listAttrs(type, orderedStart) {
  return type === 'orderedList' ? { start: orderedStart } : {}
}

function taskAttrs(type, checked) {
  return type === 'taskList' ? { checked } : {}
}

function makeMixedSiblingFixture(outerType, firstType, secondType) {
  const firstTarget = `${firstType}-目标首项`
  const firstTail = `${firstType}-目标次项`
  const secondTarget = `${secondType}-目标首项`
  const secondTail = `${secondType}-目标次项`
  return {
    name: `mixed-siblings/${outerType}>${firstType}+${secondType}`,
    targets: [firstTarget, firstTail, secondTarget, secondTail],
    content: {
      type: 'doc',
      content: [list(outerType, [
        item(outerType, '外层前项'),
        item(outerType, '外层容器', [
          list(firstType, [
            item(firstType, firstTarget, [], taskAttrs(firstType, true)),
            item(firstType, firstTail),
          ], listAttrs(firstType, 3)),
          list(secondType, [
            item(secondType, secondTarget, [], taskAttrs(secondType, false)),
            item(secondType, secondTail, [], taskAttrs(secondType, true)),
          ], listAttrs(secondType, 5)),
        ], taskAttrs(outerType, true)),
        item(outerType, '外层后项'),
      ], listAttrs(outerType, 4))],
    },
  }
}

const MIXED_SIBLING_FIXTURES = []
for (const outerType of LIST_TYPES) {
  const innerTypes = LIST_TYPES.filter((type) => type !== outerType)
  for (const [firstType, secondType] of [innerTypes, [...innerTypes].reverse()]) {
    MIXED_SIBLING_FIXTURES.push(makeMixedSiblingFixture(outerType, firstType, secondType))
  }
}

function makeDeepCrossFixture(outerType, parentType, childType, targetParentIndex, checkedAll) {
  const parentItems = [0, 1, 2].map((index) => {
    const isTarget = index === targetParentIndex
    return item(parentType, isTarget ? '目标父项' : `父列表第${index + 1}项`, [
      list(childType, [
        item(
          childType,
          isTarget ? '深层目标' : `第${index + 1}项子树`,
          [],
          taskAttrs(childType, checkedAll),
        ),
      ], listAttrs(childType, 6)),
    ], taskAttrs(parentType, checkedAll))
  })

  return {
    name: `deep-cross/${outerType}>${parentType}>${childType}/target-${targetParentIndex}/${checkedAll ? 'checked' : 'unchecked'}`,
    target: '深层目标',
    content: {
      type: 'doc',
      content: [list(outerType, [
        item(outerType, '外层前项'),
        item(outerType, '外层容器', [
          list(parentType, parentItems, listAttrs(parentType, 5)),
        ], taskAttrs(outerType, checkedAll)),
        item(outerType, '外层后项'),
      ], listAttrs(outerType, 4))],
    },
  }
}

const DEEP_CROSS_FIXTURES = []
for (const outerType of LIST_TYPES) {
  for (const parentType of LIST_TYPES) {
    for (const childType of LIST_TYPES) {
      const hasTaskItem = [outerType, parentType, childType].includes('taskList')
      const checkedStates = hasTaskItem ? [false, true] : [false]
      for (const targetParentIndex of [0, 1, 2]) {
        for (const checkedAll of checkedStates) {
          DEEP_CROSS_FIXTURES.push(
            makeDeepCrossFixture(outerType, parentType, childType, targetParentIndex, checkedAll),
          )
        }
      }
    }
  }
}

// 两个原始缺陷的精确最小结构，矩阵之外单独锁死，防止矩阵调整后失去针对性。
const ISSUE_379801_FIXTURES = [
  {
    name: 'issue-379801-ordered-boundary-after-mixed-lift',
    targets: ['无序子项'],
    content: {
      type: 'doc',
      content: [list('orderedList', [
        item('orderedList', '有序A'),
        item('orderedList', '有序B', [
          list('bulletList', [item('bulletList', '无序子项')]),
        ]),
        item('orderedList', '有序D'),
      ], { start: 4 })],
    },
  },
  {
    name: 'issue-mixed-adjacent-order',
    targets: ['有序目标', '待办目标'],
    content: {
      type: 'doc',
      content: [list('bulletList', [
        item('bulletList', '外层A', [
          list('orderedList', [item('orderedList', '有序目标')]),
          list('taskList', [item('taskList', '待办目标', [], { checked: true })]),
        ]),
      ])],
    },
  },
]

test('精确缺陷结构：相邻混合列表顺序与同类型列表边界必须完整恢复', () => {
  for (const fixture of ISSUE_379801_FIXTURES) {
    for (const target of fixture.targets) {
      for (const offset of targetOffsets(target)) {
        assertRoundTrip(fixture, target, offset)
      }
    }
  }
})

test('混合类型相邻列表矩阵：内外类型、先后顺序、列表项位置、列表状态与光标偏移全交叉', () => {
  for (const fixture of MIXED_SIBLING_FIXTURES) {
    for (const target of fixture.targets) {
      for (const offset of targetOffsets(target)) {
        assertRoundTrip(fixture, target, offset)
      }
    }
  }
})

test('三级列表组合矩阵：全部类型三元组、父项位置、待办状态与光标偏移全交叉', () => {
  for (const fixture of DEEP_CROSS_FIXTURES) {
    const target = fixture.target
    for (const offset of targetOffsets(target)) {
      assertRoundTrip(fixture, target, offset)
    }
  }
})

test('代表结构连续三轮往返、光标与勾选状态均不得漂移', () => {
  const samples = [
    ISSUE_379801_FIXTURES[0],
    ISSUE_379801_FIXTURES[1],
    MIXED_SIBLING_FIXTURES[0],
    MIXED_SIBLING_FIXTURES.at(-1),
    DEEP_CROSS_FIXTURES[0],
    DEEP_CROSS_FIXTURES.find((fixture) => fixture.name.endsWith('/checked')),
    DEEP_CROSS_FIXTURES.at(-1),
  ].filter(Boolean)
  for (const fixture of samples) {
    const target = fixture.target || fixture.targets[0]
    assertRoundTrip(fixture, target, Math.floor(target.length / 2), 3)
  }
})

test('反缩进后若已编辑文档，旧快照不得覆盖用户新内容', () => {
  const window = setupDom()
  const fixture = ISSUE_379801_FIXTURES[1]
  const editor = createEditor(window, fixture.content)
  const target = fixture.targets[0]
  const range = findTextRange(editor, target)
  editor.chain().focus().setTextSelection(range.from + 1).run()

  assert.equal(pressTab(editor, window, true), true)
  editor.chain().focus().insertContent('用户编辑').run()
  assert.match(editor.state.doc.textContent, /用户编辑/)
  pressTab(editor, window, false)
  assert.match(editor.state.doc.textContent, /用户编辑/, '旧往返上下文不能回滚后续输入')
  editor.destroy()
})
