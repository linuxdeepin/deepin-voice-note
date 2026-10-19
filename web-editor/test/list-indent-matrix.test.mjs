// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 列表缩进 / 反缩进的组合矩阵回归。
//
// 单点用例挡不住"修东边冒西边"，这里把所有结构形态 × 所有选区 × 两个方向
// 全量枚举，统一校验不变量：
//   1. 文本顺序不变、不丢项
//   2. 未选中的项视觉层级不变
//   3. 选中项最多各降/升一级，且不能反向
//   4. 选中的项不能有"该动却没动"的
//   5. 操作后选区不能消失
import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { liftActiveListItem, sinkActiveListItem } from '../src/runtime/list-behavior.js'

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
function listItem(text, extra = [], indentLevel = 0) {
  return { type: 'listItem', attrs: { dvnIndentLevel: indentLevel }, content: [paragraph(text), ...extra] }
}
function taskItem(text, extra = [], indentLevel = 0) {
  return { type: 'taskItem', attrs: { checked: false, dvnIndentLevel: indentLevel }, content: [paragraph(text), ...extra] }
}
function list(type, items) {
  return type === 'orderedList' ? { type, attrs: { start: 1 }, content: items } : { type, content: items }
}

function createEditor(window, content) {
  const element = window.document.createElement('div')
  window.document.body.appendChild(element)
  return new Editor({ element, extensions: createTiptapExtensions(), content })
}

// 文档顺序枚举所有"块"：列表项（含视觉层级）与被反缩进踢出列表的普通段落。
//
// 关键：一级项反缩进会**退出列表变成普通段落**，如果只数 listItem，就会把
// "A 掉出列表"误判成"文本顺序变了、A 丢了"，产生假阳性。这里统一按顶层块
// 顺序枚举，段落记为 visualDepth 0，并按 (文本, 第几次出现) 做身份匹配。
function enumerateBlocks(node, structuralDepth = 0, acc = []) {
  if (!node || typeof node !== 'object') return acc
  const kind = node.type
  if (kind === 'listItem' || kind === 'taskItem') {
    const own = node.content?.find((child) => child.type === 'paragraph')
    acc.push({
      blockKind: 'item',
      text: own?.content?.[0]?.text ?? '',
      visualDepth: Math.min(3, structuralDepth + 1 + (Number(node.attrs?.dvnIndentLevel) || 0)),
    })
    if (Array.isArray(node.content)) {
      node.content.forEach((child) => {
        if (child.type === 'bulletList' || child.type === 'orderedList' || child.type === 'taskList') {
          enumerateBlocks(child, structuralDepth + 1, acc)
        }
      })
    }
    return acc
  }
  if (kind === 'paragraph') {
    acc.push({
      blockKind: 'paragraph',
      text: node.content?.[0]?.text ?? '',
      visualDepth: 0,
    })
    return acc
  }
  if (Array.isArray(node.content)) {
    node.content.forEach((child) => enumerateBlocks(child, structuralDepth, acc))
  }
  return acc
}

// 与上面同口径，但直接走 ProseMirror 节点，顺便带出"块自身文本"的位置区间，
// 用于校验变换后选区端点是否仍然完整覆盖这批块（只查"非空"会漏掉端点缩水）。
function enumerateDocBlocks(doc) {
  const acc = []
  const walk = (node, pos, structuralDepth, insideItem) => {
    const isItem = node.type.name === 'listItem' || node.type.name === 'taskItem'
    if (isItem) {
      const textblockPos = pos + 1
      const textblock = doc.nodeAt(textblockPos)
      const textStart = textblockPos + 1
      acc.push({
        blockKind: 'item',
        text: textblock ? textblock.textContent : '',
        visualDepth: Math.min(3, structuralDepth + 1 + (Number(node.attrs?.dvnIndentLevel) || 0)),
        textStart,
        textEnd: textStart + (textblock ? textblock.content.size : 0),
      })
    } else if (!insideItem && node.isTextblock) {
      acc.push({
        blockKind: 'paragraph',
        text: node.textContent,
        visualDepth: 0,
        textStart: pos + 1,
        textEnd: pos + 1 + node.content.size,
      })
    }
    let offset = pos + 1
    for (let index = 0; index < node.childCount; index += 1) {
      const child = node.child(index)
      walk(child, offset, isItem ? structuralDepth + 1 : structuralDepth, insideItem || isItem)
      offset += child.nodeSize
    }
  }
  walk(doc, -1, 0, false)
  return acc
}

// 用 (文本, 出现序号) 给块编号，重复文本也能对上号。
function blockKeys(blocks) {
  const seen = new Map()
  return blocks.map((block) => {
    const nth = seen.get(block.text) ?? 0
    seen.set(block.text, nth + 1)
    return `${block.text}#${nth}`
  })
}

function textStartPos(editor, index) {
  let seen = -1
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (found !== null) return false
    if (node.isTextblock) {
      seen += 1
      if (seen === index) { found = pos + 1; return false }
    }
    return true
  })
  if (found === null) throw new Error(`textblock ${index} not found`)
  return found
}

function textEndPos(editor, index) {
  let seen = -1
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (found !== null) return false
    if (node.isTextblock) {
      seen += 1
      if (seen === index) { found = pos + node.nodeSize - 1; return false }
    }
    return true
  })
  if (found === null) throw new Error(`textblock ${index} not found`)
  return found
}

// ---- 结构形态库 -----------------------------------------------------------
// 结构形态库按"列表类型"参数化：同一个结构要在 无序 / 有序 / 待办 三种顶层列表
// 各跑一遍，再叠三种列表互相嵌套的交叉结构。只跑无序会漏掉整整两类列表。
//
// mkItem 造列表项（listItem / taskItem），mkList 造同级子列表。
function buildStructures(mkItem, mkList) {
  return {
    'flat-4': [mkItem('A'), mkItem('B'), mkItem('C'), mkItem('D')],
    'nested-1': [mkItem('A', [mkList([mkItem('B'), mkItem('C'), mkItem('D')])]), mkItem('E')],
    'nested-2': [mkItem('A', [mkList([mkItem('B', [mkList([mkItem('C'), mkItem('D')])])])]), mkItem('E')],
    'visual-12': [mkItem('A'), mkItem('B', [], 1), mkItem('C', [], 2), mkItem('D', [], 1), mkItem('E')],
    'visual-sibling': [mkItem('A'), mkItem('B', [], 1), mkItem('C', [], 1), mkItem('D', [], 1), mkItem('E')],
    'mixed-struct+visual': [
      mkItem('A', [mkList([mkItem('B')])]),
      mkItem('C', [], 1),
      mkItem('D', [], 1),
      mkItem('E'),
    ],
    'mixed-visual+struct': [
      mkItem('A', [mkList([mkItem('B'), mkItem('C')])]),
      mkItem('D', [], 1),
      mkItem('E'),
    ],
    'nested-then-visual': [
      mkItem('A', [mkList([mkItem('B'), mkItem('C')])]),
      mkItem('D', [], 1),
      mkItem('E', [], 1),
      mkItem('F'),
    ],
    'visual-3level': [mkItem('A'), mkItem('B', [], 1), mkItem('C', [], 2), mkItem('D', [], 2), mkItem('E', [], 1)],
    'flat-with-empty': [mkItem('A'), mkItem(''), mkItem(''), mkItem('D')],
    'nested-with-empty': [mkItem('A', [mkList([mkItem('B'), mkItem(''), mkItem('D')])]), mkItem('E')],
    'visual-with-empty': [mkItem('A'), mkItem('', [], 1), mkItem('', [], 1), mkItem('E')],
    'mixed-task': [
      mkItem('A', [list('taskList', [taskItem('B'), taskItem('C')])]),
      mkItem('D'),
    ],
  }
}

// 三种顶层列表类型（列表项工厂 + 子列表类型）。
const LIST_TYPES = [
  { label: '无序', listType: 'bulletList', mkItem: listItem },
  { label: '有序', listType: 'orderedList', mkItem: listItem },
  { label: '待办', listType: 'taskList', mkItem: taskItem },
]

// 交叉嵌套：三种列表互相包含（外层 / 中层 / 内层各不同）。
const CROSS_STRUCTURES = {
  'cross-ul-ol-task': [list('bulletList', [
    listItem('A', [list('orderedList', [
      listItem('B', [list('taskList', [taskItem('C'), taskItem('D')])]),
    ])]),
    listItem('E'),
  ])],
  'cross-task-ol-ul': [list('taskList', [
    taskItem('A', [list('orderedList', [
      listItem('B', [list('bulletList', [listItem('C'), listItem('D')])]),
    ])]),
    taskItem('E'),
  ])],
  'cross-ol-task-ul-visual': [list('orderedList', [
    listItem('A', [list('taskList', [
      taskItem('B', [list('bulletList', [listItem('C')])]),
      taskItem('D', [], 1),
    ])]),
    listItem('E', [], 1),
    listItem('F'),
  ])],
}

// 单类型结构 × 3 种列表类型。
const STRUCTURES = {}
const STRUCTURE_TYPES = []
for (const { label, listType, mkItem } of LIST_TYPES) {
  const built = buildStructures(mkItem, (items) => list(listType, items))
  for (const [name, content] of Object.entries(built)) {
    const key = `${label}/${name}`
    STRUCTURES[key] = { content, listType }
    STRUCTURE_TYPES.push(key)
  }
}
// 交叉嵌套结构（外层类型随结构自带）。
for (const [name, content] of Object.entries(CROSS_STRUCTURES)) {
  STRUCTURES[`交叉/${name}`] = { content, listType: null }
  STRUCTURE_TYPES.push(`交叉/${name}`)
}

function topLevelList(listType, content) {
  // listType 为 null（交叉结构）时 content 已经是完整列表节点。
  return listType ? list(listType, content) : content[0]
}

function analyze(before, after, selectedIndexes, direction) {
  const problems = []
  const beforeKeys = blockKeys(before)
  const afterKeys = blockKeys(after)
  if (beforeKeys.join('|') !== afterKeys.join('|')) {
    problems.push(`文本顺序/内容变化: ${before.map((b) => b.text).join(',')} -> ${after.map((a) => a.text).join(',')}`)
    return problems
  }

  for (let i = 0; i < before.length; i += 1) {
    const isSelected = selectedIndexes.has(i)
    const delta = after[i].visualDepth - before[i].visualDepth
    if (!isSelected && delta !== 0) {
      problems.push(`未选中项 ${before[i].text} 视觉层级被改: ${before[i].visualDepth} -> ${after[i].visualDepth}`)
    }
    if (direction === 'out' && delta > 0) {
      problems.push(`反缩进时 ${before[i].text} 反向变深: ${before[i].visualDepth} -> ${after[i].visualDepth}`)
    }
    if (direction === 'in' && delta < 0) {
      problems.push(`缩进时 ${before[i].text} 反向变浅: ${before[i].visualDepth} -> ${after[i].visualDepth}`)
    }
    if (Math.abs(delta) > 1) {
      problems.push(`${before[i].text} 一次变化超过一级: ${before[i].visualDepth} -> ${after[i].visualDepth}`)
    }
    // 反缩进时选中的项必须"各降一级"：命中这条的就是"选中了却没动"
    // （跨类型嵌套的祖先/子孙同时被选中时最容易踩：子孙的视觉层级被保持逻辑写回）。
    if (direction === 'out' && isSelected && delta !== -1) {
      problems.push(`选中项 ${before[i].text} 未按"各降一级"处理: ${before[i].visualDepth} -> ${after[i].visualDepth}`)
    }
  }
  return problems
}

const FAILURES = []

for (const [name, spec] of Object.entries(STRUCTURES)) {
  const { content, listType } = spec
  const window = setupDom()
  const probe = createEditor(window, { type: 'doc', content: [topLevelList(listType, content)] })
  const total = enumerateBlocks(probe.getJSON()).length
  probe.destroy()

  for (const direction of ['out', 'in']) {
    // 选区：[i, j] 覆盖文档顺序的第 i..j 个列表项
    const ranges = []
    for (let i = 0; i < total; i += 1) {
      for (let j = i; j < total; j += 1) ranges.push([i, j])
    }
    for (const [i, j] of ranges) {
      const editor = createEditor(window, { type: 'doc', content: [topLevelList(listType, content)] })
      const before = enumerateDocBlocks(editor.state.doc)
      const beforeKeys = blockKeys(before)
      // 跳过最外层同名容器带来的索引歧义：用 textblock 索引即列表项本身
      editor.chain().focus().setTextSelection({ from: textStartPos(editor, i), to: textEndPos(editor, j) }).run()
      // 空项的“块自身文本”是零宽的，选它本身就等于折叠光标（光标场景），
      // 这类组合不能按“范围选区”校验端点。
      const selectionWasEmpty = editor.state.selection.empty
      const selected = new Set()
      for (let k = i; k <= j; k += 1) selected.add(k)

      const handled = direction === 'out' ? liftActiveListItem(editor) : sinkActiveListItem(editor)
      const after = enumerateDocBlocks(editor.state.doc)
      const problems = analyze(before, after, selected, direction)
      if (handled && selectionWasEmpty) {
        // 光标场景：操作后必须仍然是光标，不能被扩成范围
        if (!editor.state.selection.empty) {
          problems.push('光标场景被扩成范围选区')
        }
      }
      if (handled && !selectionWasEmpty) {
        const { from, to, empty } = editor.state.selection
        // 变换后选区不应消失（否则无法连续按 Shift+Tab / Tab）
        if (empty) {
          problems.push('变换后选区消失')
        } else {
          const afterKeys = blockKeys(after)
          const firstEntry = after[afterKeys.indexOf(beforeKeys[i])]
          const lastEntry = after[afterKeys.indexOf(beforeKeys[j])]
          // 只查"非空"会漏掉端点缩水（用户现象：第一项整段高亮，后面几项只剩一条细缝）
          if (firstEntry && from > firstEntry.textStart) {
            problems.push(`选区起点晚于首个选中项文本起点: ${from} > ${firstEntry.textStart}`)
          }
          if (lastEntry && to < lastEntry.textEnd) {
            problems.push(`选区终点早于最后一个选中项文本终点: ${to} < ${lastEntry.textEnd}`)
          }
          // 选区必须恰好盖住这批项自身的文本，不能缩水也不能连带选中没选的项。
          const covered = new Set()
          after.forEach((entry, index) => {
            if (from <= entry.textEnd && to >= entry.textStart) covered.add(afterKeys[index])
          })
          const expected = new Set(beforeKeys.slice(i, j + 1))
          for (const key of expected) {
            if (!covered.has(key)) problems.push(`选区漏掉选中项 ${key}`)
          }
          for (const key of covered) {
            if (!expected.has(key)) problems.push(`选区多盖了未选中项 ${key}`)
          }
        }
      }
      if (problems.length) {
        FAILURES.push(`[${name}/${direction}/选中 ${i}..${j}] ${problems.join(' ; ')}`)
      }
      editor.destroy()
    }
  }
}

// ---- 方案 A 不变量 1：AllSelection（Ctrl+A）与拖选全文必须完全等价 -------------
//
// 回归背景：全选后 $from 落在文档首块，第一按 Shift+Tab 会把首块踢成普通段落，
// 之后 $from 就在段落里了。旧实现拿 isInListItem($from) 当入口门槛，第二按起
// 全部返回 false（用户现象：连按无反应、只有主动再点一次才动）。
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

function dispatchTab(editor, window, shift) {
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

function imageNode(relPath = 'images/a.png') {
  return {
    type: 'image',
    attrs: {
      src: `file:///x/${relPath}`,
      relPath,
      alt: '',
      title: null,
    },
  }
}

function imageListItem(itemType, relPath, text = '', indentLevel = 0) {
  const attrs = itemType === 'taskItem'
    ? { checked: false, dvnIndentLevel: indentLevel }
    : { dvnIndentLevel: indentLevel }
  const content = [{ type: 'paragraph', content: [
    ...(text ? [{ type: 'text', text }] : []),
    imageNode(relPath),
  ] }]
  return { type: itemType, attrs, content }
}

function imageListDoc(listType, relPaths) {
  const itemType = listType === 'taskList' ? 'taskItem' : 'listItem'
  return {
    type: 'doc',
    content: [{
      type: listType,
      attrs: listType === 'orderedList' ? { start: 1 } : {},
      content: relPaths.map((relPath, index) => imageListItem(itemType, relPath, `item-${index + 1}`)),
    }],
  }
}

function imagePositions(editor, relPath) {
  const positions = []
  editor.state.doc.descendants((node, pos) => {
    if (node.type.name === 'image' && node.attrs?.relPath === relPath) positions.push(pos)
    return true
  })
  return positions
}

function selectImage(editor, relPath, occurrence = 0) {
  const positions = imagePositions(editor, relPath)
  assert.ok(positions.length > occurrence, `没有找到第 ${occurrence + 1} 张图片 ${relPath}`)
  assert.equal(editor.commands.setNodeSelection(positions[occurrence]), true)
  assert.equal(editor.state.selection.node?.type.name, 'image')
}

function selectedImageOccurrence(editor, relPath) {
  let occurrence = 0
  let selected = -1
  editor.state.doc.descendants((node, pos) => {
    if (node.type.name !== 'image' || node.attrs?.relPath !== relPath) return true
    if (pos === editor.state.selection.from) selected = occurrence
    occurrence += 1
    return true
  })
  return selected
}

function selectedImageVisualDepth(editor) {
  const $pos = editor.state.doc.resolve(editor.state.selection.from)
  let structuralDepth = 0
  let item = null
  for (let depth = 1; depth <= $pos.depth; depth += 1) {
    const node = $pos.node(depth)
    if (node.type.name === 'listItem' || node.type.name === 'taskItem') {
      structuralDepth += 1
      item = node
    }
  }
  if (!item) return 0
  return structuralDepth + (Number(item.attrs?.dvnIndentLevel) || 0)
}

function hasListNodes(node) {
  if (!node || typeof node !== 'object') return false
  if (node.type === 'bulletList' || node.type === 'orderedList' || node.type === 'taskList') return true
  return Array.isArray(node.content) && node.content.some(hasListNodes)
}

function selectionCoverage(editor) {
  return blockKeys(enumerateDocBlocks(editor.state.doc)).length === 0
    ? ''
    : editor.state.doc.textBetween(editor.state.selection.from, editor.state.selection.to, '\n')
}

test('方案A：Ctrl+A 全选与拖选全文，Shift+Tab / Tab 结果必须完全一致', () => {
  const problems = []
  for (const [name, spec] of Object.entries(STRUCTURES)) {
    const { content, listType } = spec
    for (const direction of ['out', 'in']) {
      const window = setupDom()
      const results = []
      for (const mode of ['all', 'drag']) {
        const editor = createEditor(window, { type: 'doc', content: [topLevelList(listType, content)] })
        if (mode === 'all') {
          editor.chain().focus().selectAll().run()
        } else {
          editor.chain().focus().setTextSelection({ from: 1, to: editor.state.doc.content.size - 1 }).run()
        }
        const event = dispatchTab(editor, window, direction === 'out')
        results.push({
          handled: event.defaultPrevented,
          json: JSON.stringify(editor.getJSON()),
          coverage: selectionCoverage(editor),
          empty: editor.state.selection.empty,
        })
        editor.destroy()
      }
      const [all, drag] = results
      if (all.handled !== drag.handled) {
        problems.push(`[${name}/${direction}] 处理结果不一致：Ctrl+A=${all.handled} 拖选=${drag.handled}`)
      }
      if (all.json !== drag.json) {
        problems.push(`[${name}/${direction}] 文档结果不一致`)
      }
      if (all.coverage !== drag.coverage || all.empty !== drag.empty) {
        problems.push(`[${name}/${direction}] 选区结果不一致`)
      }
    }
  }
  assert.equal(problems.length, 0, `Ctrl+A / 拖选全文等价性失败 ${problems.length} 例：\n${problems.join('\n')}`)
})

// ---- 方案 A 不变量 2：多级阶梯连按 Shift+Tab，必须逐级降到底 ------------------
//
// 每一按只允许各块降一级；文本顺序、块集合、选区覆盖集合都不能变；一直按到
// 没有任何列表项为止，文档里不能残留列表节点。
const LADDERS = {
  'ladder-3level-task': [list('taskList', [
    taskItem('T1A', [list('taskList', [
      taskItem('T2A', [list('taskList', [taskItem('T3A'), taskItem('T3B')])]),
    ])]),
    taskItem('T1B'),
  ])],
  'ladder-cross-ul-ol-task': [list('bulletList', [
    listItem('C1A', [list('orderedList', [
      listItem('C2A', [list('taskList', [taskItem('C3A'), taskItem('C3B')])]),
    ])]),
    listItem('C1B'),
  ])],
  'ladder-cross-task-ol-ul': [list('taskList', [
    taskItem('D1A', [list('orderedList', [
      listItem('D2A', [list('bulletList', [listItem('D3A'), listItem('D3B')])]),
    ])]),
    taskItem('D1B'),
  ])],
  'ladder-3level': [list('bulletList', [
    listItem('L1A', [list('bulletList', [
      listItem('L2A', [list('bulletList', [listItem('L3A'), listItem('L3B')])]),
    ])]),
    listItem('L1B'),
  ])],
  'ladder-3level-ordered': [list('orderedList', [
    listItem('O1A', [list('orderedList', [
      listItem('O2A', [list('orderedList', [listItem('O3A'), listItem('O3B')])]),
    ])]),
    listItem('O1B'),
  ])],
  'ladder-mixed-visual': [list('bulletList', [
    listItem('M1A', [list('bulletList', [listItem('M2A', [], 1), listItem('M2B')])]),
    listItem('M1B', [], 1),
    listItem('M1C'),
  ])],
  'ladder-user-report': [list('bulletList', [
    listItem('大萨达撒', [list('bulletList', [
      listItem('大萨达', [list('bulletList', [listItem('大萨达'), listItem('啊实打实')])]),
    ])]),
    listItem('阿萨德'),
  ])],
}

test('方案A：多级阶梯连按 Shift+Tab 必须逐级降到底、不丢项、不反向', () => {
  const problems = []
  for (const [name, content] of Object.entries(LADDERS)) {
    for (const mode of ['all', 'drag']) {
      const window = setupDom()
      const editor = createEditor(window, { type: 'doc', content })
      const baselineKeys = blockKeys(enumerateDocBlocks(editor.state.doc))
      let previous = enumerateDocBlocks(editor.state.doc)
      let presses = 0

      while (presses < 8) {
        if (mode === 'all') {
          editor.chain().focus().selectAll().run()
        } else {
          editor.chain().focus().setTextSelection({ from: 1, to: editor.state.doc.content.size - 1 }).run()
        }
        const event = dispatchTab(editor, window, true)
        if (!event.defaultPrevented) break
        presses += 1

        const after = enumerateDocBlocks(editor.state.doc)
        const afterKeys = blockKeys(after)
        if (afterKeys.join('|') !== baselineKeys.join('|')) {
          problems.push(`[${name}/${mode}/第${presses}按] 文本顺序或块数量变化：${baselineKeys.join(',')} -> ${afterKeys.join(',')}`)
          break
        }
        for (let index = 0; index < after.length; index += 1) {
          const delta = after[index].visualDepth - previous[index].visualDepth
          if (delta > 0) {
            problems.push(`[${name}/${mode}/第${presses}按] ${after[index].text} 反向变深 ${previous[index].visualDepth} -> ${after[index].visualDepth}`)
          }
          if (delta < -1) {
            problems.push(`[${name}/${mode}/第${presses}按] ${after[index].text} 一次降超过一级 ${previous[index].visualDepth} -> ${after[index].visualDepth}`)
          }
          if (!afterKeys.includes(baselineKeys[index])) {
            problems.push(`[${name}/${mode}/第${presses}按] 块 ${baselineKeys[index]} 身份对不上`)
          }
        }
        const { empty, from, to } = editor.state.selection
        if (empty) problems.push(`[${name}/${mode}/第${presses}按] 选区消失`)
        const covered = new Set()
        after.forEach((entry, index) => {
          if (from <= entry.textEnd && to >= entry.textStart) covered.add(afterKeys[index])
        })
        for (const key of covered) {
          if (!baselineKeys.includes(key)) problems.push(`[${name}/${mode}/第${presses}按] 选区多盖未选中项 ${key}`)
        }
        // 拖选全文后连按：选区必须一直覆盖原来那批块，不能每按一次缩水一圈
        // （用户现象：紫色高亮越按越少，看着像选区丢了）。
        for (const key of baselineKeys) {
          if (!covered.has(key)) problems.push(`[${name}/${mode}/第${presses}按] 选区缩水，漏掉 ${key}`)
        }
        previous = after
      }

      const finalBlocks = enumerateDocBlocks(editor.state.doc)
      if (finalBlocks.some((block) => block.visualDepth > 0)) {
        problems.push(`[${name}/${mode}] 连按 ${presses} 次仍未全部退出列表：${finalBlocks.map((b) => `${b.text}@${b.visualDepth}`).join(' | ')}`)
      }
      if (hasListNodes(editor.getJSON())) {
        problems.push(`[${name}/${mode}] 全部退出后文档里仍残留列表节点`)
      }
      if (presses === 0) problems.push(`[${name}/${mode}] 第一按就没有被处理`)
      editor.destroy()
    }
  }
  assert.equal(problems.length, 0, `阶梯反缩进失败 ${problems.length} 例：\n${problems.join('\n')}`)
})

// ---- B04 回归：一级唯一项绝不"凭空缩进"，和它在文档里的位置无关 -----------
//
// 回归背景（2026-10-16 用户反馈）：选中图片 → 转成列表 → 一级项居然能 Tab，
// 但"单独开一个文档"有时又复现不出来。原因是 canUseExtraIndent() 里有一条
// isDetachedTopLevelListItem() 启发式 —— 它把"顶层列表首项 + 文档里它前面还有
// 别的块"一律当成 Shift+Tab 拆出来的游离项，于是同一条操作会因为"列表前面有
// 没有内容"而表现不同。正确判据只有两条：已经在二级/三级，或该项自身带
// dvnIndentLevel（游离子树把视觉层级存在属性里）。
test('B04：一级唯一项无论在文档哪个位置，Tab 都不得产生悬空缩进', () => {
  const window = setupDom()
  const cases = [
    { name: '列表是文档第一块', content: [{ type: 'paragraph', content: [{ type: 'text', text: '唯一的图片行' }] }] },
    {
      name: '列表前面有一个普通段落',
      content: [
        { type: 'paragraph', content: [{ type: 'text', text: '上面的正文' }] },
        { type: 'paragraph', content: [{ type: 'text', text: '唯一的图片行' }] },
      ],
    },
    {
      name: '列表前面有多段内容',
      content: [
        { type: 'paragraph', content: [{ type: 'text', text: '正文一' }] },
        { type: 'paragraph', content: [{ type: 'text', text: '正文二' }] },
        { type: 'paragraph', content: [{ type: 'text', text: '唯一的图片行' }] },
      ],
    },
  ]

  const problems = []
  for (const spec of cases) {
    for (const listType of ['bulletList', 'orderedList', 'taskList']) {
      const editor = createEditor(window, { type: 'doc', content: spec.content })
      // 把"唯一的图片行"那段整体转成列表（等价于选中图片后点列表按钮）
      const targetPos = findTextRange(editor, '唯一的图片行')
      editor.chain().focus().setTextSelection(targetPos).run()
      const command = `toggle${listType.charAt(0).toUpperCase()}${listType.slice(1)}`
      editor.chain().focus()[command]().run()

      const beforeBlocks = enumerateDocBlocks(editor.state.doc)
      const item = beforeBlocks.find((block) => block.blockKind === 'item' && block.text === '唯一的图片行')
      if (!item) {
        problems.push(`[${spec.name}/${listType}] 转列表失败，没找到列表项`)
        editor.destroy()
        continue
      }
      if (item.visualDepth !== 1) {
        problems.push(`[${spec.name}/${listType}] 转列表后应是视觉一级，实际 ${item.visualDepth}`)
      }

      const event = dispatchTab(editor, window, false)
      const after = enumerateDocBlocks(editor.state.doc)
      const afterItem = after.find((block) => block.blockKind === 'item' && block.text === '唯一的图片行')
      if (event.defaultPrevented && afterItem && afterItem.visualDepth !== 1) {
        problems.push(
          `[${spec.name}/${listType}] 一级唯一项被 Tab 悬空缩进到视觉 ${afterItem.visualDepth}`,
        )
      }
      if (!afterItem || afterItem.visualDepth !== 1) {
        problems.push(`[${spec.name}/${listType}] Tab 后该行视觉层级变成 ${afterItem ? afterItem.visualDepth : '丢失'}`)
      }
      editor.destroy()
    }
  }
  assert.equal(problems.length, 0, `一级唯一项悬空缩进 ${problems.length} 例：\n${problems.join('\n')}`)
})

// 反向保证：反缩进把子列表拆成新的顶层列表块后，靠 dvnIndentLevel 仍能 Tab 回到二级。
test('B04 反向：反缩进拆出来的游离子项仍可用 Tab 恢复到二级', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [{
      type: 'orderedList',
      attrs: { start: 1 },
      content: [listItem('a', [list('orderedList', [listItem('b')])])],
    }],
  })

  editor.chain().focus().setTextSelection(findTextRange(editor, 'a')).run()
  dispatchTab(editor, window, true) // Shift+Tab：a 退出列表，b 成为新的顶层首项
  const afterLift = enumerateDocBlocks(editor.state.doc)
  const lifted = afterLift.find((block) => block.blockKind === 'item' && block.text === 'b')
  assert.ok(lifted, '反缩进后 b 应该还是列表项')
  assert.equal(lifted.visualDepth, 2, 'b 反缩进后视觉层级应保持二级')

  const bRange = findTextRange(editor, 'b')
  editor.chain().focus().setTextSelection(bRange).run()
  const handled = sinkActiveListItem(editor)
  const afterTab = enumerateDocBlocks(editor.state.doc)
  const reindented = afterTab.find((block) => block.blockKind === 'item' && block.text === 'b')
  assert.equal(handled, true, '游离子项 Tab 应被处理')
  assert.equal(reindented.visualDepth, 3, 'b 应能 Tab 回到三级')
  editor.destroy()
})


// 图片是 NodeSelection，不是文本范围选区；列表缩进/反缩进后必须保留图片节点
// 选中框。三种列表都要覆盖，重复图片还要按“第几次出现”确认恢复的是原图片。
test('图片 NodeSelection 在 Tab / Shift+Tab 后保持选中，三种列表全覆盖', () => {
  const problems = []
  for (const listType of ['bulletList', 'orderedList', 'taskList']) {
    const window = setupDom()
    const editor = createEditor(window, imageListDoc(listType, ['images/a.png', 'images/a.png', 'images/a.png']))

    selectImage(editor, 'images/a.png', 1)
    const sinkEvent = dispatchTab(editor, window, false)
    if (!sinkEvent.defaultPrevented) problems.push(`[${listType}] Tab 未被列表逻辑处理`)
    if (editor.state.selection.node?.type.name !== 'image') {
      problems.push(`[${listType}] Tab 后图片 NodeSelection 丢失：${editor.state.selection.constructor.name}`)
    }
    if (selectedImageOccurrence(editor, 'images/a.png') !== 1) {
      problems.push(`[${listType}] Tab 后恢复到了错误的重复图片`)
    }
    if (selectedImageVisualDepth(editor) !== 2) {
      problems.push(`[${listType}] Tab 后图片项视觉层级应为 2，实际 ${selectedImageVisualDepth(editor)}`)
    }

    const liftEvent = dispatchTab(editor, window, true)
    if (!liftEvent.defaultPrevented) problems.push(`[${listType}] Shift+Tab 未被列表逻辑处理`)
    if (editor.state.selection.node?.type.name !== 'image') {
      problems.push(`[${listType}] Shift+Tab 后图片 NodeSelection 丢失：${editor.state.selection.constructor.name}`)
    }
    if (selectedImageOccurrence(editor, 'images/a.png') !== 1) {
      problems.push(`[${listType}] Shift+Tab 后恢复到了错误的重复图片`)
    }
    if (selectedImageVisualDepth(editor) !== 1) {
      problems.push(`[${listType}] Shift+Tab 后图片项视觉层级应为 1，实际 ${selectedImageVisualDepth(editor)}`)
    }

    editor.destroy()
  }
  assert.equal(problems.length, 0, `图片列表选中态回归失败：\n${problems.join('\n')}`)
})

// 一级唯一图片项按 Tab 不能悬空缩进；即使列表逻辑处理了按键，图片选中态也不能丢。
test('图片是列表首项时 Tab 不悬空，且选中态保持', () => {
  const window = setupDom()
  const editor = createEditor(window, imageListDoc('bulletList', ['images/only.png']))
  selectImage(editor, 'images/only.png')

  const event = dispatchTab(editor, window, false)
  assert.equal(event.defaultPrevented, true, '一级唯一项 Tab 应被列表逻辑接管但不改变结构')
  assert.equal(editor.state.selection.node?.type.name, 'image', 'Tab 后图片 NodeSelection 应保持')
  assert.equal(selectedImageVisualDepth(editor), 1, '一级唯一图片项不得悬空缩进')
  editor.destroy()
})

// ---- 全部组合在模块加载期跑完，这里统一汇报。一次跑出所有坏组合，而不是只报第一个。
test('list indent/outdent matrix', () => {
  assert.equal(FAILURES.length, 0, `矩阵失败 ${FAILURES.length} 例：\n${FAILURES.join('\n')}`)
})
