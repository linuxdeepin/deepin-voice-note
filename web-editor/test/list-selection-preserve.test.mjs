// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { AllSelection } from '@tiptap/pm/state'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { switchSelectedListType, toggleActiveListFormatting } from '../src/runtime/list-behavior.js'

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
}

function listDoc(itemCount, listType = 'orderedList') {
  return {
    type: 'doc',
    content: [
      {
        type: listType,
        attrs: listType === 'orderedList' ? { start: 1 } : {},
        content: Array.from({ length: itemCount }, () => ({
          type: 'listItem',
          content: [{ type: 'paragraph', content: [{ type: 'text', text: 'line' }] }],
        })),
      },
    ],
  }
}

function createEditor(content) {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  return new Editor({ element, extensions: createTiptapExtensions(), content })
}

function textRanges(editor) {
  const ranges = []
  editor.state.doc.descendants((node, pos) => {
    if (node.isText) ranges.push({ from: pos, to: pos + node.nodeSize })
    return true
  })
  return ranges
}

test('全选后切换列表类型，全选状态保持（紫色高亮不消失）', () => {
  const editor = createEditor(listDoc(3))
  const before = textRanges(editor)
  editor.commands.setTextSelection({ from: before[0].from, to: before.at(-1).to })

  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.equal(editor.state.doc.firstChild.type.name, 'bulletList')

  const after = textRanges(editor)
  assert.equal(editor.state.selection.empty, false, '切换列表后不应塌缩成光标')
  assert.equal(editor.state.selection.from, after[0].from)
  assert.equal(editor.state.selection.to, after.at(-1).to)
  editor.destroy()
})

test('再切回有序列表，全选状态仍然保持', () => {
  const editor = createEditor(listDoc(3))
  const before = textRanges(editor)
  editor.commands.setTextSelection({ from: before[0].from, to: before.at(-1).to })

  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)
  assert.equal(editor.state.doc.firstChild.type.name, 'orderedList')

  const after = textRanges(editor)
  assert.equal(editor.state.selection.empty, false)
  assert.equal(editor.state.selection.from, after[0].from)
  assert.equal(editor.state.selection.to, after.at(-1).to)
  editor.destroy()
})

test('文档只有部分列表被完整选中时，选区仍覆盖被转换的那个列表', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'head' }] },
      ...listDoc(3).content,
    ],
  })
  // 只选中第二个块（整个有序列表），不是全选文档。
  const ranges = textRanges(editor)
  editor.commands.setTextSelection({ from: ranges[1].from, to: ranges.at(-1).to })

  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.equal(editor.state.doc.child(1).type.name, 'bulletList')

  const listStart = editor.state.doc.child(0).nodeSize
  const after = textRanges(editor).filter((range) => range.from > listStart)
  assert.equal(editor.state.selection.empty, false)
  assert.equal(editor.state.selection.from, after[0].from)
  assert.equal(editor.state.selection.to, after.at(-1).to)
  editor.destroy()
})

test('AllSelection 全选切换列表后仍是 AllSelection', () => {
  const editor = createEditor(listDoc(3))
  editor.view.dispatch(editor.state.tr.setSelection(new AllSelection(editor.state.doc)))

  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.equal(editor.state.selection instanceof AllSelection, true)
  editor.destroy()
})

test('目标类型与当前一致时取消列表，不缩进且保持选区', () => {
  const editor = createEditor(listDoc(3))
  const before = textRanges(editor)
  editor.commands.setTextSelection({ from: before[0].from, to: before.at(-1).to })

  assert.equal(switchSelectedListType(editor, 'orderedList'), true)
  assert.equal(editor.state.doc.childCount, 3)
  assert.deepEqual(
    Array.from({ length: editor.state.doc.childCount }, (_, index) => editor.state.doc.child(index).type.name),
    ['paragraph', 'paragraph', 'paragraph'],
  )
  assert.equal(editor.state.selection.empty, false)
  assert.equal(selectedText(editor), 'line|line|line')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 只选中列表的一部分：应该只转换选中的那几项，未选中的保持原类型。
// ---------------------------------------------------------------------------

function itemTypeFor(listType) {
  return listType === 'taskList' ? 'taskItem' : 'listItem'
}

function listDocWithTexts(texts, listType = 'bulletList') {
  return {
    type: 'doc',
    content: [
      {
        type: listType,
        attrs: listType === 'orderedList' ? { start: 1 } : {},
        content: texts.map((text) => ({
          type: itemTypeFor(listType),
          content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
        })),
      },
    ],
  }
}

function selectListItems(editor, firstIndex, lastIndex) {
  const list = editor.state.doc.firstChild
  let pos = 1
  let from = null
  let to = null
  for (let index = 0; index < list.childCount; index += 1) {
    const item = list.child(index)
    if (index === firstIndex) from = pos + 2
    if (index === lastIndex) to = pos + item.nodeSize - 2
    pos += item.nodeSize
  }
  editor.commands.setTextSelection({ from, to })
  return { from, to }
}

function blockSummary(editor) {
  return editor.state.doc.content.content.map((node) => ({
    type: node.type.name,
    texts: node.content.content.map((child) => child.textContent),
  }))
}

function selectedText(editor) {
  const { from, to } = editor.state.selection
  return editor.state.doc.textBetween(from, to, '|')
}

const TEN_LINES = Array.from({ length: 10 }, (_, index) => `line${index + 1}`)
const TWELVE_LINES = Array.from({ length: 12 }, (_, index) => `line${index + 1}`)

test('无序列表只选中后 5 项切有序：前 5 项保持无序', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES))

  selectListItems(editor, 5, 9)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  assert.deepEqual(blockSummary(editor), [
    { type: 'bulletList', texts: TEN_LINES.slice(0, 5) },
    { type: 'orderedList', texts: TEN_LINES.slice(5) },
  ])
  assert.equal(editor.state.doc.child(1).attrs.start, 1, '新有序段从 1 开始编号')
  assert.equal(editor.state.selection.empty, false, '转换后选区不应塌缩')
  assert.equal(selectedText(editor), TEN_LINES.slice(5).join('|'))
  editor.destroy()
})

test('有序列表只选中后 5 项切无序：前 5 项保持有序', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES, 'orderedList'))

  selectListItems(editor, 5, 9)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)

  assert.deepEqual(blockSummary(editor), [
    { type: 'orderedList', texts: TEN_LINES.slice(0, 5) },
    { type: 'bulletList', texts: TEN_LINES.slice(5) },
  ])
  assert.equal(editor.state.selection.empty, false)
  editor.destroy()
})

test('只选中中间 3 项切有序：拆成 无序 | 有序 | 无序 三段', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES))

  selectListItems(editor, 3, 5)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  assert.deepEqual(blockSummary(editor), [
    { type: 'bulletList', texts: TEN_LINES.slice(0, 3) },
    { type: 'orderedList', texts: TEN_LINES.slice(3, 6) },
    { type: 'bulletList', texts: TEN_LINES.slice(6) },
  ])
  assert.equal(selectedText(editor), TEN_LINES.slice(3, 6).join('|'))
  editor.destroy()
})

test('待办列表只选中后 5 项切有序：只转换选中的 5 项', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES, 'taskList'))

  selectListItems(editor, 5, 9)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  assert.deepEqual(blockSummary(editor), [
    { type: 'taskList', texts: TEN_LINES.slice(0, 5) },
    { type: 'orderedList', texts: TEN_LINES.slice(5) },
  ])
  editor.destroy()
})

test('无序列表只选中后 5 项切待办：只转换选中的 5 项', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES))

  selectListItems(editor, 5, 9)
  assert.equal(switchSelectedListType(editor, 'taskList'), true)

  assert.deepEqual(blockSummary(editor), [
    { type: 'bulletList', texts: TEN_LINES.slice(0, 5) },
    { type: 'taskList', texts: TEN_LINES.slice(5) },
  ])
  editor.destroy()
})

test('只选中一项切有序：同样只拆出这一项', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES))

  selectListItems(editor, 4, 4)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  assert.deepEqual(blockSummary(editor), [
    { type: 'bulletList', texts: TEN_LINES.slice(0, 4) },
    { type: 'orderedList', texts: TEN_LINES.slice(4, 5) },
    { type: 'bulletList', texts: TEN_LINES.slice(5) },
  ])
  editor.destroy()
})

test('部分选中时点同类列表按钮只取消选中项，未选中项仍留在原列表', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES))
  selectListItems(editor, 4, 8)

  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.deepEqual(blockSummary(editor), [
    { type: 'bulletList', texts: TEN_LINES.slice(0, 4) },
    { type: 'paragraph', texts: ['line5'] },
    { type: 'paragraph', texts: ['line6'] },
    { type: 'paragraph', texts: ['line7'] },
    { type: 'paragraph', texts: ['line8'] },
    { type: 'paragraph', texts: ['line9'] },
    { type: 'bulletList', texts: TEN_LINES.slice(9) },
  ])
  assert.equal(selectedText(editor), TEN_LINES.slice(4, 9).join('|'))
  editor.destroy()
})

test('取消有序列表中间一段后，前后两段有序列表分别从 1 重新编号', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES, 'orderedList'))
  selectListItems(editor, 4, 8)

  assert.equal(switchSelectedListType(editor, 'orderedList'), true)
  assert.deepEqual(blockSummary(editor), [
    { type: 'orderedList', texts: TEN_LINES.slice(0, 4) },
    { type: 'paragraph', texts: ['line5'] },
    { type: 'paragraph', texts: ['line6'] },
    { type: 'paragraph', texts: ['line7'] },
    { type: 'paragraph', texts: ['line8'] },
    { type: 'paragraph', texts: ['line9'] },
    { type: 'orderedList', texts: TEN_LINES.slice(9) },
  ])
  assert.deepEqual(orderedNumbers(editor, 0), [1, 2, 3, 4])
  assert.deepEqual(orderedNumbers(editor, 6), [1], '中断后的有序片段应从 1 重新编号')
  assert.equal(selectedText(editor), TEN_LINES.slice(4, 9).join('|'))
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 反复“全选 / 部分选中”混用切换类型：相邻同类型列表必须合并，序号必须连续。
// ---------------------------------------------------------------------------

function listItemRanges(editor, blockIndex) {
  const doc = editor.state.doc
  let blockPos = 0
  for (let index = 0; index < blockIndex; index += 1) blockPos += doc.child(index).nodeSize

  const list = doc.child(blockIndex)
  const ranges = []
  let itemPos = blockPos + 1
  for (let index = 0; index < list.childCount; index += 1) {
    const size = list.child(index).nodeSize
    ranges.push({ from: itemPos + 2, to: itemPos + size - 2 })
    itemPos += size
  }
  return ranges
}

function selectBlockItems(editor, blockIndex, firstIndex, lastIndex) {
  const ranges = listItemRanges(editor, blockIndex)
  const from = ranges[firstIndex].from
  const to = ranges[lastIndex].to
  editor.commands.setTextSelection({ from, to })
  return { from, to }
}

function selectWholeDocumentText(editor) {
  const ranges = textRanges(editor)
  const from = ranges[0].from
  const to = ranges.at(-1).to
  editor.commands.setTextSelection({ from, to })
  return { from, to }
}

function orderedNumbers(editor, blockIndex) {
  const list = editor.state.doc.child(blockIndex)
  const start = Number(list.attrs?.start ?? 1)
  return Array.from({ length: list.childCount }, (_, index) => start + index)
}

test('选区落在子列表里时只拆最内层子列表', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      {
        type: 'bulletList',
        attrs: {},
        content: [
          {
            type: 'listItem',
            content: [
              { type: 'paragraph', content: [{ type: 'text', text: 'outer1' }] },
              {
                type: 'bulletList',
                attrs: {},
                content: ['inner1', 'inner2', 'inner3'].map((text) => ({
                  type: 'listItem',
                  content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
                })),
              },
            ],
          },
          { type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'outer2' }] }] },
        ],
      },
    ],
  })

  const ranges = textRanges(editor)
  const innerRanges = ranges.filter((range) => editor.state.doc.textBetween(range.from, range.to).startsWith('inner'))
  editor.commands.setTextSelection({ from: innerRanges[1].from, to: innerRanges[1].to })

  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  const outer = editor.state.doc.child(0)
  assert.equal(outer.type.name, 'bulletList', '外层列表保持无序')
  assert.equal(outer.childCount, 2)
  assert.equal(outer.child(0).child(0).textContent, 'outer1')

  // 子列表按选区拆成 无序 | 有序 | 无序 三段，只有选中的 inner2 换了类型。
  const nestedBlocks = outer.child(0).content.content.slice(1)
  assert.deepEqual(
    nestedBlocks.map((node) => [node.type.name, node.child(0).textContent]),
    [['bulletList', 'inner1'], ['orderedList', 'inner2'], ['bulletList', 'inner3']],
  )
  editor.destroy()
})

test('全选后轮流切换列表类型：相邻同类型列表合并成一个，不残留多块', () => {
  const editor = createEditor(listDocWithTexts(TWELVE_LINES, 'orderedList'))

  selectWholeDocumentText(editor)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['bulletList'])

  selectBlockItems(editor, 0, 8, 11)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)
  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['bulletList', 'orderedList'])

  selectBlockItems(editor, 1, 1, 2)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.deepEqual(
    blockSummary(editor).map((block) => block.type),
    ['bulletList', 'orderedList', 'bulletList', 'orderedList'],
  )

  selectWholeDocumentText(editor)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['bulletList'], '全选切无序后四段合并成一块')
  assert.equal(editor.state.doc.child(0).childCount, 12)
  editor.destroy()
})

test('全选切有序后是一个连续编号的有序列表（1..N，不重新计数）', () => {
  const editor = createEditor(listDocWithTexts(TWELVE_LINES, 'orderedList'))

  selectWholeDocumentText(editor)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  selectBlockItems(editor, 0, 8, 11)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)
  selectBlockItems(editor, 1, 1, 2)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  selectWholeDocumentText(editor)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  const blocks = blockSummary(editor)
  assert.equal(blocks.length, 1, '整体只剩一个有序列表块')
  assert.equal(blocks[0].type, 'orderedList')
  assert.equal(blocks[0].texts.length, 12)
  assert.equal(editor.state.doc.child(0).attrs.start, 1)
  assert.deepEqual(orderedNumbers(editor, 0), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], '编号连续不重置')
  editor.destroy()
})

test('相邻同类型列表在转换后被合并为一个列表', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      ...listDocWithTexts(['a1', 'a2', 'a3']).content,
      ...listDocWithTexts(['b1', 'b2']).content,
    ],
  })

  assert.equal(editor.state.doc.childCount, 2)
  selectWholeDocumentText(editor)
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['orderedList'])
  assert.deepEqual(orderedNumbers(editor, 0), [1, 2, 3, 4, 5])
  assert.equal(editor.state.selection.empty, false, '合并后选区不应塌缩')
  assert.equal(selectedText(editor), 'a1|a2|a3|b1|b2')
  editor.destroy()
})

test('转换后有序段与前一个有序块相邻时合并，编号接着延续', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      ...listDocWithTexts(['o1', 'o2', 'o3'], 'orderedList').content,
      { type: 'paragraph', content: [{ type: 'text', text: 'sep' }] },
      ...listDocWithTexts(['x1', 'x2'], 'orderedList').content,
    ],
  })

  // 只选中第三块（中间隔了段落，不应被合并）
  const ranges = listItemRanges(editor, 2)
  editor.commands.setTextSelection({ from: ranges[0].from, to: ranges.at(-1).to })
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['orderedList', 'paragraph', 'bulletList'])

  editor.commands.setTextSelection({ from: ranges[0].from, to: ranges.at(-1).to })
  const bulletRanges = listItemRanges(editor, 2)
  editor.commands.setTextSelection({ from: bulletRanges[0].from, to: bulletRanges.at(-1).to })
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)
  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['orderedList', 'paragraph', 'orderedList'])
  assert.equal(editor.state.doc.child(2).attrs.start, 1, '被段落隔开的块不合并，仍是新的一段')
  editor.destroy()
})

test('选中段与前面的同类型列表合并后，选区仍然覆盖被选中的那几项（不缩水）', () => {
  const editor = createEditor(listDocWithTexts(TWELVE_LINES, 'orderedList'))

  // 1) 选中第 9~12 项，切成无序：前面 8 项保持有序，选中段成为新的无序列表。
  selectBlockItems(editor, 0, 8, 11)
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)
  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['orderedList', 'bulletList'])
  assert.equal(selectedText(editor), TWELVE_LINES.slice(8).join('|'))

  // 2) 保持这四项仍被选中，切回有序：会与前 8 项的有序列表合并成一个列表。
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)
  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['orderedList'])
  assert.equal(editor.state.doc.child(0).childCount, 12)

  // 合并之后选区必须还是第 9~12 项，而不是缩成合并结果里的第一个列表项。
  assert.equal(editor.state.selection.empty, false)
  assert.equal(selectedText(editor), TWELVE_LINES.slice(8).join('|'), '合并后选区不应缩水到只覆盖第 9 项')
  editor.destroy()
})

test('混合列表里选中的有序段切无序并与前一段合并后，选区保持原选中项', () => {
  const editor = createEditor({
    type: 'doc',
    content: [
      ...listDocWithTexts(TWELVE_LINES.slice(0, 9)).content,
      ...listDocWithTexts(TWELVE_LINES.slice(9), 'orderedList').content,
    ],
  })

  const ranges = listItemRanges(editor, 1)
  editor.commands.setTextSelection({ from: ranges[0].from, to: ranges.at(-1).to })
  assert.equal(switchSelectedListType(editor, 'bulletList'), true)

  assert.deepEqual(blockSummary(editor).map((block) => block.type), ['bulletList'], '与前一段无序列表合并')
  assert.equal(editor.state.doc.child(0).childCount, 12)
  assert.equal(editor.state.selection.empty, false)
  assert.equal(selectedText(editor), TWELVE_LINES.slice(9).join('|'))
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 跨层级部分选中：选中项内部还挂着子列表
//
// 用户反馈：选中“地方法 + 其下两个子级打”后转待办，高亮起点向前漂到未选中的
// “打”，再点有序时整个外层列表被 Tiptap 默认命令拆散、高亮扩到全部。
// 根因是外层列表和它内部被完整选中的子列表同时进入转换操作（范围重叠），
// 以及 partial 选择只看 depth 不看“选区边界落在哪个列表里”。
// ---------------------------------------------------------------------------

function nestedMixedDoc() {
  const textItem = (text, extra = []) => ({
    type: 'listItem',
    content: [{ type: 'paragraph', content: [{ type: 'text', text }] }, ...extra],
  })
  const ordered = (items) => ({ type: 'orderedList', attrs: { start: 1 }, content: items })
  return {
    type: 'doc',
    content: [ordered([
      textItem('飒飒', [ordered([
        textItem('打'),
        textItem('地方法', [ordered([textItem('打'), textItem('打')])]),
      ])]),
      textItem('撒啊'),
      textItem('ss'),
      textItem('发给过'),
    ])],
  }
}

function selectFromMidNestedToLastText(editor, startText, endText) {
  let from = null
  let to = null
  editor.state.doc.descendants((node, pos) => {
    if (!node.isText) return true
    if (node.text === startText && from === null) from = pos
    if (node.text === endText) to = pos + node.nodeSize
    return true
  })
  if (from === null || to === null) throw new Error('selection text not found')
  editor.commands.setTextSelection({ from, to })
  return { from, to }
}

test('跨层级选中子列表内容：转待办后选区文本不漂移、未选中项保持原类型', () => {
  const editor = createEditor(nestedMixedDoc())
  selectFromMidNestedToLastText(editor, '地方法', '打')

  assert.equal(switchSelectedListType(editor, 'taskList'), true)

  const top = editor.state.doc.child(0)
  assert.equal(top.type.name, 'orderedList', '外层有序列表必须保留')
  assert.equal(top.childCount, 4, '外层 4 个列表项不能增减')
  assert.deepEqual(
    Array.from({ length: top.childCount }, (_, index) => top.child(index).child(0).textContent),
    ['飒飒', '撒啊', 'ss', '发给过'],
  )

  const nested = top.child(0).content.content.slice(1)
  assert.deepEqual(nested.map((node) => node.type.name), ['orderedList', 'taskList'],
    '未选中的“打”保持有序，选中的“地方法”变待办')
  assert.equal(nested[0].child(0).child(0).textContent, '打')
  const taskItem = nested[1].child(0)
  assert.equal(taskItem.child(0).textContent, '地方法')
  assert.equal(taskItem.child(1).type.name, 'taskList', '选中项内部的子列表一起转待办')

  const selection = editor.state.selection
  assert.equal(editor.state.doc.textBetween(selection.from, selection.to, '|'), '地方法|打|打',
    '选区必须仍只覆盖最初选中的文字')
  editor.destroy()
})

test('跨层级选中后连续切换：第二次点有序不再被默认命令拆散结构', () => {
  const editor = createEditor(nestedMixedDoc())
  selectFromMidNestedToLastText(editor, '地方法', '打')
  assert.equal(switchSelectedListType(editor, 'taskList'), true)

  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  const top = editor.state.doc.child(0)
  assert.equal(top.type.name, 'orderedList')
  assert.equal(top.childCount, 4, '外层列表结构不能被拆散')
  assert.deepEqual(
    Array.from({ length: top.childCount }, (_, index) => top.child(index).child(0).textContent),
    ['飒飒', '撒啊', 'ss', '发给过'],
  )

  const nested = top.child(0).content.content.slice(1)
  assert.deepEqual(nested.map((node) => node.type.name), ['orderedList'],
    '切回有序后相邻的同类型列表必须合并回一个')
  const innerList = nested[0]
  assert.deepEqual(
    Array.from({ length: innerList.childCount }, (_, index) => innerList.child(index).child(0).textContent),
    ['打', '地方法'],
  )
  assert.equal(innerList.child(1).child(1).type.name, 'orderedList', '三级子列表也保持有序')

  const selection = editor.state.selection
  assert.equal(editor.state.doc.textBetween(selection.from, selection.to, '|'), '地方法|打|打',
    '第二次切换后选区文本仍不能扩大')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 混合嵌套列表：选中整段后点任意列表按钮，必须把选中范围内的所有层级整体
// 转成目标类型，不能只改最内层后落到 Tiptap 默认命令，表现成 Shift+Tab。
// ---------------------------------------------------------------------------

const ALL_LIST_TYPES = ['bulletList', 'orderedList', 'taskList']

function mixedListItem(text, listType, children = []) {
  return {
    type: itemTypeFor(listType),
    attrs: listType === 'taskList' ? { checked: false } : {},
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  }
}

function mixedListOfType(listType, items) {
  return {
    type: listType,
    attrs: listType === 'orderedList' ? { start: 1 } : {},
    content: items,
  }
}

// 与用户截图同构：三种列表在多个层级相互嵌套。
function screenshotMixedNestedDoc() {
  return {
    type: 'doc',
    content: [
      mixedListOfType('bulletList', [
        mixedListItem('dasdsa', 'bulletList', [
          mixedListOfType('bulletList', [
            mixedListItem('dasdsadas', 'bulletList'),
          ]),
          mixedListOfType('orderedList', [
            mixedListItem('ddsadsad', 'orderedList', [
              mixedListOfType('taskList', [
                mixedListItem('dasdsa', 'taskList'),
                mixedListItem('dasdsa', 'taskList'),
              ]),
            ]),
          ]),
        ]),
        mixedListItem('asdas', 'bulletList', [
          mixedListOfType('orderedList', [
            mixedListItem('dasds', 'orderedList', [
              mixedListOfType('orderedList', [
                mixedListItem('dsadsa', 'orderedList'),
                mixedListItem('dsadsad', 'orderedList'),
              ]),
            ]),
          ]),
        ]),
        mixedListItem('asdas', 'bulletList', [
          mixedListOfType('taskList', [
            mixedListItem('dsdsads', 'taskList', [
              mixedListOfType('orderedList', [
                mixedListItem('dasdadsa', 'orderedList'),
              ]),
            ]),
          ]),
        ]),
      ]),
    ],
  }
}

function singleNestedTaskDoc() {
  const taskItem = (text, children = []) => ({
    type: 'taskItem',
    attrs: { checked: false, dvnIndentLevel: 0 },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })
  const taskList = (content) => ({ type: 'taskList', content })
  return {
    type: 'doc',
    content: [
      taskList([
        taskItem('asdas', [
          taskList([
            taskItem('dasds', [
              taskList([
                taskItem('dsadsa'),
                taskItem('dasdsad'),
              ]),
            ]),
          ]),
        ]),
        taskItem('asdas'),
      ]),
    ],
  }
}

function selectTextEnd(editor, text) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (found != null) return false
    if (node.isText && node.text === text) {
      found = pos + node.nodeSize
      return false
    }
    return true
  })
  assert.notEqual(found, null, `找不到文本：${text}`)
  editor.commands.setTextSelection(found)
}

function selectScreenshotBranch(editor) {
  const ranges = textRanges(editor)
  const start = ranges.find((range) => editor.state.doc.textBetween(range.from, range.to) === 'dasdsadas')
  const selectedDasdsa = ranges.filter((range) => editor.state.doc.textBetween(range.from, range.to) === 'dasdsa')
  assert.ok(start)
  assert.ok(selectedDasdsa.length >= 3)
  editor.commands.setTextSelection({ from: start.from, to: selectedDasdsa[2].to })
}

// 选区里每个文本节点的列表祖先。用于验证“取消”后确实离开列表，而不是仅仅
// 把项目符号/复选框藏起来；也用于验证重新应用后只有一层目标列表。
function selectedTextblockListContexts(editor) {
  const { doc, selection } = editor.state
  const contexts = []
  doc.nodesBetween(selection.from, selection.to, (node, pos) => {
    if (!node.isText) return true
    const $pos = doc.resolve(pos)
    const listTypes = []
    const itemTypes = []
    for (let depth = 1; depth <= $pos.depth; depth += 1) {
      const ancestor = $pos.node(depth)
      if (ALL_LIST_TYPES.includes(ancestor.type.name)) listTypes.push(ancestor.type.name)
      if (ancestor.type.name === 'listItem' || ancestor.type.name === 'taskItem') itemTypes.push(ancestor.type.name)
    }
    contexts.push({ text: node.text, listTypes, itemTypes })
    return true
  })
  return contexts
}

// 只记录“列表项自身文本 + 嵌套深度”，用于断言类型转换不会改变文本与层级。
function listItemHierarchy(doc) {
  const hierarchy = []
  const collect = (node, depth) => {
    if (ALL_LIST_TYPES.includes(node.type.name)) {
      node.forEach((item) => {
        hierarchy.push({ text: item.firstChild?.textContent ?? '', depth })
        item.forEach((child) => {
          if (ALL_LIST_TYPES.includes(child.type.name)) collect(child, depth + 1)
        })
      })
      return
    }
    node.forEach((child) => collect(child, depth))
  }
  doc.forEach((child) => collect(child, 0))
  return hierarchy
}

function assertEveryListAndItemIs(editor, targetListType) {
  const targetItemType = itemTypeFor(targetListType)
  const listTypes = []
  const itemTypes = []
  editor.state.doc.descendants((node) => {
    if (ALL_LIST_TYPES.includes(node.type.name)) listTypes.push(node.type.name)
    if (node.type.name === 'listItem' || node.type.name === 'taskItem') itemTypes.push(node.type.name)
    return true
  })
  assert.ok(listTypes.length > 0, '文档中应存在列表')
  assert.deepEqual([...new Set(listTypes)], [targetListType], '所有层级的列表都应为目标类型')
  assert.deepEqual([...new Set(itemTypes)], [targetItemType], '所有层级的列表项都应为目标项类型')
}

function nodeCount(editor, typeName) {
  let count = 0
  editor.state.doc.descendants((node) => {
    if (node.type.name === typeName) count += 1
    return true
  })
  return count
}

for (const targetListType of ALL_LIST_TYPES) {
  test(`截图同款混合三层列表全选转 ${targetListType}：所有层级整体转换且层级不变`, () => {
    const editor = createEditor(screenshotMixedNestedDoc())
    const beforeHierarchy = listItemHierarchy(editor.state.doc)
    selectWholeDocumentText(editor)
    const beforeSelectedText = selectedText(editor)

    assert.equal(switchSelectedListType(editor, targetListType), true)

    assertEveryListAndItemIs(editor, targetListType)
    assert.deepEqual(listItemHierarchy(editor.state.doc), beforeHierarchy, '文本和嵌套层级必须保持不变')
    assert.equal(selectedText(editor), beforeSelectedText, '转换后仍应选中同样的完整内容')
    editor.destroy()
  })
}

for (const sourceListType of ALL_LIST_TYPES) {
  for (const targetListType of ALL_LIST_TYPES) {
    test(`列表类型矩阵 ${sourceListType} -> ${targetListType}`, () => {
      const texts = ['one', 'two', 'three']
      const editor = createEditor(listDocWithTexts(texts, sourceListType))
      selectWholeDocumentText(editor)

      const handled = switchSelectedListType(editor, targetListType)
      if (sourceListType === targetListType) {
        assert.equal(handled, true, '点同类列表按钮应直接取消列表')
        assert.equal(editor.state.doc.childCount, texts.length)
        assert.deepEqual(
          Array.from({ length: editor.state.doc.childCount }, (_, index) => editor.state.doc.child(index).type.name),
          texts.map(() => 'paragraph'),
        )
      } else {
        assert.equal(handled, true)
        assertEveryListAndItemIs(editor, targetListType)
        assert.equal(selectedText(editor), texts.join('|'))
      }
      editor.destroy()
    })
  }
}

test('截图同款混合列表连续切换无序/有序/待办：每次全选范围和嵌套层级都不漂移', () => {
  const editor = createEditor(screenshotMixedNestedDoc())
  const hierarchy = listItemHierarchy(editor.state.doc)
  selectWholeDocumentText(editor)
  const selected = selectedText(editor)

  for (const targetListType of ['bulletList', 'orderedList', 'taskList']) {
    assert.equal(switchSelectedListType(editor, targetListType), true)
    assertEveryListAndItemIs(editor, targetListType)
    assert.deepEqual(listItemHierarchy(editor.state.doc), hierarchy)
    assert.equal(selectedText(editor), selected)
  }
  editor.destroy()
})

test('混合嵌套列表应用待办后再次点击待办：所有选中项都取消，不会拆层', () => {
  const editor = createEditor(screenshotMixedNestedDoc())
  selectWholeDocumentText(editor)

  assert.equal(switchSelectedListType(editor, 'taskList'), true)
  assertEveryListAndItemIs(editor, 'taskList')
  assert.equal(switchSelectedListType(editor, 'taskList'), true)

  assert.equal(editor.state.doc.childCount, 12, '每个原列表项都应变成独立普通段落')
  assert.deepEqual(
    Array.from({ length: editor.state.doc.childCount }, (_, index) => editor.state.doc.child(index).type.name),
    Array.from({ length: 12 }, () => 'paragraph'),
  )
  assert.equal(selectedText(editor), [
    'dasdsa', 'dasdsadas', 'ddsadsad', 'dasdsa', 'dasdsa', 'asdas',
    'dasds', 'dsadsa', 'dsadsad', 'asdas', 'dsdsads', 'dasdadsa',
  ].join('|'))
  editor.destroy()
})

test('截图中只选中四个嵌套待办后点待办：先应用再取消，选中行退出所有列表结构', () => {
  const editor = createEditor(screenshotMixedNestedDoc())
  const ranges = textRanges(editor)
  const start = ranges.find((range) => editor.state.doc.textBetween(range.from, range.to) === 'dasdsadas')
  const selectedDasdsa = ranges.filter((range) => editor.state.doc.textBetween(range.from, range.to) === 'dasdsa')
  assert.ok(start)
  assert.ok(selectedDasdsa.length >= 3)
  editor.commands.setTextSelection({ from: start.from, to: selectedDasdsa[2].to })

  assert.equal(switchSelectedListType(editor, 'taskList'), true, '第一次点击把选中分支统一应用为待办')
  assert.equal(switchSelectedListType(editor, 'taskList'), true, '第二次点击取消选中的待办')

  assert.equal(nodeCount(editor, 'taskItem'), 1, '未选中的待办必须保留，选中的 4 个应移除')
  assert.equal(nodeCount(editor, 'listItem'), 7, '未选中的普通列表项不能被连带取消')
  assert.equal(selectedText(editor), 'dasdsadas|ddsadsad|dasdsa|dasdsa')
  const selectedParagraphs = Array.from({ length: 4 }, (_, index) => editor.state.doc.child(index + 1))
  assert.deepEqual(selectedParagraphs.map((node) => node.type.name), [
    'paragraph', 'paragraph', 'paragraph', 'paragraph',
  ])
  assert.deepEqual(selectedParagraphs.map((node) => node.textContent), [
    'dasdsadas', 'ddsadsad', 'dasdsa', 'dasdsa',
  ], '取消后的选中行必须是独立的顶层段落，不能继续包在父待办中')
  editor.destroy()
})

test('折叠光标只取消一个嵌套待办：未选中的父级和兄弟层级保持原样', () => {
  const editor = createEditor(singleNestedTaskDoc())
  assert.deepEqual(listItemHierarchy(editor.state.doc), [
    { text: 'asdas', depth: 0 },
    { text: 'dasds', depth: 1 },
    { text: 'dsadsa', depth: 2 },
    { text: 'dasdsad', depth: 2 },
    { text: 'asdas', depth: 0 },
  ])

  selectTextEnd(editor, 'dasdsad')
  assert.equal(toggleActiveListFormatting(editor, 'taskList'), true)
  assert.equal(nodeCount(editor, 'taskItem'), 4, '只取消 dasdsad 这一个待办')
  assert.deepEqual(listItemHierarchy(editor.state.doc), [
    { text: 'asdas', depth: 0 },
    { text: 'dasds', depth: 1 },
    { text: 'dsadsa', depth: 2 },
    { text: 'asdas', depth: 0 },
  ], '父级 asdas/dasds 和兄弟 dsadsa 都不能被拍平')
  assert.deepEqual(
    Array.from({ length: editor.state.doc.childCount }, (_, index) => editor.state.doc.child(index).type.name),
    ['taskList', 'paragraph', 'taskList'],
  )
  assert.equal(editor.state.doc.child(1).textContent, 'dasdsad')
  assert.equal(editor.state.selection.$from.parent.textContent, 'dasdsad')
  assert.equal(editor.state.selection.$from.parentOffset, 'dasdsad'.length, '取消后光标仍在原文本末尾')

  assert.equal(toggleActiveListFormatting(editor, 'taskList'), true, '再次点击应重新应用，而不是继续缩进')
  assert.deepEqual(listItemHierarchy(editor.state.doc), [
    { text: 'asdas', depth: 0 },
    { text: 'dasds', depth: 1 },
    { text: 'dsadsa', depth: 2 },
    { text: 'dasdsad', depth: 0 },
    { text: 'asdas', depth: 0 },
  ], '重新应用只恢复 dasdsad 自身，未选中的嵌套层级保持不变')

  assert.equal(toggleActiveListFormatting(editor, 'taskList'), true, '第三次点击直接取消')
  assert.deepEqual(listItemHierarchy(editor.state.doc), [
    { text: 'asdas', depth: 0 },
    { text: 'dasds', depth: 1 },
    { text: 'dsadsa', depth: 2 },
    { text: 'asdas', depth: 0 },
  ])
  editor.destroy()
})

test('截图选区应用、取消、再应用九种列表组合：取消后不再缩进，重新应用只有一层目标列表', () => {
  for (const firstType of ALL_LIST_TYPES) {
    for (const secondType of ALL_LIST_TYPES) {
      const editor = createEditor(screenshotMixedNestedDoc())
      selectScreenshotBranch(editor)
      const selected = 'dasdsadas|ddsadsad|dasdsa|dasdsa'

      assert.equal(switchSelectedListType(editor, firstType), true, `${firstType}: 首次应用`)
      assert.equal(selectedText(editor), selected, `${firstType}: 首次应用后选区漂移`)

      assert.equal(switchSelectedListType(editor, firstType), true, `${firstType}: 再次点击取消`)
      assert.equal(selectedText(editor), selected, `${firstType}: 取消后选区漂移`)
      const cancelled = selectedTextblockListContexts(editor)
      assert.equal(cancelled.length, 4, `${firstType}: 取消后应覆盖四个文本块`)
      for (const context of cancelled) {
        assert.deepEqual(context.itemTypes, [], `${firstType}: ${context.text} 仍包在列表项里`)
        assert.deepEqual(context.listTypes, [], `${firstType}: ${context.text} 仍包在列表里`)
      }

      assert.equal(switchSelectedListType(editor, secondType), true, `${firstType} -> ${secondType}: 取消后重新应用`)
      assert.equal(selectedText(editor), selected, `${firstType} -> ${secondType}: 重新应用后选区漂移`)
      const reapplied = selectedTextblockListContexts(editor)
      assert.equal(reapplied.length, 4, `${firstType} -> ${secondType}: 重新应用后应覆盖四个文本块`)
      for (const context of reapplied) {
        assert.deepEqual(context.listTypes, [secondType], `${firstType} -> ${secondType}: ${context.text} 出现多余列表层级`)
        assert.deepEqual(context.itemTypes, [itemTypeFor(secondType)], `${firstType} -> ${secondType}: ${context.text} 项目类型错误`)
      }

      assert.equal(switchSelectedListType(editor, secondType), true, `${firstType} -> ${secondType}: 再次取消`)
      const cancelledAgain = selectedTextblockListContexts(editor)
      for (const context of cancelledAgain) {
        assert.deepEqual(context.itemTypes, [], `${firstType} -> ${secondType}: ${context.text} 二次取消后仍在列表项里`)
        assert.deepEqual(context.listTypes, [], `${firstType} -> ${secondType}: ${context.text} 二次取消后仍在列表里`)
      }
      editor.destroy()
    }
  }
})

test('截图同款混合列表中只拖选第一个嵌套分支：选中项整体转有序，后续分支保持原类型', () => {
  const editor = createEditor(screenshotMixedNestedDoc())
  const ranges = textRanges(editor)
  const start = ranges.find((range) => editor.state.doc.textBetween(range.from, range.to) === 'dasdsadas')
  const firstBranchTaskRanges = []
  editor.state.doc.descendants((node, pos) => {
    if (node.type.name !== 'taskItem') return true
    if (firstBranchTaskRanges.length < 2) {
      firstBranchTaskRanges.push({ from: pos + 2, to: pos + 2 + node.firstChild.content.size })
    }
    return true
  })
  assert.ok(start)
  assert.equal(firstBranchTaskRanges.length, 2)
  editor.commands.setTextSelection({ from: start.from, to: firstBranchTaskRanges[1].to })

  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  const items = []
  editor.state.doc.descendants((node) => {
    if (node.type.name !== 'listItem' && node.type.name !== 'taskItem') return true
    const parent = node.type.name === 'taskItem' ? 'taskList' : null
    items.push({
      text: node.firstChild?.textContent ?? '',
      itemType: node.type.name,
      parentHint: parent,
    })
    return true
  })
  // 第一分支 B/C/D/E 都转换；后续两个顶层分支仍保持混合类型。
  const firstNestedItems = items.slice(0, 5)
  assert.deepEqual(firstNestedItems.map((item) => item.itemType), [
    'listItem', 'listItem', 'listItem', 'listItem', 'listItem',
  ])
  const lists = []
  editor.state.doc.descendants((node) => {
    if (ALL_LIST_TYPES.includes(node.type.name)) lists.push(node.type.name)
    return true
  })
  assert.deepEqual(lists.slice(0, 4), ['bulletList', 'orderedList', 'orderedList', 'orderedList'],
    '顶层容器保持无序，B/C 以及任务子列表都变有序')
  assert.equal(lists.includes('taskList'), true, '未选中的后续待办分支不能丢类型')
  assert.equal(editor.state.doc.textBetween(editor.state.selection.from, editor.state.selection.to, '|'),
    'dasdsadas|ddsadsad|dasdsa|dasdsa')
  editor.destroy()
})

function mixedRangeSelectionDoc() {
  const item = (type, text, children = []) => ({
    type,
    attrs: type === 'taskItem' ? { checked: false } : {},
    content: [{ type: 'paragraph', content: [{ type: 'text', text }] }, ...children],
  })
  return {
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'P1' }] },
      { type: 'paragraph', content: [{ type: 'text', text: 'P2' }] },
      {
        type: 'orderedList',
        attrs: { start: 1 },
        content: [
          item('listItem', 'L1', [{
            type: 'bulletList',
            attrs: {},
            content: [item('listItem', 'L1a'), item('listItem', 'L1b')],
          }]),
          item('listItem', 'L2'),
        ],
      },
      { type: 'taskList', attrs: {}, content: [item('taskItem', 'T1'), item('taskItem', 'T2')] },
    ],
  }
}

function selectBetweenText(editor, startText, endText = startText) {
  const ranges = textRanges(editor)
  const start = ranges.find((range) => editor.state.doc.textBetween(range.from, range.to) === startText)
  const end = ranges.find((range) => editor.state.doc.textBetween(range.from, range.to) === endText)
  assert.ok(start, `missing start text: ${startText}`)
  assert.ok(end, `missing end text: ${endText}`)
  editor.commands.setTextSelection({ from: start.from, to: end.to })
}

test('只选中父项文字切换列表：选区不能吞掉未选中的子列表', () => {
  const editor = createEditor(mixedRangeSelectionDoc())
  selectBetweenText(editor, 'L1')

  assert.equal(switchSelectedListType(editor, 'bulletList'), true)

  assert.equal(selectedText(editor), 'L1')
  assert.equal(editor.state.doc.textBetween(editor.state.selection.from, editor.state.selection.to), 'L1')
  editor.destroy()
})

test('从正文选到父项末尾切换有序列表：选区不能追加子项或后续兄弟项', () => {
  const editor = createEditor(mixedRangeSelectionDoc())
  selectBetweenText(editor, 'P1', 'L1')

  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  assert.equal(selectedText(editor), 'P1|P2|L1')
  editor.destroy()
})

test('从子项选到后续父项切换有序列表：选区不能反向追加祖先或前置兄弟项', () => {
  const editor = createEditor(mixedRangeSelectionDoc())
  selectBetweenText(editor, 'L1b', 'L2')

  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  assert.equal(selectedText(editor), 'L1b|L2')
  editor.destroy()
})

test('连续两次列表按钮点击在历史中保持为两个独立操作，Undo 逐次恢复', () => {
  const editor = createEditor(screenshotMixedNestedDoc())
  selectWholeDocumentText(editor)
  const original = editor.getJSON()

  assert.equal(switchSelectedListType(editor, 'taskList'), true)
  const applied = editor.getJSON()
  assert.equal(switchSelectedListType(editor, 'taskList'), true)
  const canceled = editor.getJSON()

  assert.equal(editor.commands.undo(), true)
  assert.deepEqual(editor.getJSON(), applied, '第一次 Undo 只应撤销第二次按钮点击')
  assert.equal(editor.commands.undo(), true)
  assert.deepEqual(editor.getJSON(), original, '第二次 Undo 才应撤销第一次按钮点击')
  assert.equal(editor.commands.redo(), true)
  assert.deepEqual(editor.getJSON(), applied)
  assert.equal(editor.commands.redo(), true)
  assert.deepEqual(editor.getJSON(), canceled)
  editor.destroy()
})
