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
const LIST_TYPES = new Set(['bulletList', 'orderedList', 'taskList'])

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

const LIST_ITEM_TEXTBLOCK_TYPES = new Set(['paragraph', 'heading'])
const HEADING_DEFAULT_FONT_SIZE = new Map([
  [1, 24],
  [2, 20],
  [3, 18],
  [4, 16],
  [5, 14],
  [6, 10],
])
const HEADING_LINE_HEIGHT_RATIO = new Map([
  [1, 1.5],
  [2, 1.5],
  [3, 1.5],
  [4, 1.5],
  // H5 的无单位行高保留原设计比例 22 / 14。
  [5, 1.5714285714],
  // H6 没写独立 line-height，使用 ProseMirror 正文的 1.72 倍率。
  [6, 1.72],
])

function firstTextblock(node) {
  const first = node?.firstChild
  return LIST_ITEM_TEXTBLOCK_TYPES.has(first?.type?.name) ? first : null
}

// 列表项首段首个自然行内的最大 inline font-size（来自 fontWeight 之外的
// fontSize mark 或样式）。只取首段，遇到 hardBreak 即停止，避免把后续行、
// 后续段落的字号误当成首行行高。marker 真正跟随的是行盒高度，不是字形大小；
// 这里返回的字号只参与 --dvn-marker-ratio，CSS 再用它算出首行有效行盒高度。
export function listItemMarkerFontSize(listItem) {
  const textblock = firstTextblock(listItem)
  if (!textblock) return null

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

  visit(textblock)
  return widest
}

// 标题行高使用无单位倍率，会随显式字号增长。marker 不能只看标题级别
// 写死 32/28/26...，否则在“H4 + 24px”等场景会按默认行高居中，勾选框
// 再次偏高。这里读取首行最大字号，算出与 CSS 完全一致的标题行高。
//
// 行盒高度不是“显式字号 × 倍率”，而是取行内最大字号与标题自身默认字号
// 的较大值。比如 H1 默认 24px，即使正文 mark 改成 8px，标题的 24px 行盒
// 撑杆仍然存在，实际行距依旧是 36px；若只按 8×1.5=12px 计算，marker 会
// 直接缩成半个行高并贴到顶部。
export function listItemMarkerLineHeight(listItem) {
  const textblock = firstTextblock(listItem)
  if (textblock?.type?.name !== 'heading') return null
  const level = Number(textblock.attrs?.level)
  const defaultFontSize = HEADING_DEFAULT_FONT_SIZE.get(level)
  const ratio = HEADING_LINE_HEIGHT_RATIO.get(level)
  if (!defaultFontSize || !ratio) return null
  const explicitFontSize = listItemMarkerFontSize(listItem)
  const lineBoxFontSize = Math.max(defaultFontSize, explicitFontSize ?? 0)
  return Number((lineBoxFontSize * ratio).toFixed(4))
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
  return orderedListMarkerGutterForLastNumber(lastNumber)
}

function orderedListMarkerGutterForLastNumber(lastNumber) {
  const number = Number.isFinite(lastNumber) ? Math.max(1, Math.floor(lastNumber)) : 1
  const digits = String(number).length
  return Math.max(MARKER_GUTTER_MIN, digits * MARKER_GUTTER_PER_DIGIT + MARKER_GUTTER_BASE)
}

// 列表项可能因为取消/重新应用、Tab/Shift+Tab 而暂时以“扁平列表 +
// dvnIndentLevel”的形式存在。视觉上它们仍是多级节点，序号必须按可见层级
// 分组，而不能把一整个扁平 <ol> 当成同一组连续编号。
//
// 例如截图中的 A(0) B(1) C(2) D(2) E(0) 应显示 1,1,1,2,2；
// 只有回到根层级后的 E 才继续根组编号。每进入更深一层，更深层计数器清零；
// 每回到较浅层，只重置它下面的层级，较浅层自己的编号继续递增。
function effectiveMarkerDepth(item) {
  const rawDepth = Number(item?.attrs?.dvnIndentLevel)
  return Number.isFinite(rawDepth) ? Math.max(0, Math.floor(rawDepth)) : 0
}

function markerNumberingType(listNode) {
  const type = listNode?.attrs?.type
  return !type || type === '1' ? null : type
}

// 连续的有序列表可能因为视觉缩进被暂时拆成多个 <ol>。编号状态必须能跨
// 这些块继续：例如 [A(0), B(0), B1(1)] + [C(0), D(0)]，C 返回根层级后
// 应继续为 3，而不是在新 <ol> 中重新从 1 开始。
function orderedListMarkerNumbersWithState(listNode, state) {
  const numbers = []
  const start = orderedListStart(listNode?.attrs?.start)
  const firstDepth = listNode?.firstChild ? effectiveMarkerDepth(listNode.firstChild) : null
  const type = markerNumberingType(listNode)
  const continuesPrevious = state.counters.length > 0
    && state.lastDepth != null
    && firstDepth != null
    && firstDepth <= state.lastDepth
    && start === 1
    && state.numberingType === type
  const counters = continuesPrevious ? state.counters.slice() : []

  listNode?.forEach?.((item) => {
    const depth = effectiveMarkerDepth(item)
    for (let index = depth + 1; index < counters.length; index += 1) {
      counters[index] = null
    }
    if (!Number.isFinite(counters[depth])) {
      counters[depth] = depth === 0 ? start : 1
    }
    numbers.push(counters[depth])
    counters[depth] += 1
  })

  state.counters = counters
  state.lastDepth = listNode?.childCount ? effectiveMarkerDepth(listNode.lastChild) : null
  state.numberingType = type
  return numbers
}

export function orderedListMarkerNumbers(listNode) {
  return orderedListMarkerNumbersWithState(listNode, {
    counters: [],
    lastDepth: null,
    numberingType: null,
  })
}

export function normalListMarkerStyleForNumbers(numbers, start = 1) {
  const resetValue = orderedListStart(start) - 1
  const widestNumber = numbers?.length ? Math.max(1, ...numbers) : 1
  const gutter = orderedListMarkerGutterForLastNumber(widestNumber)
  return `--dvn-ordered-marker-gutter: ${gutter}px; counter-reset: dvn-ordered-list ${resetValue}`
}

export function normalListMarkerStyle(itemCount, start) {
  const count = Number.isFinite(itemCount) ? Math.max(0, itemCount) : 0
  return normalListMarkerStyleForNumbers(
    Array.from({ length: count }, (_, index) => orderedListStart(start) + index),
    start,
  )
}

export function listItemMarkerStyle(fontSize, gutterPx, orderedNumber = null, lineHeightPx = null) {
  const gutter = Number.isFinite(gutterPx) ? gutterPx : listItemMarkerGutter()
  const numberStyle = Number.isFinite(orderedNumber)
    // content: var(...) 需要字符串 token；裸数字会在生成内容时被 CSS 视为
    // 无效类型，最终 ::before 变成 content: none，序号直接消失。
    ? `; --dvn-list-number: "${Math.max(1, Math.floor(orderedNumber))}"`
    : ''
  const lineHeightStyle = Number.isFinite(lineHeightPx) && lineHeightPx > 0
    ? `; --dvn-marker-line-height: ${lineHeightPx}px`
    : ''
  return `--dvn-marker-ratio: ${markerRatio(fontSize)}; --dvn-marker-gutter: ${gutter}px${numberStyle}${lineHeightStyle}`
}

// 编号必须在“视觉层级”上连续，而不是在每棵结构子树上各自从 1 开始。
// 典型场景：父项本来位于结构一级，通过 dvnIndentLevel 缩进到视觉三级；
// 它的有序子列表项虽然结构层级更深，但视觉层级也是三级，必须和父项共用
// 同一个计数器。反过来，普通的 1 -> 2 级嵌套仍应各自从 1 开始。
//
// 第一遍按文档顺序计算编号，第二遍生成 decoration。两遍分开是为了先知道
// 每个 <ol> 最终的最大编号（决定 marker 槽位），同时保证列表项编号严格按
// “父项 -> 它的子项 -> 下一个兄弟项”的真实阅读顺序推进。
function normalizedMarkerIndent(item) {
  const rawDepth = Number(item?.attrs?.dvnIndentLevel)
  return Number.isFinite(rawDepth) ? Math.max(0, Math.floor(rawDepth)) : 0
}

function markerVisualDepth(item, structuralDepth) {
  return Math.max(0, Math.floor(Number(structuralDepth) || 0)) + normalizedMarkerIndent(item)
}

function resetMarkerNumberingFromDepth(state, depth) {
  const from = Math.max(0, Math.floor(Number(depth) || 0))
  state.counters.length = Math.min(state.counters.length, from)
  state.types.length = Math.min(state.types.length, from)
}

function allocateOrderedMarkerNumber(state, {
  depth,
  numberingType,
  start,
  firstInList,
}) {
  // 回到较浅视觉层级时，更深层的计数必须作废；较浅层自己的计数继续。
  resetMarkerNumberingFromDepth(state, depth + 1)

  const explicitStart = orderedListStart(start)
  if (firstInList && explicitStart !== 1) {
    resetMarkerNumberingFromDepth(state, depth)
    state.counters[depth] = explicitStart
  } else if (state.types[depth] != null && state.types[depth] !== numberingType) {
    resetMarkerNumberingFromDepth(state, depth)
  }

  state.types[depth] = numberingType
  if (!Number.isFinite(state.counters[depth])) state.counters[depth] = 1

  const number = state.counters[depth]
  state.counters[depth] += 1
  return number
}

function walkMarkerNumbering(node, pos, ownerItemDepth, insideListItem, state, numbering) {
  const name = node?.type?.name
  if (LIST_TYPES.has(name)) {
    const childItemDepth = ownerItemDepth + 1
    const ordered = name === 'orderedList'
    const numbers = []
    const numberingType = ordered ? markerNumberingType(node) : null
    const start = node?.attrs?.start

    node?.forEach?.((child, offset) => {
      const childPos = pos + 1 + offset
      if (!LIST_ITEM_TYPES.has(child.type?.name)) {
        walkMarkerNumbering(child, childPos, childItemDepth, insideListItem, state, numbering)
        return
      }

      const depth = markerVisualDepth(child, childItemDepth)
      if (ordered) {
        const number = allocateOrderedMarkerNumber(state, {
          depth,
          numberingType,
          start,
          firstInList: numbers.length === 0,
        })
        numbers.push(number)
        numbering.itemNumbers.set(childPos, number)
      } else {
        // 无序/待办项在自身视觉层级打断有序序列；更深的子序列也应重新开始。
        resetMarkerNumberingFromDepth(state, depth)
      }

      walkMarkerItemContents(child, childPos, childItemDepth, state, numbering)
    })

    if (ordered) numbering.listNumbers.set(pos, numbers)
    return
  }

  // 顶层非列表块是有序编号的硬分隔符：普通文本块、语音块、图片容器、
  // 引用块等两侧的有序列表必须各自重新编号。容器块还要在离开时再次重置，
  // 否则容器内部列表的序号会泄漏给后面的根级列表。
  // 取消嵌套项后留下的普通文本块仍通过 dvnIndentLevel 保留视觉缩进；
  // 此时只重置它所在视觉层级及更深层，不能误清更浅的根级计数。
  if (!insideListItem && name !== 'doc') {
    const visualDepth = ownerItemDepth + 1 + normalizedMarkerIndent(node)
    resetMarkerNumberingFromDepth(state, visualDepth)
    if (node?.isTextblock) return
    node?.forEach?.((child, offset) => {
      walkMarkerNumbering(child, pos + 1 + offset, ownerItemDepth, false, state, numbering)
    })
    resetMarkerNumberingFromDepth(state, visualDepth)
    return
  }

  node?.forEach?.((child, offset) => {
    walkMarkerNumbering(child, pos + 1 + offset, ownerItemDepth, insideListItem, state, numbering)
  })
}

function walkMarkerItemContents(item, pos, itemDepth, state, numbering) {
  item?.forEach?.((child, offset) => {
    walkMarkerNumbering(child, pos + 1 + offset, itemDepth, true, state, numbering)
  })
}

function collectOrderedMarkerNumbering(doc) {
  const state = { counters: [], types: [] }
  const numbering = { listNumbers: new Map(), itemNumbers: new Map() }
  walkMarkerNumbering(doc, -1, 0, false, state, numbering)
  return numbering
}

function appendListItemDecorations(item, pos, itemDepth, orderedGutter, orderedNumber, numbering, decorations) {
  const size = listItemMarkerFontSize(item)
  const lineHeight = listItemMarkerLineHeight(item)
  decorations.push(Decoration.node(pos, pos + item.nodeSize, {
    style: listItemMarkerStyle(size, orderedGutter ?? undefined, orderedNumber, lineHeight),
  }))
  item?.forEach?.((child, offset) => {
    appendMarkerDecorations(
      child,
      pos + 1 + offset,
      itemDepth,
      numbering,
      decorations,
    )
  })
}

// 第二遍严格按文档树顺序下发 decoration。marker 只影响视图，不写入文档，
// 避免为了显示连续编号去合并列表或改写真实结构。
function appendMarkerDecorations(node, pos, ownerItemDepth, numbering, decorations) {
  const name = node?.type?.name
  if (name === 'orderedList') {
    const numbers = numbering.listNumbers.get(pos) ?? []
    const gutter = orderedListMarkerGutterForLastNumber(numbers.length ? Math.max(...numbers) : 1)
    decorations.push(Decoration.node(pos, pos + node.nodeSize, {
      style: normalListMarkerStyleForNumbers(numbers, node.attrs?.start),
    }))
    node.forEach((child, offset) => {
      const childPos = pos + 1 + offset
      if (LIST_ITEM_TYPES.has(child.type?.name)) {
        appendListItemDecorations(
          child,
          childPos,
          ownerItemDepth + 1,
          gutter,
          numbering.itemNumbers.get(childPos),
          numbering,
          decorations,
        )
      } else {
        appendMarkerDecorations(child, childPos, ownerItemDepth + 1, numbering, decorations)
      }
    })
    return
  }

  if (name === 'bulletList' || name === 'taskList') {
    node.forEach((child, offset) => {
      const childPos = pos + 1 + offset
      if (LIST_ITEM_TYPES.has(child.type?.name)) {
        appendListItemDecorations(
          child,
          childPos,
          ownerItemDepth + 1,
          null,
          null,
          numbering,
          decorations,
        )
      } else {
        appendMarkerDecorations(child, childPos, ownerItemDepth + 1, numbering, decorations)
      }
    })
    return
  }

  if (LIST_ITEM_TYPES.has(name)) {
    appendListItemDecorations(
      node,
      pos,
      ownerItemDepth,
      null,
      numbering.itemNumbers.get(pos),
      numbering,
      decorations,
    )
    return
  }

  node?.forEach?.((child, offset) => {
    appendMarkerDecorations(child, pos + 1 + offset, ownerItemDepth, numbering, decorations)
  })
}

export function buildNormalListMarkerDecorations(doc) {
  const decorations = []
  const numbering = collectOrderedMarkerNumbering(doc)
  appendMarkerDecorations(doc, -1, 0, numbering, decorations)
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
