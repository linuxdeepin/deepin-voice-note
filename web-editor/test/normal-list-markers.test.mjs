// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 有序列表 marker 槽位单测：marker 槽位按序号位数撑开（"10." 不能压住正文），
// 且必须通过 ProseMirror decoration 下发，否则会被视图重渲染回滚。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import {
  NormalListMarkers,
  buildNormalListMarkerDecorations,
  listItemMarkerFontSize,
  listItemMarkerGutter,
  listItemMarkerLineHeight,
  listItemMarkerStyle,
  markerRatio,
  orderedListMarkerGutter,
  orderedListMarkerNumbers,
  normalListMarkerStyle,
  normalListMarkerStyleForNumbers,
} from '../src/runtime/normal-list-markers.js'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createEmptyDoc } from '../src/schema/document-envelope.js'

function defineGlobal(name, value) {
  Object.defineProperty(globalThis, name, {
    value,
    writable: true,
    configurable: true,
  })
}

function setupDom() {
  const window = new Window()
  defineGlobal('window', window)
  defineGlobal('document', window.document)
  defineGlobal('navigator', window.navigator)
  defineGlobal('Node', window.Node)
  defineGlobal('Element', window.Element)
  defineGlobal('HTMLElement', window.HTMLElement)
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('getComputedStyle', window.getComputedStyle.bind(window))
  return window
}

function orderedListContent(itemCount, start = 1) {
  return {
    type: 'doc',
    content: [
      {
        type: 'orderedList',
        attrs: { start },
        content: Array.from({ length: itemCount }, (_, i) => ({
          type: 'listItem',
          content: [{ type: 'paragraph', content: [{ type: 'text', text: `item ${i + 1}` }] }],
        })),
      },
    ],
  }
}

function createEditorWithMarkers() {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
  return { editor, element }
}

function markerStyleAttr(element) {
  const list = element.querySelector('.ProseMirror ol')
  return list ? list.getAttribute('style') : null
}

function renderedOrderedNumbers(element) {
  return Array.from(element.querySelectorAll('.ProseMirror ol > li'))
    .map((item) => Number(/--dvn-list-number: "(\d+)"/.exec(item.getAttribute('style') || '')?.[1]))
}

test('single digit markers keep the historical 20px gutter', () => {
  const { editor, element } = createEditorWithMarkers()

  editor.commands.setContent(orderedListContent(9))

  const style = markerStyleAttr(element)
  assert.match(style, /--dvn-ordered-marker-gutter: 20px/)
  assert.match(style, /counter-reset: dvn-ordered-list 0/)
  editor.destroy()
})

test('two digit markers widen the gutter so "10." cannot overlap the text', () => {
  const { editor, element } = createEditorWithMarkers()

  editor.commands.setContent(orderedListContent(13))

  const style = markerStyleAttr(element)
  // 13 items -> 最后一项 "13." -> 两位数槽位，必须大于默认 20px。
  assert.match(style, /--dvn-ordered-marker-gutter: 25px/)
  assert.ok(orderedListMarkerGutter(13, 1) > 20)
  editor.destroy()
})

test('start offset decides the widest marker digit count', () => {
  const { editor, element } = createEditorWithMarkers()

  // 9, 10, 11 -> 列表很短，但最后一项已经是两位数。
  editor.commands.setContent(orderedListContent(3, 9))

  const style = markerStyleAttr(element)
  assert.match(style, /--dvn-ordered-marker-gutter: 25px/)
  assert.match(style, /counter-reset: dvn-ordered-list 8/)
  editor.destroy()
})

test('gutter survives editor re-render instead of being reverted', () => {
  const { editor, element } = createEditorWithMarkers()

  editor.commands.setContent(orderedListContent(13))
  assert.match(markerStyleAttr(element), /--dvn-ordered-marker-gutter: 25px/)

  // 任何后续事务都会触发视图片段重渲染：直接写 element.style 的做法会在这里
  // 被 ProseMirror 回滚（并反复触发整篇重渲染），decoration 必须保持住。
  editor.commands.insertContentAt(1, 'x')
  assert.match(markerStyleAttr(element), /--dvn-ordered-marker-gutter: 25px/)
  assert.match(markerStyleAttr(element), /counter-reset: dvn-ordered-list 0/)
  editor.destroy()
})

test('marker decorations never enter the saved document', () => {
  const { editor } = createEditorWithMarkers()

  editor.commands.setContent(orderedListContent(13))

  const json = editor.getJSON()
  const list = json.content[0]
  assert.equal(list.type, 'orderedList')
  assert.equal(JSON.stringify(list.attrs), JSON.stringify({ start: 1, type: null }))
  assert.equal(JSON.stringify(json).includes('dvn-ordered-marker-gutter'), false)
  editor.destroy()
})

test('decoration builder sets the ordered gutter on ol and marker vars on items', () => {
  setupDom()
  const { editor } = createEditorWithMarkers()

  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'plain' }] },
      {
        type: 'bulletList',
        content: [{ type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'bullet' }] }] }],
      },
      orderedListContent(2).content[0],
    ],
  })

  const found = []
  buildNormalListMarkerDecorations(editor.state.doc).find().forEach((deco) => {
    found.push({ from: deco.from, nodeName: deco.type.attrs?.nodeName ?? null, style: deco.type.attrs?.style ?? null })
  })

  const orderedGutters = found.filter((entry) => /--dvn-ordered-marker-gutter/.test(entry.style || ''))
  assert.equal(orderedGutters.length, 1, '只有有序列表下发编号槽位变量')
  assert.match(orderedGutters[0].style, /--dvn-ordered-marker-gutter: 20px/)

  const itemVars = found.filter((entry) => /--dvn-marker-ratio/.test(entry.style || ''))
  assert.equal(itemVars.length, 3, '每个列表项（无序 + 有序）都下发 marker 字号变量')
  itemVars.forEach((entry) => {
    assert.match(entry.style, /--dvn-marker-ratio: 1; --dvn-marker-gutter: 20px/)
  })
  // 段落不是列表项，不应被装饰
  assert.equal(found.some((entry) => entry.from === 0), false)
  editor.destroy()
})

test('marker ratio follows the widest inline font size in the first paragraph', () => {
  setupDom()
  const { editor } = createEditorWithMarkers()

  const item = (parts) => ({
    type: 'listItem',
    content: [{
      type: 'paragraph',
      content: parts.map(({ size, text }) => ({
        type: 'text',
        text,
        marks: [{ type: 'fontSize', attrs: { fontSize: `${size}px` } }],
      })),
    }],
  })
  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'bulletList', content: [item([{ size: 36, text: '大字' }])] },
      { type: 'bulletList', content: [item([{ size: 10, text: '小字' }])] },
      {
        type: 'bulletList',
        content: [item([
          { size: 14, text: '小字' },
          { size: 36, text: '混排大字' },
        ])],
      },
    ],
  })

  const items = []
  editor.state.doc.descendants((node) => {
    if (node.type.name === 'listItem') items.push(node)
  })
  assert.equal(listItemMarkerFontSize(items[0]), 36)
  assert.equal(listItemMarkerFontSize(items[1]), 10)
  assert.equal(listItemMarkerFontSize(items[2]), 36, '同段混排字号取最大值，marker 行盒才能跟住最高行')

  // 36px 正文：ratio 记录行盒高度倍率，但 marker 槽位仍保持固定 20px。
  assert.equal(markerRatio(36), 2.5714)
  assert.equal(listItemMarkerGutter(36), 20)
  assert.match(listItemMarkerStyle(36), /--dvn-marker-ratio: 2\.5714; --dvn-marker-gutter: 20px/)
  assert.equal(orderedListMarkerGutter(12, 1, 36), orderedListMarkerGutter(12, 1))

  // 10px 正文：ratio 仍记录真实字号倍率，槽位不缩到 20px 以下。
  assert.equal(markerRatio(10), 0.7143)
  assert.equal(listItemMarkerGutter(10), 20)
  assert.match(listItemMarkerStyle(10), /--dvn-marker-ratio: 0\.7143; --dvn-marker-gutter: 20px/)

  // 没有字号 mark / 非法字号都退回 14px 基准。
  assert.equal(listItemMarkerFontSize({ firstChild: null }), null)
  assert.equal(markerRatio(null), 1)
  assert.equal(markerRatio(0), 1)
  assert.equal(listItemMarkerGutter(undefined), 20)
  editor.destroy()
})

test('heading list items publish the real heading line box for marker alignment', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()

  const headingItem = (text, level) => ({
    type: 'taskItem',
    attrs: { checked: false },
    content: [{
      type: 'heading',
      attrs: { level },
      content: [{ type: 'text', text }],
    }],
  })
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [1, 2, 3, 4, 5, 6].map((level) => headingItem(`H${level}`, level)),
    }],
  })

  const items = []
  editor.state.doc.descendants((node) => {
    if (node.type.name === 'taskItem') items.push(node)
  })
  assert.deepEqual(
    items.map((item) => listItemMarkerLineHeight(item)),
    [36, 30, 27, 24, 22, 17.2],
    'H1-H6 must follow the browser line box, including H6 inheriting the body ratio',
  )
  assert.match(
    listItemMarkerStyle(null, 20, null, 36),
    /--dvn-marker-line-height: 36px/,
    'heading line height must be emitted with item marker variables',
  )

  const itemStyles = Array.from(element.querySelectorAll('ul[data-type="taskList"] > li'))
    .map((item) => item.getAttribute('style') || '')
  assert.deepEqual(
    itemStyles.map((style) => /--dvn-marker-line-height: ([\d.]+)px/.exec(style)?.[1]),
    ['36', '30', '27', '24', '22', '17.2'],
  )
  editor.destroy()
})

test('heading marker line height grows with an explicit font size', () => {
  const { editor } = createEditorWithMarkers()
  const headingItem = (level, fontSize) => ({
    type: 'taskItem',
    attrs: { checked: false },
    content: [{
      type: 'heading',
      attrs: { level },
      content: [{
        type: 'text',
        text: `H${level}`,
        marks: [{ type: 'fontSize', attrs: { fontSize: `${fontSize}px` } }],
      }],
    }],
  })

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [1, 2, 3, 4, 5].map((level) => headingItem(level, 24)),
    }],
  })
  const items = []
  editor.state.doc.descendants((node) => {
    if (node.type.name === 'taskItem') items.push(node)
  })
  assert.deepEqual(
    items.map((item) => listItemMarkerLineHeight(item)),
    [36, 36, 36, 36, 37.7143],
    'explicit 24px headings must publish the same scaled line box as CSS',
  )

  // 显式字号小于标题默认字号时，标题自身行盒仍然决定实际行距。
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [1, 2, 3, 4, 5, 6].map((level) => headingItem(level, 8)),
    }],
  })
  const smallItems = []
  editor.state.doc.descendants((node) => {
    if (node.type.name === 'taskItem') smallItems.push(node)
  })
  assert.deepEqual(
    smallItems.map((item) => listItemMarkerLineHeight(item)),
    [36, 30, 27, 24, 22, 17.2],
    '8px inline text cannot shrink the heading strut',
  )

  editor.commands.setContent({
    type: 'doc',
    content: [{ type: 'taskList', content: [headingItem(4, 36)] }],
  })
  const largeHeadingItem = editor.state.doc.firstChild.firstChild
  assert.equal(listItemMarkerLineHeight(largeHeadingItem), 54, '36px heading must scale the line box to 54px')

  editor.commands.setContent({
    type: 'doc',
    content: [{ type: 'taskList', content: [headingItem(6, 36)] }],
  })
  const largeH6 = editor.state.doc.firstChild.firstChild
  assert.equal(listItemMarkerLineHeight(largeH6), 61.92, 'H6 must follow its inherited 1.72 line box')
  editor.destroy()
})

test('ordered gutter depends on digit count, not marker font size', () => {
  setupDom()
  const { editor } = createEditorWithMarkers()

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      attrs: { start: 1 },
      content: Array.from({ length: 13 }, (_, i) => ({
        type: 'listItem',
        content: [{
          type: 'paragraph',
          content: [{
            type: 'text',
            text: `item ${i + 1}`,
            marks: [{ type: 'fontSize', attrs: { fontSize: i === 0 ? '36px' : '14px' } }],
          }],
        }],
      })),
    }],
  })

  const list = editor.view.dom.querySelector('ol')
  // 13 项 -> 最后一项 "13." -> 两位数槽位 25px；首项 36px 不再放大槽位。
  assert.match(list.getAttribute('style'), /--dvn-ordered-marker-gutter: 25px/)
  editor.destroy()
})

test('marker gutter only depends on the rendered last number', () => {
  assert.equal(orderedListMarkerGutter(1, 1), 20)
  assert.equal(orderedListMarkerGutter(9, 1), 20)
  assert.equal(orderedListMarkerGutter(10, 1), 25)
  assert.equal(orderedListMarkerGutter(3, 9), 25)
  assert.equal(orderedListMarkerGutter(100, 1), 33)
  assert.equal(orderedListMarkerGutter(0, 1), 20)
  assert.equal(orderedListMarkerGutter(undefined, undefined), 20)
  // 字号倍率不参与槽位计算：无论 14px 还是 36px，槽位只由序号位数决定。
  assert.equal(orderedListMarkerGutter(9, 1, 14), 20)
  assert.equal(orderedListMarkerGutter(9, 1, 36), 20)
  assert.equal(orderedListMarkerGutter(12, 1, 36), 25)
})

test('marker style keeps counter reset in sync with start attr', () => {
  assert.equal(normalListMarkerStyle(2, 3), '--dvn-ordered-marker-gutter: 20px; counter-reset: dvn-ordered-list 2')
  assert.equal(normalListMarkerStyle(13, 1), '--dvn-ordered-marker-gutter: 25px; counter-reset: dvn-ordered-list 0')
})

test('扁平视觉层级按可见节点分组编号，不按整个 ol 连续编号', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()
  const visualItem = (text, dvnIndentLevel) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel },
    content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
  })

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      attrs: { start: 1 },
      content: [
        visualItem('A', 0),
        visualItem('B', 1),
        visualItem('C', 2),
        visualItem('D', 2),
        visualItem('E', 0),
        visualItem('F', 1),
        visualItem('大萨达', 1),
      ],
    }],
  })

  const list = editor.state.doc.firstChild
  assert.deepEqual(orderedListMarkerNumbers(list), [1, 1, 1, 2, 2, 1, 2])
  assert.equal(
    normalListMarkerStyleForNumbers(orderedListMarkerNumbers(list), 1),
    '--dvn-ordered-marker-gutter: 20px; counter-reset: dvn-ordered-list 0',
  )

  const styles = Array.from(element.querySelectorAll('.ProseMirror ol > li'))
    .map((item) => item.getAttribute('style'))
  assert.deepEqual(
    styles.map((style) => Number(/--dvn-list-number: "(\d+)"/.exec(style)?.[1])),
    [1, 1, 1, 2, 2, 1, 2],
    '每个列表项必须收到按视觉层级计算的显式编号',
  )
  editor.destroy()
})

test('视觉子级结束返回根级时，相邻 ol 的根序号继续而不是重置', () => {
  const { editor, element } = createEditorWithMarkers()
  const item = (text, dvnIndentLevel = 0) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel },
    content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
  })
  const list = (items) => ({ type: 'orderedList', attrs: { start: 1 }, content: items })

  editor.commands.setContent({
    type: 'doc',
    content: [
      list([item('A'), item('B'), item('B1', 1)]),
      list([item('C'), item('D'), item('E')]),
    ],
  })

  const styles = Array.from(element.querySelectorAll('.ProseMirror ol > li'))
    .map((li) => li.getAttribute('style'))
  assert.equal(editor.state.doc.childCount, 2, '该用例只校验编号显示，不要求改写文档结构')
  assert.deepEqual(
    styles.map((style) => Number(/--dvn-list-number: "(\d+)"/.exec(style)?.[1])),
    [1, 2, 1, 3, 4, 5],
    'B1 的子级编号独立，C/D/E 返回根级后必须继续 3/4/5',
  )
  editor.destroy()
})

test('视觉层级编号矩阵：回退层级继续父组，进入子层重新从 1 开始', () => {
  const item = (dvnIndentLevel) => ({ attrs: { dvnIndentLevel } })
  const numbersFor = (depths, start = 1) => orderedListMarkerNumbers({
    attrs: { start },
    forEach: (callback) => depths.forEach((depth, index) => callback(item(depth), index, index)),
  })

  assert.deepEqual(numbersFor([0, 1, 2, 1, 2, 2, 0, 1, 0]), [1, 1, 1, 2, 1, 2, 2, 1, 3])
  assert.deepEqual(numbersFor([0, 0, 0], 5), [5, 6, 7])
  assert.deepEqual(numbersFor([0, 1, 0, 1], 5), [5, 1, 6, 1])
  assert.deepEqual(numbersFor([1, 1, 0, 2], 5), [1, 2, 5, 1])
})

test('带直接子列表的项缩进后按视觉层级进入子级编号组', () => {
  setupDom()
  const { editor } = createEditorWithMarkers()
  const item = (text, dvnIndentLevel, children = []) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })
  const childList = {
    type: 'orderedList',
    attrs: { start: 1 },
    content: [item('B1', 0), item('B2', 0)],
  }

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      attrs: { start: 1 },
      content: [
        item('A', 0),
        item('B', 1, [childList]),
        item('C', 0),
        item('D', 0),
      ],
    }],
  })

  const outer = editor.state.doc.firstChild
  assert.deepEqual(
    orderedListMarkerNumbers(outer),
    [1, 1, 2, 3],
    'B 视觉缩进后属于子级编号组，后续根项必须从父级剩余序号继续',
  )
  assert.deepEqual(
    orderedListMarkerNumbers(outer.child(1).lastChild),
    [1, 2],
    '子列表应独立从自己的 start 编号',
  )
  editor.destroy()
})

test('嵌套列表按真实视觉层级跨结构继续编号', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()
  const item = (text, dvnIndentLevel = 0, children = []) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })
  const ordered = (children) => ({ type: 'orderedList', attrs: { start: 1 }, content: children })

  // 用户截图同构：父项通过 dvnIndentLevel 位于视觉三级；其子列表的结构
  // 层级更深，但视觉上仍是三级，因此四项必须共用 1/2/3/4 的计数器。
  // 返回到根级的 b/c/d/e 则继续根组 2/3/4/5。
  editor.commands.setContent({
    type: 'doc',
    content: [ordered([
      item('a', 0, [ordered([
        item('adsa', 1),
        item('dsad', 1, [ordered([
          item('dsad', 0),
          item('dasd', 0),
        ])]),
      ])]),
      item('b'),
      item('c'),
      item('d'),
      item('e'),
    ])],
  })

  assert.deepEqual(renderedOrderedNumbers(element), [1, 1, 2, 3, 4, 2, 3, 4, 5])
  editor.destroy()
})

test('普通结构嵌套仍按各自视觉层级重新从 1 编号', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()
  const item = (text, children = []) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel: 0 },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      attrs: { start: 1 },
      content: [
        item('A', [{
          type: 'orderedList',
          attrs: { start: 1 },
          content: [item('A1'), item('A2')],
        }]),
        item('B'),
      ],
    }],
  })

  assert.deepEqual(renderedOrderedNumbers(element), [1, 1, 2, 2])
  editor.destroy()
})

test('视觉层级相同的子列表显式 start 能重设编号，返回父级后恢复父级计数', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()
  const item = (text, dvnIndentLevel = 0, children = []) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      attrs: { start: 1 },
      content: [
        item('A', 0, [{
          type: 'orderedList',
          attrs: { start: 4 },
          content: [item('A1'), item('A2')],
        }]),
        item('B'),
      ],
    }],
  })

  assert.deepEqual(renderedOrderedNumbers(element), [1, 4, 5, 2])
  editor.destroy()
})

test('有序/无序/待办混合嵌套时只有有序项参与编号', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()
  const item = (text, children = []) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel: 0 },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })
  const taskItem = (text, children = []) => ({
    type: 'taskItem',
    attrs: { checked: false, dvnIndentLevel: 0 },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })

  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      attrs: { start: 1 },
      content: [
        item('A'),
        item('B', [
          {
            type: 'bulletList',
            content: [item('B1', [{
              type: 'taskList',
              content: [taskItem('B1a', [{
                type: 'orderedList',
                attrs: { start: 1 },
                content: [item('B1a1'), item('B1a2')],
              }])],
            }])],
          },
          {
            type: 'bulletList',
            content: [item('B2')],
          },
        ]),
        item('C'),
      ],
    }],
  })

  assert.deepEqual(renderedOrderedNumbers(element), [1, 2, 1, 2, 3])
  editor.destroy()
})

test('取消嵌套项留下的缩进普通段落只重置所在视觉层级，根级编号继续', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()
  const item = (text, dvnIndentLevel = 0, children = []) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel },
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...children,
    ],
  })
  const ordered = (children) => ({ type: 'orderedList', attrs: { start: 1 }, content: children })

  // 取消二级 C2 后，它会变成带 dvnIndentLevel=1 的顶层普通段落。
  // 该段落应只打断视觉二级的编号组，根级 P/Q 的 1/2 不能跟着重置。
  editor.commands.setContent({
    type: 'doc',
    content: [
      ordered([item('P', 0, [ordered([item('C1')])])]),
      {
        type: 'paragraph',
        attrs: { dvnIndentLevel: 1 },
        content: [{ type: 'text', text: 'C2' }],
      },
      ordered([item('C3', 1), item('C4', 1)]),
      ordered([item('Q')]),
    ],
  })

  assert.deepEqual(
    renderedOrderedNumbers(element),
    [1, 1, 1, 2, 2],
    'P=1、C1=1、C3/C4 从 1 重新编号、Q 必须继续根级 2',
  )

  // 根级普通段落仍应重置整个编号序列，不能在根级强行续号。
  editor.commands.setContent({
    type: 'doc',
    content: [
      ordered([item('A')]),
      { type: 'paragraph', content: [{ type: 'text', text: '普通段落' }] },
      ordered([item('B')]),
    ],
  })
  assert.deepEqual(renderedOrderedNumbers(element), [1, 1])
  editor.destroy()
})

test('extension exposes the marker plugin', () => {
  assert.equal(NormalListMarkers.name, 'dvnNormalListMarkers')
  assert.equal(typeof NormalListMarkers.config.addProseMirrorPlugins, 'function')
})

test('顶层非列表块会切断两侧有序列表编号，容器内部编号也不能泄漏到后面', () => {
  setupDom()
  const { editor, element } = createEditorWithMarkers()
  const item = (text) => ({
    type: 'listItem',
    attrs: { dvnIndentLevel: 0 },
    content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
  })
  const ordered = (...texts) => ({
    type: 'orderedList',
    attrs: { start: 1 },
    content: texts.map((text) => item(text)),
  })
  const paragraph = (text) => ({
    type: 'paragraph',
    content: [{ type: 'text', text }],
  })

  editor.commands.setContent({
    type: 'doc',
    content: [
      ordered('A', 'B'),
      {
        type: 'voiceBlock',
        attrs: { voiceId: 'marker-separator', voicePath: 'voice/separator.wav', voiceSize: 1 },
      },
      ordered('C', 'D'),
    ],
  })
  assert.deepEqual(renderedOrderedNumbers(element), [1, 2, 1, 2])

  editor.commands.setContent({
    type: 'doc',
    content: [
      ordered('A', 'B'),
      { type: 'blockquote', content: [paragraph('引用'), ordered('X', 'Y')] },
      ordered('C', 'D'),
    ],
  })
  assert.deepEqual(
    renderedOrderedNumbers(element),
    [1, 2, 1, 2, 1, 2],
    '引用块内部有序列表从 1 开始，离开引用块后的根列表也必须从 1 开始',
  )
  editor.destroy()
})
