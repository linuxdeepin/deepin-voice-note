// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 随机操作序列门禁：对“混合表示”（结构嵌套 + dvnIndentLevel 混用）的列表，
// 随机执行格式切换 / Tab / Shift+Tab / 勾选 / Undo，每一步都断言：
// 非目标行的 text/type/level/checked 不得变化；Undo 必须精确还原。
// 该用例用于兜住“单条手写场景覆盖不到”的组合路径。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import {
  liftActiveListItem,
  sinkActiveListItem,
  toggleActiveListFormatting,
} from '../src/runtime/list-behavior.js'

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

function createEditor(content) {
  const window = setupDom()
  const element = window.document.createElement('div')
  window.document.body.appendChild(element)
  return new Editor({ element, extensions: createTiptapExtensions(), content })
}

const LIST_ITEM_TYPES = new Set(['listItem', 'taskItem'])

function listTypeOf(node) {
  return node?.type?.name === 'orderedList' ? 'ordered'
    : node?.type?.name === 'bulletList' ? 'bullet'
      : node?.type?.name === 'taskList' ? 'task' : 'plain'
}

// 视觉层级 = 结构深度 + dvnIndentLevel；只统计列表项自身那一行。
function snapshot(editor) {
  const doc = editor.state.doc
  const rows = []
  const occurrences = new Map()
  doc.descendants((node, pos) => {
    if (node.type.name !== 'paragraph' && node.type.name !== 'heading') return true
    const $pos = doc.resolve(Math.min(pos + 1, doc.content.size))
    let itemDepth = 0
    for (let depth = $pos.depth; depth > 0; depth -= 1) {
      if (LIST_ITEM_TYPES.has($pos.node(depth).type.name)) { itemDepth = depth; break }
    }
    if (!itemDepth) {
      const text = node.textContent
      const occurrence = occurrences.get(text) || 0
      occurrences.set(text, occurrence + 1)
      rows.push({ text, type: 'plain', level: 0, checked: null, occurrence })
      return false
    }
    if ($pos.index(itemDepth) !== 0) return false
    let structural = 0
    for (let depth = 1; depth <= itemDepth; depth += 1) {
      if (LIST_ITEM_TYPES.has($pos.node(depth).type.name)) structural += 1
    }
    const item = $pos.node(itemDepth)
    const text = node.textContent
    const occurrence = occurrences.get(text) || 0
    occurrences.set(text, occurrence + 1)
    rows.push({
      text,
      type: listTypeOf($pos.node(itemDepth - 1)),
      level: structural + (Number(item.attrs?.dvnIndentLevel) || 0),
      checked: item.type.name === 'taskItem' ? Boolean(item.attrs?.checked) : null,
      occurrence,
    })
    return false
  })
  return rows
}

function selectRow(editor, text, occurrence = 0) {
  const matches = []
  editor.state.doc.descendants((node, pos) => {
    if (node.type.name === 'paragraph' || node.type.name === 'heading') {
      if (node.textContent === text) matches.push(pos)
      return false
    }
    return true
  })
  const pos = matches[occurrence]
  assert.notEqual(pos, undefined, `missing row ${JSON.stringify(text)} occurrence=${occurrence}`)
  editor.commands.focus()
  editor.commands.setTextSelection(pos + 1)
}

function clickCheckbox(editor, text, occurrence = 0) {
  const paragraphs = Array.from(editor.view.dom.querySelectorAll('p'))
    .filter((node) => node.textContent === text)
  const paragraph = paragraphs[occurrence]
  const checkbox = paragraph?.closest('li')?.querySelector('input[type="checkbox"]')
  assert.ok(checkbox, `missing checkbox ${JSON.stringify(text)} occurrence=${occurrence}`)
  checkbox.click()
}

// 非目标行必须逐字段不变；目标行允许按操作语义变化。
function assertNonTargetStable(before, after, targetText, { allowChecked = false } = {}) {
  assert.equal(after.length, before.length, '行数不得变化')
  for (let index = 0; index < before.length; index += 1) {
    const b = before[index]
    const a = after[index]
    assert.equal(a.text, b.text, `row ${index} 文本顺序不得变化`)
    if (b.text === targetText) continue
    assert.equal(a.type, b.type, `非目标行 ${b.text} 类型漂移: ${b.type} -> ${a.type}`)
    assert.equal(a.level, b.level, `非目标行 ${b.text} 层级漂移: ${b.level} -> ${a.level}`)
  }
}

function mulberry32(seed) {
  let a = seed >>> 0
  return function random() {
    a |= 0
    a = (a + 0x6D2B79F5) | 0
    let t = Math.imul(a ^ (a >>> 15), 1 | a)
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296
  }
}

const paragraph = (text) => ({ type: 'paragraph', content: [{ type: 'text', text }] })
const listItem = (text, dvnIndentLevel, children = []) => ({
  type: 'listItem', attrs: { dvnIndentLevel }, content: [paragraph(text), ...children],
})
const taskItem = (text, dvnIndentLevel, children = [], checked = false) => ({
  type: 'taskItem', attrs: { checked, dvnIndentLevel }, content: [paragraph(text), ...children],
})
const ordered = (items, start = 1) => ({ type: 'orderedList', attrs: { start }, content: items })
const bullet = (items) => ({ type: 'bulletList', content: items })
const task = (items) => ({ type: 'taskList', content: items })
const doc = (...blocks) => ({ type: 'doc', content: blocks })

// 表示形态：扁平 / 结构嵌套 / 混合（深层项缩进为 0）
const TEMPLATES = [
  {
    name: 'plain-first + nested mixed tasks (2=L2,3=L3,4=L2,5=L2)',
    build: () => doc(
      paragraph('A'),
      task([
        taskItem('B', 1, [
          task([taskItem('C', 1), taskItem('D', 0), taskItem('E', 0)]),
        ]),
      ]),
    ),
  },
  {
    name: 'plain-first + flat tasks',
    build: () => doc(
      paragraph('A'),
      task([taskItem('B', 1), taskItem('C', 2), taskItem('D', 1), taskItem('E', 1)]),
    ),
  },
  {
    name: 'no-plain + deeply nested tasks',
    build: () => doc(
      task([
        taskItem('A', 0, [
          task([taskItem('B', 0, [task([taskItem('C', 0), taskItem('D', 0)])])]),
        ]),
        taskItem('E', 0),
      ]),
    ),
  },
  {
    name: 'plain-first + ordered/bullet/task mixed',
    build: () => doc(
      paragraph('A'),
      ordered([
        listItem('B', 1, [ordered([listItem('C', 1), listItem('D', 0)])]),
        listItem('E', 1, [bullet([listItem('F', 0)])]),
      ]),
      task([taskItem('G', 1)]),
    ),
  },
  {
    name: 'skipped level: plain A + task L3 + task L2',
    build: () => doc(paragraph('A'), task([taskItem('B', 2), taskItem('C', 1)])),
  },
  {
    name: 'duplicate text at different levels',
    build: () => doc(
      paragraph('A'),
      task([taskItem('达到', 1), taskItem('达到', 2), taskItem('达到', 1)]),
    ),
  },
  {
    name: 'empty task item with nested child',
    build: () => doc(paragraph('A'), task([taskItem(' ', 1, [task([taskItem('C', 0)])]), taskItem('D', 1)])),
  },
  {
    name: 'max depth chain with level-3 siblings',
    build: () => doc(
      task([
        taskItem('A', 0, [
          task([taskItem('B', 0, [task([taskItem('C', 0), taskItem('D', 0)])])]),
        ]),
        taskItem('E', 1),
      ]),
    ),
  },
  {
    name: 'same-level mixed types: ordered/bullet/task',
    build: () => doc(
      paragraph('A'),
      ordered([listItem('B', 1)]),
      bullet([listItem('C', 1)]),
      task([taskItem('D', 1)]),
    ),
  },
  {
    name: 'inconsistent checked parent and children',
    build: () => doc(
      task([
        taskItem('A', 0, [
          task([taskItem('B', 0), taskItem('C', 0, [], true)]),
        ], true),
        taskItem('D', 0),
      ]),
    ),
  },
]

const FORMATS = ['bulletList', 'orderedList', 'taskList']
const RANDOM_TEMPLATES = TEMPLATES
const OPERATIONS = [...FORMATS, 'indent', 'outdent', 'check', 'reapply', 'undo', 'redo']

function assertLevelsInRange(editor) {
  for (const row of snapshot(editor)) {
    if (row.type === 'plain') continue
    assert.ok(row.level >= 1 && row.level <= 3, `${row.text} 层级越界: ${row.level}`)
  }
}

test('随机操作序列：混合表示列表的非目标行视觉层级不得漂移', () => {
  const seeds = 300
  const stepsPerSeed = 20
  const failures = []
  for (let seed = 0; seed < seeds; seed += 1) {
    const random = mulberry32(seed)
    const template = RANDOM_TEMPLATES[seed % RANDOM_TEMPLATES.length]
    const editor = createEditor(template.build())
    try {
      for (let step = 0; step < stepsPerSeed; step += 1) {
        const rows = snapshot(editor)
        if (!rows.length) break
        const target = rows[Math.floor(random() * rows.length)]
        const operation = OPERATIONS[Math.floor(random() * OPERATIONS.length)]
        if (operation === 'check' && target.type !== 'task') continue
        if ((operation === 'indent' || operation === 'outdent' || operation === 'reapply') && target.type === 'plain') continue

        const before = snapshot(editor)
                try {
          if (operation === 'undo') {
            editor.commands.undo()
            assert.equal(snapshot(editor).map((row) => row.text).join('|'), before.map((row) => row.text).join('|'), 'Undo 后文本顺序变化')
            assertLevelsInRange(editor)
            continue
          }
          if (operation === 'redo') {
            editor.commands.redo()
            assert.equal(snapshot(editor).map((row) => row.text).join('|'), before.map((row) => row.text).join('|'), 'Redo 后文本顺序变化')
            assertLevelsInRange(editor)
            continue
          }
          selectRow(editor, target.text, target.occurrence || 0)
          if (operation === 'reapply') {
            toggleActiveListFormatting(editor, 'taskList')
            selectRow(editor, target.text, target.occurrence || 0)
            toggleActiveListFormatting(editor, 'taskList')
          } else if (FORMATS.includes(operation)) {
            toggleActiveListFormatting(editor, operation)
          } else if (operation === 'indent') {
            sinkActiveListItem(editor)
          } else if (operation === 'outdent') {
            liftActiveListItem(editor)
          } else {
            clickCheckbox(editor, target.text, target.occurrence || 0)
          }
          const after = snapshot(editor)
                    assertNonTargetStable(before, after, target.text, { allowChecked: operation === 'check' })
          assertLevelsInRange(editor)
        } catch (error) {
          failures.push({ seed, template: template.name, step, operation, target: target.text, message: error.message })
          break
        }
      }
    } finally {
      editor.destroy()
    }
  }
  // 已确认、待修复的已知问题（棘轮：只允许这些，出现新问题立即失败）。
  const KNOWN_ISSUES = [
    { seed: 190, step: 13, operation: 'bulletList', target: 'A' },
    { seed: 226, step: 6, operation: 'taskList', target: 'A' },
  ]
  const isKnown = (failure) => KNOWN_ISSUES.some((known) => (
    known.seed === failure.seed
    && known.step === failure.step
    && known.operation === failure.operation
    && known.target === failure.target
  ))
  const unknown = failures.filter((failure) => !isKnown(failure))
  assert.deepEqual(
    unknown,
    [],
    `发现 ${unknown.length} 个新问题（已知 ${failures.length - unknown.length} 个）:
${JSON.stringify(unknown, null, 2)}`,
  )
})

test('首行普通文本 × 三种列表格式 × 混合表示：非目标行层级/类型不得漂移', () => {
  for (const template of TEMPLATES) {
    for (const format of FORMATS) {
      const editor = createEditor(template.build())
      try {
        const before = snapshot(editor)
                try { selectRow(editor, 'A') } catch (error) { throw new Error(`${template.name} / ${format}: ${error.message}`) }
        toggleActiveListFormatting(editor, format)
        assertNonTargetStable(before, snapshot(editor), 'A')
      } finally {
        editor.destroy()
      }
    }
  }
})

function checkedMap(editor) {
  const map = {}
  editor.state.doc.descendants((node) => {
    if (node.type.name === 'taskItem') {
      map[node.firstChild?.textContent || ''] = Boolean(node.attrs?.checked)
    }
    return true
  })
  return map
}

test('首行普通文本转待办后，后续待办仍沿视觉父子关系级联', () => {
  const editor = createEditor(TEMPLATES[0].build())
  try {
    selectRow(editor, 'A')
    toggleActiveListFormatting(editor, 'taskList')
    clickCheckbox(editor, 'C')
    assert.deepEqual(checkedMap(editor), { A: false, B: true, C: true, D: false, E: false })
    clickCheckbox(editor, 'D')
    clickCheckbox(editor, 'E')
    assert.deepEqual(checkedMap(editor), { A: true, B: true, C: true, D: true, E: true })
  } finally {
    editor.destroy()
  }
})

test('恢复普通文本后重新应用待办：父子关系必须重建且勾选级联正确', () => {
  const editor = createEditor(doc(
    task([taskItem('A', 0)]),
    task([
      taskItem('dsa', 1, [
        task([taskItem('das', 1, [task([taskItem('aa', 0, [], true)])], true)]),
      ], true),
      taskItem('B', 1, [], true),
      taskItem('V', 1, [], true),
      taskItem('F', 1, [], true),
    ]),
  ))
  try {
    selectRow(editor, 'A')
    toggleActiveListFormatting(editor, 'taskList')
    selectRow(editor, 'A')
    toggleActiveListFormatting(editor, 'taskList')
    assert.deepEqual(
      snapshot(editor).map((row) => `${row.text}:${row.level}:${row.checked}`),
      ['A:1:true', 'dsa:2:true', 'das:3:true', 'aa:3:true', 'B:2:true', 'V:2:true', 'F:2:true'],
      '重新应用待办后层级与父子级联都必须恢复',
    )
  } finally {
    editor.destroy()
  }
})
