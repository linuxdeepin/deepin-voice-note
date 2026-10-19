// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { AllSelection } from '@tiptap/pm/state'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { switchSelectedListType } from '../src/runtime/list-behavior.js'

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
  editor.commands.setTextSelection({ from: ranges[0].from, to: ranges.at(-1).to })

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

test('目标类型与当前一致时不改文档、也不改选区', () => {
  const editor = createEditor(listDoc(3))
  const before = textRanges(editor)
  editor.commands.setTextSelection({ from: before[0].from, to: before.at(-1).to })

  assert.equal(switchSelectedListType(editor, 'orderedList'), false)
  assert.equal(editor.state.selection.empty, false)
  assert.equal(editor.state.selection.from, before[0].from)
  assert.equal(editor.state.selection.to, before.at(-1).to)
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
    if (index === lastIndex) to = pos + item.nodeSize - 3
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

test('部分选中时点同类列表按钮不改文档（交回默认命令取消列表）', () => {
  const editor = createEditor(listDocWithTexts(TEN_LINES))
  const { from, to } = selectListItems(editor, 5, 9)

  assert.equal(switchSelectedListType(editor, 'bulletList'), false)
  assert.deepEqual(blockSummary(editor), [
    { type: 'bulletList', texts: TEN_LINES },
  ])
  assert.equal(editor.state.selection.from, from)
  assert.equal(editor.state.selection.to, to)
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
    ranges.push({ from: itemPos + 2, to: itemPos + size - 3 })
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
