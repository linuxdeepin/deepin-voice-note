// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 普通有序列表用 CSS counter 自绘 marker，以便和无序/待办列表一样在行盒内
// 垂直居中。marker 槽位必须容纳 "10." 这类多位数编号，否则会压住正文，
// 所以这里按序号位数算出槽位宽度，再用 ProseMirror decoration 下发给 <ol>：
//   - decoration 由视图渲染流程下发，不会被 ProseMirror 当成外部 DOM 改动
//     回滚。早期实现直接写 element.style，会被回滚并反复触发整篇重渲染。
//   - decoration 只影响显示，不写入文档、不参与保存；CSS 侧变量未下发时
//     回退 20px，保证单数字列表排版与历史表现一致。
//
// 字号同样通过 decoration 下发：正文字号走 inline font-size mark（渲染成
// <span style="font-size: Xpx">），marker 的 ::before 挂在 <p> 上，拿不到
// span 的字号。这里读取列表项首段首个自然行的最大字号，下发：
//   --dvn-marker-ratio   字号倍率（相对 14px 基准），只用于计算首行有效行盒高度，
//                        不用于放大 marker 字形、勾选框或槽位
//   --dvn-marker-gutter  marker 槽位宽度（px，按序号位数计算，固定不随字号缩放）

import { Extension } from '@tiptap/core'
import { Plugin, PluginKey } from '@tiptap/pm/state'
import { Decoration, DecorationSet } from '@tiptap/pm/view'

// marker 字形固定 14px：数字进宽约 8px、"." 约 4px，再叠加 ::before 的
// 4px 右内边距并留 1px 余量。单个数字沿用 20px 槽位；多位数按位数加宽。
const MARKER_BASE_FONT_SIZE = 14
const MARKER_GUTTER_MIN = 20
const MARKER_GUTTER_PER_DIGIT = 8
const MARKER_GUTTER_BASE = 9
const LIST_ITEM_TYPES = new Set(['listItem', 'taskItem'])

export const normalListMarkerPluginKey = new PluginKey('dvnNormalListMarkers')

export function orderedListStart(value) {
  const start = Number(value)
  return Number.isFinite(start) ? start : 1
}

function parsePxSize(value) {
  const match = /^\s*([0-9]*\.?[0-9]+)\s*px\s*$/i.exec(String(value ?? ''))
  if (!match) return null
  const size = Number(match[1])
  return Number.isFinite(size) && size > 0 ? size : null
}

function firstParagraph(node) {
  const first = node?.firstChild
  return first?.type?.name === 'paragraph' ? first : null
}

// 列表项首段首个自然行内的最大 inline font-size（来自 fontWeight 之外的
// fontSize mark 或样式）。只取首段，遇到 hardBreak 即停止，避免把后续行、
// 后续段落的字号误当成首行行高。marker 真正跟随的是行盒高度，不是字形大小；
// 这里返回的字号只参与 --dvn-marker-ratio，CSS 再用它算出首行有效行盒高度。
export function listItemMarkerFontSize(listItem) {
  const paragraph = firstParagraph(listItem)
  if (!paragraph) return null

  let widest = null
  let stopped = false
  const visit = (node) => {
    if (!node || stopped) return
    if (node.type?.name === 'hardBreak') {
      stopped = true
      return
    }
    if (node.isText) {
      const mark = node.marks?.find((candidate) => candidate.type?.name === 'fontSize')
      const size = parsePxSize(mark?.attrs?.fontSize)
      if (size && (!widest || size > widest)) widest = size
      return
    }
    if (node.isLeaf) return
    node.forEach((child) => visit(child))
  }

  visit(paragraph)
  return widest
}

// 相对 14px 基准的字号倍率；非正数/缺失一律按 1 处理。
export function markerRatio(fontSize) {
  const size = Number.isFinite(fontSize) && fontSize > 0 ? fontSize : MARKER_BASE_FONT_SIZE
  return Number((size / MARKER_BASE_FONT_SIZE).toFixed(4))
}

// 无序/待办 marker 槽位固定为 20px。marker 字形本身不随字号变化，槽位也不该
// 跟着正文放大；多级列表的缩进因此保持稳定。
export function listItemMarkerGutter() {
  return MARKER_GUTTER_MIN
}

// 序号自增，最宽的 marker 一定出现在最后一项，因此只需要按最后一项的位数
// 计算槽位宽度；"10." 这类两位数 marker 不再溢出到正文上。槽位宽度只和
// 序号位数有关，和正文字号无关。
export function orderedListMarkerGutter(itemCount = 1, start = 1) {
  const count = Number.isFinite(itemCount) ? Math.max(0, itemCount) : 0
  const lastNumber = Math.max(1, orderedListStart(start) + count - 1)
  const digits = String(lastNumber).length
  return Math.max(MARKER_GUTTER_MIN, digits * MARKER_GUTTER_PER_DIGIT + MARKER_GUTTER_BASE)
}

export function normalListMarkerStyle(itemCount, start) {
  const resetValue = orderedListStart(start) - 1
  return `--dvn-ordered-marker-gutter: ${orderedListMarkerGutter(itemCount, start)}px; counter-reset: dvn-ordered-list ${resetValue}`
}

export function listItemMarkerStyle(fontSize, gutterPx) {
  const gutter = Number.isFinite(gutterPx) ? gutterPx : listItemMarkerGutter()
  return `--dvn-marker-ratio: ${markerRatio(fontSize)}; --dvn-marker-gutter: ${gutter}px`
}

// 自顶向下递归：进有序列表时先算出该列表的编号槽位，再把它作为
// orderedGutter 传给直接子项，保证同一列表内所有列表项的
// --dvn-marker-gutter 与列表槽位一致（网格首列 / 折行缩进都依赖它）。
function collectMarkerDecorations(node, pos, orderedGutter, decorations) {
  const name = node?.type?.name
  if (name === 'orderedList') {
    const gutter = orderedListMarkerGutter(node.childCount, node.attrs?.start)
    decorations.push(Decoration.node(pos, pos + node.nodeSize, {
      style: normalListMarkerStyle(node.childCount, node.attrs?.start),
    }))
    node.forEach((child, offset) => collectMarkerDecorations(child, pos + 1 + offset, gutter, decorations))
    return
  }
  if (name === 'bulletList' || name === 'taskList') {
    node.forEach((child, offset) => collectMarkerDecorations(child, pos + 1 + offset, null, decorations))
    return
  }
  if (LIST_ITEM_TYPES.has(name)) {
    const size = listItemMarkerFontSize(node)
    decorations.push(Decoration.node(pos, pos + node.nodeSize, {
      style: listItemMarkerStyle(size, orderedGutter ?? undefined),
    }))
    node.forEach((child, offset) => collectMarkerDecorations(child, pos + 1 + offset, null, decorations))
    return
  }
  node?.forEach?.((child, offset) => collectMarkerDecorations(child, pos + 1 + offset, null, decorations))
}

export function buildNormalListMarkerDecorations(doc) {
  const decorations = []
  doc?.forEach?.((node, offset) => collectMarkerDecorations(node, offset, null, decorations))
  return DecorationSet.create(doc, decorations)
}

export const NormalListMarkers = Extension.create({
  name: 'dvnNormalListMarkers',

  addProseMirrorPlugins() {
    return [
      new Plugin({
        key: normalListMarkerPluginKey,
        props: {
          decorations: (state) => buildNormalListMarkerDecorations(state.doc),
        },
      }),
    ]
  },
})
