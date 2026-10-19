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
  listItemMarkerStyle,
  markerRatio,
  orderedListMarkerGutter,
  normalListMarkerStyle,
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

test('extension exposes the marker plugin', () => {
  assert.equal(NormalListMarkers.name, 'dvnNormalListMarkers')
  assert.equal(typeof NormalListMarkers.config.addProseMirrorPlugins, 'function')
})
