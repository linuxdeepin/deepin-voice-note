// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { AllSelection, NodeSelection, Selection, TextSelection } from '@tiptap/pm/state'
import { canJoin } from '@tiptap/pm/transform'
import { taskListCascadePluginKey } from './task-list-cascade.js'

export const MAX_LIST_NESTING_DEPTH = 3
export const MAX_LIST_EXTRA_INDENT = MAX_LIST_NESTING_DEPTH - 1

const LIST_ITEM_TYPES = new Set(['listItem', 'taskItem'])
const LIST_TYPES = new Set(['bulletList', 'orderedList', 'taskList'])

// 单项反缩进后再 Tab 必须精确回到按键前。仅按内容重新挂回会丢失兄弟顺序、
// 列表边界以及 orderedList.start 等属性；以反缩进后的当前文档节点为 key，
// 保证只有“文档未被其它编辑修改”时才能使用这份快照。
const listLiftRoundTripContexts = new WeakMap()

function normalizedIndentLevel(value, maxLevel = MAX_LIST_EXTRA_INDENT) {
  const level = Number(value)
  if (!Number.isFinite(level) || level <= 0 || maxLevel <= 0) return 0
  return Math.min(maxLevel, Math.floor(level))
}

function maxExtraIndentForStructuralDepth(structuralDepth) {
  return Math.max(0, MAX_LIST_NESTING_DEPTH - structuralDepth)
}

export function activeListItemType(editor) {
  return activeListItemInfo(editor)?.typeName || null
}

export function activeListType(editor) {
  const { doc, selection } = editor.state
  if (selection.empty) return activeListItemInfo(editor)?.parentList?.type?.name || null

  // 工具栏激活态属于“整个选区”的属性。选区跨越正文与列表、或跨越
  // 无序/有序/待办列表时不是某一种列表类型，三个按钮都不能激活。
  const types = []
  doc.nodesBetween(selection.from, selection.to, (node, pos) => {
    if (!node.isTextblock) return true
    const from = pos + 1
    const to = from + node.content.size
    if (selection.from > to || selection.to < from) return true
    types.push(listTypeForTextblockAt(doc, pos))
    return false
  })
  if (types.length === 0 || types.some((type) => type === null || type !== types[0])) return null
  return types[0]
}

function listTypeForTextblockAt(doc, textblockPos) {
  const $pos = doc.resolve(Math.max(0, Math.min(doc.content.size, textblockPos + 1)))
  for (let depth = $pos.depth; depth > 0; depth -= 1) {
    const node = $pos.node(depth)
    if (!LIST_ITEM_TYPES.has(node.type.name)) continue
    const parentList = $pos.node(depth - 1)
    return LIST_TYPES.has(parentList.type.name) ? parentList.type.name : null
  }
  return null
}

function activeListItemInfo(editor) {
  const { $from } = editor.state.selection
  for (let depth = $from.depth; depth > 0; depth -= 1) {
    const node = $from.node(depth)
    if (!LIST_ITEM_TYPES.has(node.type.name)) continue

    const listNode = depth > 0 ? $from.node(depth - 1) : null
    if (!listNode || !LIST_TYPES.has(listNode.type.name)) return null
    const structuralDepth = listItemDepthAtSelection($from)
    const parentList = $from.node(depth - 1)
    const childIndex = $from.index(depth - 1)
    return {
      depth,
      node,
      pos: $from.before(depth),
      typeName: node.type.name,
      parentList,
      parentListDepth: depth - 1,
      childIndex,
      previousSibling: childIndex > 0 ? parentList.child(childIndex - 1) : null,
      structuralDepth,
      indentLevel: normalizedIndentLevel(
        node.attrs?.dvnIndentLevel,
        maxExtraIndentForStructuralDepth(structuralDepth),
      ),
    }
  }
  return null
}

function listItemDepthAtSelection($from) {
  let depth = 0
  for (let index = 1; index <= $from.depth; index += 1) {
    if (LIST_ITEM_TYPES.has($from.node(index).type.name)) depth += 1
  }
  return depth
}

function listItemDepthAtDocPos(doc, pos) {
  return listItemDepthAtSelection(doc.resolve(pos + 1))
}

// 选区覆盖的列表项：以“包含选区起点的那一项”为第一项，顺着它的同级兄弟往后收，
// 直到覆盖选区终点。
// 该项**自身那一行文本**是否落在选区里。
//
// 用"自身文本块与选区的交集"判定，而不是"选区起点是否落在该项内部"——
// 后者会把"选区从祖先项自身那行开始、一直拉到它的子孙项"这种最普通的拖选
// 判成"祖先没被选中"，于是一整批项只收上来祖先一个（用户现象：回退后选区
// 只剩第一项）。
function selectionCoversOwnTextblock(node, pos, selection) {
  const textblock = node.firstChild
  if (!textblock || !textblock.isTextblock) return false
  return selectionCoversTextblock(textblock, pos + 1, selection)
}

function selectionCoversTextblock(textblock, pos, selection) {
  if (!textblock?.isTextblock) return false
  const start = pos + 1
  const end = start + textblock.content.size
  return selection.from <= end && selection.to >= start
}

// 选区覆盖的列表项（文档顺序）。
//
// 三个坑：
//  1. 祖先项只要自身那行文本被选中，就必须收进来，同时**继续下潜**收它的子孙项
//     —— 选区从祖先那行一直拉到子孙项时，两边都在选区里。早先在收下祖先就
//     `return false` 停止下潜，子孙项一个都收不到，批量分支因此整批失效，
//     退化成"只处理光标项 + 只还原光标项那一段"（用户现象：回退后选区缩水成
//     第一项，后面几项只剩一条细缝）。
//  2. 自身那行文本没被选中的祖先（选区起点在它的子孙项里）不能收，否则会
//     把容器当成目标项（用户现象：第一项原地不动，看着比另外两项深一级）。
//  3. 不能只顺着起点所在列表往后收兄弟——选区跨层时后面的项在别的列表里。
function collectSelectionTopListItems(doc, selection) {
  if (selection.empty) return []
  const items = []

  doc.nodesBetween(selection.from, selection.to, (node, pos) => {
    if (!LIST_ITEM_TYPES.has(node.type.name)) return true
    if (!selectionCoversOwnTextblock(node, pos, selection)) return true

    const text = ownTextblockText(node)
    items.push({
      node,
      pos,
      structuralDepth: listItemDepthAtDocPos(doc, pos),
      text,
      occurrence: blockOccurrenceBefore(doc, text, pos),
    })
    return true
  })
  return items
}

function selectionTopListItems(editor) {
  const { doc, selection } = editor.state
  if (selection.empty) return []
  return collectSelectionTopListItems(doc, selection)
}

// 范围选区是否覆盖到列表项（**与光标是否落在列表项里无关**）。
//
// 全选（Ctrl+A）与拖选全文之后，文档首块很可能已经是普通段落——被反缩进
// 踢出列表的项。这时 $from 在段落里，isInListItem() 为 false，不能再拿它当
// 缩进逻辑的入口门槛，否则第一次 Shift+Tab 之后就再也按不动（用户现象：
// "后续所有顶层才会消失"卡住、连按无反应）。
export function selectionCoversListItems(editor) {
  return selectionTopListItems(editor).length > 0
}

function indentLevelOfItem(item) {
  return normalizedIndentLevel(
    item.node.attrs?.dvnIndentLevel,
    maxExtraIndentForStructuralDepth(item.structuralDepth),
  )
}

export function getActiveListNestingDepth(editor) {
  const info = activeListItemInfo(editor)
  if (!info) return 0
  return Math.min(MAX_LIST_NESTING_DEPTH, info.structuralDepth + info.indentLevel)
}

export function isInListItem(editor) {
  return getActiveListNestingDepth(editor) > 0
}

function canUseExtraIndent(info) {
  // 规则：只有"已经在二级/三级"，或"本身带着 dvnIndentLevel"（历史数据、
  // 反缩进后被拆出来的游离子树，视觉层级存在属性里）才允许继续用额外缩进
  // 补齐/恢复到最多三级。
  //
  // 不能把"顶层首项 + 前面还有别的块"当成游离项：Tab 反缩进一级项时，
  // ProseMirror 会把它的子列表拆成新的顶层列表块，该块首项靠
  // preserveDescendantListVisualDepths 写回的 dvnIndentLevel>0 就能继续
  // Tab；而"列表前面恰好有内容"（例如选中图片后转成列表、列表上面还有正文）
  // 只是普通的一级唯一项，按 B04 不得凭空缩进，否则出现悬空缩进。
  return (info.structuralDepth > 1 || info.indentLevel > 0)
    && info.indentLevel < maxExtraIndentForStructuralDepth(info.structuralDepth)
}

// ---- 块身份定位 -----------------------------------------------------------
// 文档顺序里的“块”有两种形态：列表项自身的第一个文本块，以及**不属于列表项**的
// 普通段落。必须同时认这两种，原因是：
//   1. 一级项 Shift+Tab 会退出列表变成普通段落，身份匹配要跨形态；
//   2. 老实现用 doc.descendants + `return false` 在**每一个** listItem 上停止下潜，
//      于是"甲一 > 二级项"里的二级项根本不在遍历范围里 —— 按文本找不回目标项，
//      选区还原静默失败，用户看到的就是"Shift+Tab 之后选区消失了"。
// 重复文本（用户文档里很常见）靠"第几次出现"消歧，所以序号与查找必须走同一套遍历。
function walkBlockEntries(node, pos, insideItem, acc) {
  const isItem = LIST_ITEM_TYPES.has(node.type.name)
  if (isItem) {
    acc.push({ kind: 'item', text: ownTextblockText(node), pos })
  } else if (!insideItem && node.isTextblock) {
    acc.push({ kind: 'paragraph', text: node.textContent, pos })
  }

  let offset = pos + 1
  for (let index = 0; index < node.childCount; index += 1) {
    const child = node.child(index)
    walkBlockEntries(child, offset, insideItem || isItem, acc)
    offset += child.nodeSize
  }
}

function collectBlockEntries(doc) {
  const acc = []
  walkBlockEntries(doc, -1, false, acc)
  return acc
}

function blockOccurrenceBefore(doc, text, pos) {
  let count = 0
  for (const entry of collectBlockEntries(doc)) {
    if (entry.pos >= pos) break
    if (entry.text === text) count += 1
  }
  return count
}

function findBlockEntry(doc, text, occurrence) {
  let seen = 0
  for (const entry of collectBlockEntries(doc)) {
    if (entry.text !== text) continue
    if (seen === occurrence) return entry
    seen += 1
  }
  return null
}

// 图片等原子节点用 NodeSelection 选中时，empty === false，但它并不是一段可
// 按文本偏移还原的范围选区。sinkListItem/liftListItem 变换后 ProseMirror 只会
// 把节点位置映射成 TextSelection，节点高亮因此消失。这里记录节点身份和它在
// 同属性节点中的出现序号，变换完成后重新定位并恢复 NodeSelection。
//
// 重复图片不能只按 type/attrs 找第一张，否则会把选区恢复错图片；出现序号与
// 文档顺序在列表缩进/反缩进中不会改变，足以区分重复节点。
function isAtomicNodeSelection(selection) {
  return selection instanceof NodeSelection && Boolean(selection.node?.isAtom)
}

function sameAtomicNode(node, reference) {
  return node.type === reference.type && node.eq(reference)
}

function atomicNodeOccurrence(doc, node, pos) {
  let occurrence = 0
  let done = false
  doc.descendants((candidate, candidatePos) => {
    if (done) return false
    if (!sameAtomicNode(candidate, node)) return true
    if (candidatePos >= pos) {
      done = true
      return false
    }
    occurrence += 1
    return true
  })
  return occurrence
}

function captureAtomicNodeSelection(editor) {
  const { doc, selection } = editor.state
  if (!isAtomicNodeSelection(selection)) return null
  return {
    node: selection.node,
    occurrence: atomicNodeOccurrence(doc, selection.node, selection.from),
  }
}

function findAtomicNodePosition(doc, pending) {
  let seen = 0
  let found = null
  doc.descendants((node, pos) => {
    if (found != null) return false
    if (!sameAtomicNode(node, pending.node)) return true
    if (seen === pending.occurrence) {
      found = pos
      return false
    }
    seen += 1
    return true
  })
  return found
}

function restoreAtomicNodeSelection(editor, pending) {
  if (!pending) return false
  const pos = findAtomicNodePosition(editor.state.doc, pending)
  if (pos == null) return false
  try {
    editor.commands.setNodeSelection(pos)
    return true
  } catch (_error) {
    return false
  }
}

// 选区覆盖的“块”（列表项 **以及** 不属于列表项的普通段落），文档顺序。
//
// 与 collectSelectionTopListItems 同口径，只是不丢掉普通段落。必须带上段落的原因：
// 反缩进会把一级项踢出列表变成段落，如果还原选区时只认列表项，全选后每按一次
// Shift+Tab 选区就缩水一圈（用户现象：紫色高亮越按越少，看着像选区丢了）。
function collectSelectionBlocks(doc, selection) {
  if (selection.empty) return []
  const entries = []
  for (const entry of collectBlockEntries(doc)) {
    const textblock = entryTextblock(doc, entry)
    if (!textblock) continue
    const start = textblock.pos + 1
    const end = start + textblock.node.content.size
    if (selection.from <= end && selection.to >= start) {
      // occurrence 是块身份定位的一部分（重复文本靠"第几次出现"消歧），
      // 少了它 restoreSelectionOverBlocks 会找不到目标块、选区静默塌成光标。
      entries.push({ ...entry, occurrence: blockOccurrenceBefore(doc, entry.text, entry.pos) })
    }
  }
  return entries
}

// 块自身的文本块：列表项在它的第一个子块，普通段落就是它自己。
function entryTextblock(doc, entry) {
  const pos = entry.kind === 'item' ? entry.pos + 1 : entry.pos
  const node = doc.nodeAt(pos)
  return node && node.isTextblock ? { node, pos } : null
}

// 变换后把选区重新盖回 first..last 这批块（含被踢出列表变成段落的一级项）。
function restoreSelectionOverBlocks(editor, first, last) {
  const doc = editor.state.doc
  const firstEntry = findBlockEntry(doc, first.text, first.occurrence)
  const lastEntry = findBlockEntry(doc, last.text, last.occurrence)
  if (!firstEntry || !lastEntry) return false

  const firstTextblock = entryTextblock(doc, firstEntry)
  const lastTextblock = entryTextblock(doc, lastEntry)
  if (!firstTextblock || !lastTextblock) return false
  // 只盖到"最后一个选中项**自身文本块**的末尾"，不能顺着容器节点盖到它整棵子树
  // （否则末项下面的子孙项会被连带选进来：选区比用户选的多）。
  const from = firstTextblock.pos + 1
  const to = lastTextblock.pos + 1 + lastTextblock.node.content.size
  try {
    editor.commands.setTextSelection({ from: Math.min(from, to), to: Math.max(from, to) })
    return true
  } catch (_error) {
    // 定位失败时保持当前选区，至少不影响缩进结果
    return false
  }
}

// 单选区内的一小段（光标项自身文本的一个子区间）在反缩进/缩进后原样还原。
function captureBlockTextSelection(editor, entry) {
  const { doc, selection } = editor.state
  const textblock = entryTextblock(doc, entry)
  if (!textblock) return null
  const textStart = textblock.pos + 1
  const textEnd = textblock.pos + textblock.node.nodeSize - 1
  // 两端都必须落在这一项自己的文本块里。只查起点会把"跨多项的选区"也当成
  // 项内子区间，还原时 toOffset 被夹到本项末尾，选区缩水成第一项。
  if (selection.from < textStart || selection.to > textEnd) return null
  return {
    text: entry.text,
    occurrence: blockOccurrenceBefore(doc, entry.text, entry.pos),
    fromOffset: selection.from - textStart,
    toOffset: Math.min(textblock.node.content.size, selection.to - textStart),
  }
}

// 列表项可以包含多个普通段落，光标不一定在“该项第一个段落”里。
// 结构重建时按全文文本块身份 + 出现序号记录选区，避免多段落项内光标
// 因旧坐标映射而跳到后续列表项/后续段落。
function captureActiveTextblockSelection(editor) {
  const { doc, selection } = editor.state
  const { $from } = selection
  const textblock = $from.parent
  if (!textblock?.isTextblock) return null
  const position = $from.before($from.depth)
  const textStart = position + 1
  const textEnd = textStart + textblock.content.size
  if (selection.from < textStart || selection.to > textEnd) return null
  return {
    text: textblock.textContent,
    occurrence: textblockOccurrenceBefore(doc, textblock.textContent, position),
    fromOffset: selection.from - textStart,
    toOffset: selection.to - textStart,
  }
}

function restoreTextblockSelection(editor, pending) {
  if (!pending) return false
  const position = findTextblockPosition(editor.state.doc, pending.text, pending.occurrence)
  if (position == null) return false
  const textblock = editor.state.doc.nodeAt(position)
  if (!textblock?.isTextblock) return false
  const textStart = position + 1
  const max = textblock.content.size
  const from = textStart + Math.min(Math.max(pending.fromOffset, 0), max)
  const to = textStart + Math.min(Math.max(pending.toOffset, 0), max)
  try {
    editor.commands.setTextSelection({ from: Math.min(from, to), to: Math.max(from, to) })
    return true
  } catch (_error) {
    return false
  }
}

function restoreBlockTextSelection(editor, pending) {
  if (!pending) return false
  const doc = editor.state.doc
  const entry = findBlockEntry(doc, pending.text, pending.occurrence)
  if (!entry) return false
  const textblock = entryTextblock(doc, entry)
  if (!textblock) return false
  const max = textblock.node.content.size
  const textStart = textblock.pos + 1
  const from = textStart + Math.min(Math.max(pending.fromOffset, 0), max)
  const to = textStart + Math.min(Math.max(pending.toOffset, 0), max)
  try {
    editor.commands.setTextSelection({ from: Math.min(from, to), to: Math.max(from, to) })
    return true
  } catch (_error) {
    return false
  }
}

// 范围选区下 Shift+Tab：选区覆盖的每一项各降**一级视觉层级**。
//
// 不能只交给 ProseMirror 的 liftListItem：
//  - 靠 dvnIndentLevel 做视觉缩进的项，liftListItem 完全处理不到；
//  - 选区跨“结构嵌套项 + 视觉缩进项”时（同一视觉层级可以有这两种内部表示），
//    liftListItem 会把整块拆散，未选中的项也一起掉出列表。
// 所以这里自己调度：视觉缩进项一次属性事务降级，结构嵌套项逐个（从后往前）
// 走单点提升，最后按文本把选区还原回同一批项上，保证可以连续按。
//
// 方案 A（2026-10-16 产品确认）：**一条规则贯穿** —— Shift+Tab 反缩进一级，
// 已经在一级了这一步就退出列表。范围选区里的一级项与单光标同规则，当次就退出
// （不再"等所有深层项都升到一级之后才一起消失"）。
function liftSelectedListItems(editor) {
  const outerBlocks = collectSelectionBlocks(editor.state.doc, editor.state.selection)
  const items = selectionTopListItems(editor)
  if (!items.length) return false
  // 只选中一项、且光标就在该项里：交回单项路径，保留"项内子区间选区"的还原语义。
  // 光标不在列表项里（全选/拖选后首块已被踢成段落）则必须由这里接管。
  if (items.length === 1 && activeListItemInfo(editor)) return false

  // 选中的项全都"已经在一级、且自身没有子列表"：整批一起退出列表。
  // 这种纯平列表交给 ProseMirror 一次范围 lift 完成（一个事务、一步撤销），
  // 再补一次选区还原即可，不必逐项拆事务。
  const flatTopLevel = items.length > 1
    && items.every((item) => indentLevelOfItem(item) === 0
      && item.structuralDepth === 1
      && item.node.childCount <= 1)
    && new Set(items.map((item) => item.node.type.name)).size === 1
  if (flatTopLevel) {
    const itemType = items[0].node.type.name
    // ProseMirror 的 liftListItem 范围命令要求选区**落在列表内部**；全选（Ctrl+A）
    // 或拖选全文后选区起点可能落在普通段落上（首块已被前面的反缩进踢出列表），
    // 直接调用会被 blockRange 判为不匹配而返回 false —— 表现为"第二/第三次按
    // Shift+Tab 没反应"。这里先按"项内坐标"重新播种一段覆盖这批项的选区。
    const doc = editor.state.doc
    const lastItem = items[items.length - 1]
    const firstTextblock = doc.nodeAt(items[0].pos + 1)
    const lastTextblock = doc.nodeAt(lastItem.pos + 1)
    if (firstTextblock && lastTextblock) {
      const from = items[0].pos + 2
      const to = lastItem.pos + 2 + lastTextblock.content.size
      editor.commands.setTextSelection({ from, to })
    }
    if (!editor.chain().focus().liftListItem(itemType).run()) return false
    restoreSelectionOverBlocks(editor, outerBlocks[0] ?? items[0], outerBlocks[outerBlocks.length - 1] ?? items[items.length - 1])
    return true
  }

  // 只处理"最外层的选中项"：提升一个祖先会把整棵子树一起带上去一级。
  //
  // 若继续对它的子孙项各做一次提升：① 同类型嵌套是重复降级；② 跨类型嵌套
  // （无序 > 有序 > 待办）会直接打乱文档顺序 —— 先提升最深的子孙会把它们挪进
  // 祖先项的作用域，随后提升中间项时该列表被插到整棵外层列表之后，屏幕顺序
  // 变成 C1A, C3A, C3B, C2A。选中子孙跟着祖先一起上移即已"各降一级"。
  const liftTargets = items.filter((item) => !items.some((ancestor) => (
    ancestor !== item
    && indentLevelOfItem(ancestor) === 0
    && ancestor.pos < item.pos
    && item.pos < ancestor.pos + ancestor.node.nodeSize
  )))

  const visual = []
  const structural = []
  for (const item of liftTargets) {
    const indentLevel = indentLevelOfItem(item)
    if (indentLevel > 0) visual.push({ ...item, indentLevel })
    // 已经在一级的项在**这一步就退出列表**（与单光标 Shift+Tab 一致，
    // 也与 Notion 一致）：不再"等所有深层项都升到一级之后"才处理，
    // 否则同一次按键里，一级项与深层项各走一套规则，规则不自洽。
    else structural.push(item)
  }

  // 视觉属性事务虽然不改变节点尺寸，但 appendTransaction 里的相邻列表归一
  // 仍可能合并/移动列表，使此前缓存的结构项坐标失效。按 (文本, 出现序号)
  // 重新定位本次选中的项，后续绝不能再拿旧 pos 去 resolve。
  const refreshItems = (entries) => {
    const doc = editor.state.doc
    const refreshed = []
    for (const entry of entries) {
      const block = findBlockEntry(doc, entry.text, entry.occurrence)
      if (!block || block.kind !== 'item') continue
      const node = doc.nodeAt(block.pos)
      if (!node || !LIST_ITEM_TYPES.has(node.type.name)) continue
      refreshed.push({
        node,
        pos: block.pos,
        structuralDepth: listItemDepthAtDocPos(doc, block.pos),
        text: ownTextblockText(node),
        occurrence: entry.occurrence,
      })
    }
    return refreshed
  }

  // 1) 视觉额外缩进项：只改属性。
  let currentItems = items
  if (visual.length) {
    const tr = editor.state.tr
    for (const { pos, node, indentLevel } of visual) {
      tr.setNodeMarkup(pos, undefined, { ...node.attrs, dvnIndentLevel: indentLevel - 1 })
    }
    tr.setSelection(editor.state.selection.map(tr.doc, tr.mapping))
    editor.view.dispatch(tr.scrollIntoView())

    currentItems = refreshItems(items)
    const structuralKeys = new Set(structural.map((item) => `${item.text}\u0000${item.occurrence}`))
    structural.length = 0
    for (const item of currentItems) {
      if (structuralKeys.has(`${item.text}\u0000${item.occurrence}`)) structural.push(item)
    }
  }

  // 2) 结构嵌套项：同一父列表里连续选中的兄弟，在跨类型嵌套时必须一次提升。
  //    若逐个提升，跨类型列表会被拆成多个相邻列表块，下一次 Tab 只能挂回第一块，
  //    后面的选中项留在顶层（多选往返后 A 恢复二级、B 仍是一级）。
  //    同类型列表仍保留原来的逐项路径，避免改变视觉缩进混合列表的单步语义。
  const structuralGroups = []
  const documentSize = editor.state.doc.content.size
  for (const entry of [...structural]
    .filter((item) => item.pos >= 0 && item.pos + 1 <= documentSize)
    .sort((left, right) => left.pos - right.pos)) {
    const $entry = editor.state.doc.resolve(entry.pos + 1)
    let itemDepth = -1
    for (let depth = $entry.depth; depth > 0; depth -= 1) {
      if (LIST_ITEM_TYPES.has($entry.node(depth).type.name)) {
        itemDepth = depth
        break
      }
    }
    const groupKey = itemDepth > 0
      ? `${$entry.before(itemDepth - 1)}:${$entry.node(itemDepth).type.name}`
      : `${entry.pos}:${entry.node.type.name}`
    const lastGroup = structuralGroups[structuralGroups.length - 1]
    if (lastGroup?.key === groupKey) lastGroup.items.push(entry)
    else structuralGroups.push({ key: groupKey, items: [entry], itemDepth })
  }

  const liftIndividually = (entry) => {
    const size = editor.state.doc.content.size
    if (entry.pos < 0 || entry.pos + 1 > size) return
    const inside = Selection.near(editor.state.doc.resolve(entry.pos + 1), 1).from
    editor.commands.setTextSelection(inside)
    const selectedTexts = currentItems
      .filter((item) => (
        item !== entry
        && item.pos > entry.pos
        && item.pos < entry.pos + entry.node.nodeSize
      ))
      .map((item) => item.text)
    liftSingleActiveListItem(editor, { selectedTexts })
  }

  // 从后往前处理父列表：后面的替换不会让前面组的位置失效。
  for (let groupIndex = structuralGroups.length - 1; groupIndex >= 0; groupIndex -= 1) {
    const groupSpec = structuralGroups[groupIndex]
    const group = groupSpec.items
    const first = group[0]
    if (first.pos < 0 || first.pos + 1 > editor.state.doc.content.size) continue
    const $entry = editor.state.doc.resolve(first.pos + 1)
    const innerList = groupSpec.itemDepth > 0 ? $entry.node(groupSpec.itemDepth - 1) : null
    let outerList = null
    for (let depth = groupSpec.itemDepth - 2; depth > 0; depth -= 1) {
      const ancestor = $entry.node(depth)
      if (LIST_TYPES.has(ancestor.type.name)) {
        outerList = ancestor
        break
      }
    }
    const crossTypeGroup = group.length > 1
      && innerList
      && outerList
      && innerList.type.name !== outerList.type.name

    if (!crossTypeGroup) {
      for (let index = group.length - 1; index >= 0; index -= 1) liftIndividually(group[index])
      continue
    }

    const last = group[group.length - 1]
    const doc = editor.state.doc
    const lastTextblock = doc.nodeAt(last.pos + 1)
    if (!lastTextblock) continue
    const from = first.pos + 2
    const to = last.pos + 2 + lastTextblock.content.size
    editor.commands.setTextSelection({ from, to })
    const selectedTexts = currentItems
      .filter((item) => (
        item !== first
        && item.pos > first.pos
        && item.pos < first.pos + first.node.nodeSize
      ))
      .map((item) => item.text)
    liftSingleActiveListItem(editor, { selectedTexts })
  }

  // 3) 还原覆盖同一批块的选区，保证连续 Shift+Tab 依然作用于这一组。
  //    用 outerBlocks（含已被踢出列表的段落），而不是只用列表项，避免全选后
  //    每按一次选区就缩一圈。
  restoreSelectionOverBlocks(editor, outerBlocks[0] ?? items[0], outerBlocks[outerBlocks.length - 1] ?? items[items.length - 1])
  return true
}

// 范围选区下 Tab：选区覆盖的每一项各升一级。与反缩进对称，分两种手段——
//  - 能靠 dvnIndentLevel 表达的（已有视觉缩进，或处在结构二级及以上还有余量）
//    一次属性事务统一 +1；
//  - 纯结构下沉的项，把选区收敛到这批项上交给 ProseMirror 的 sinkListItem
//    范围语义处理（逐项调用会一层套一层，结果不对）。
function sinkSelectedListItems(editor) {
  const outerBlocks = collectSelectionBlocks(editor.state.doc, editor.state.selection)
  const items = selectionTopListItems(editor)
  if (!items.length) return false
  if (items.length === 1 && activeListItemInfo(editor)) return false

  const visual = []
  const structural = []
  for (const item of items) {
    const indentLevel = indentLevelOfItem(item)
    const maxLevel = maxExtraIndentForStructuralDepth(item.structuralDepth)
    if (item.structuralDepth > 1 || indentLevel > 0) {
      if (indentLevel < maxLevel) visual.push({ ...item, indentLevel })
    } else {
      structural.push(item)
    }
  }
  if (!visual.length && !structural.length) return false

  if (visual.length) {
    const tr = editor.state.tr
    for (const { pos, node, indentLevel } of visual) {
      tr.setNodeMarkup(pos, undefined, { ...node.attrs, dvnIndentLevel: indentLevel + 1 })
    }
    tr.setSelection(editor.state.selection.map(tr.doc, tr.mapping))
    editor.view.dispatch(tr.scrollIntoView())
  }

  if (structural.length) {
    const itemType = structural[0].node.type.name
    const firstPos = structural[0].pos
    const lastEntry = structural[structural.length - 1]
    const doc = editor.state.doc
    const lastPos = lastEntry.pos
    if (firstPos <= doc.content.size && lastPos <= doc.content.size) {
      const from = Selection.near(doc.resolve(firstPos + 1), 1).from
      const lastNode = doc.nodeAt(lastPos)
      const to = lastNode
        ? Selection.near(doc.resolve(Math.min(lastPos + lastNode.nodeSize - 1, doc.content.size)), -1).from
        : from
      editor.commands.setTextSelection({ from: Math.min(from, to), to: Math.max(from, to) })
      const info = activeListItemInfo(editor)
      const detachedHandled = info
        && !editor.can().sinkListItem(itemType)
        && sinkDetachedListIntoPreviousItem(editor, info)
      if (!detachedHandled) editor.chain().focus().sinkListItem(itemType).run()
      // 批量下沉后，后续未选中兄弟可能仍靠 dvnIndentLevel 保持视觉二级。
      // 与单光标路径一样并回当前真实子列表，避免它们继续漂到三级。
      mergeFollowingVisualSiblingsIntoCurrentList(editor)
    }
  }

  // 还原覆盖同一批块的选区（含普通段落，与反缩进同口径）。
  restoreSelectionOverBlocks(editor, outerBlocks[0] ?? items[0], outerBlocks[outerBlocks.length - 1] ?? items[items.length - 1])
  return true
}

function setActiveListExtraIndent(editor, level) {
  const info = activeListItemInfo(editor)
  if (!info) return false

  const nextLevel = normalizedIndentLevel(level, maxExtraIndentForStructuralDepth(info.structuralDepth))
  if (nextLevel === info.indentLevel) return true

  // setNodeMarkup 只改属性、不改变节点尺寸，但 ProseMirror 在列表项边界附近
  // 重新计算选区时仍可能把光标推到相邻位置。视觉缩进必须保持光标/项内选区
  // 原样，尤其是 Tab/Shift+Tab 连续操作和随机组合场景。
  const pendingSelection = captureActiveTextblockSelection(editor)
  const tr = editor.state.tr.setNodeMarkup(info.pos, undefined, {
    ...info.node.attrs,
    dvnIndentLevel: nextLevel,
  })
  editor.view.dispatch(tr.scrollIntoView())
  restoreTextblockSelection(editor, pendingSelection)
  return true
}

function listAttrsForType(typeName, previousAttrs = {}) {
  if (typeName === 'orderedList') {
    return {
      start: Number.isFinite(Number(previousAttrs?.start)) ? Number(previousAttrs.start) : 1,
      type: previousAttrs?.type ?? null,
    }
  }
  return {}
}

function itemAttrsForType(typeName, previousAttrs = {}) {
  const attrs = {
    dvnIndentLevel: normalizedIndentLevel(previousAttrs?.dvnIndentLevel),
  }
  if (typeName === 'taskItem') attrs.checked = false
  return attrs
}

function createListOfType(editor, typeName, items, previousAttrs = {}, marks = []) {
  const listNodeType = editor.schema.nodes[typeName]
  if (!listNodeType || !items.length) return null
  return listNodeType.create(listAttrsForType(typeName, previousAttrs), items, marks)
}

function textContentRangeOfNode(pos, node) {
  if (!node) return null

  let from = null
  let to = null

  node.descendants((child, relativePos) => {
    if (!child.isText) return true

    const childFrom = pos + 1 + relativePos
    const childTo = childFrom + child.nodeSize
    from = from == null ? childFrom : Math.min(from, childFrom)
    to = to == null ? childTo : Math.max(to, childTo)
    return true
  })

  return from == null ? null : { from, to }
}

// 转换后各列表块的文本范围并集：选区类型跟随转换前的语义（全选仍按全选恢复）。
// 传入的文本范围是“还没合并相邻列表”时算出来的，这里只按合并步骤映射一次；
// 若在合并之后才用节点位置反推范围，映射点会落进被并进来的那一段开头，
// 只能取到该列表项自身的文本，选区会凭空缩小（选中 9~11 项只剩第 9 项高亮）。
function selectionAfterListConversion(doc, previousSelection, convertedRanges, mergeMapping) {
  if (previousSelection instanceof AllSelection) return new AllSelection(doc)

  const ranges = convertedRanges
    .map((range) => ({
      from: mergeMapping.map(range.from, -1),
      to: mergeMapping.map(range.to, 1),
    }))
    .filter((range) => range.to > range.from)
  if (!ranges.length) return null

  const clamp = (pos) => Math.max(0, Math.min(pos, doc.content.size))
  const from = clamp(Math.min(...ranges.map((range) => range.from)))
  const to = clamp(Math.max(...ranges.map((range) => range.to)))
  return TextSelection.between(doc.resolve(from), doc.resolve(to))
}

// 转换后相邻的同类型列表必须合并回一个列表：ProseMirror/Tiptap 自己的 toggleList
// 也是靠 joinListBackwards / joinListForwards 合并的。否则每次按块转换都会留下
// 多个独立的有序列表块，每块各自从 1 开始编号，表现为序号不连续。
function normalizedNumberingType(type) {
  return !type || type === '1' ? null : type
}

function listsCanMerge(left, right, { preserveOrderedStart = false } = {}) {
  if (!LIST_TYPES.has(left.type.name)
    || left.type.name !== right.type.name
    || normalizedNumberingType(left.attrs?.type) !== normalizedNumberingType(right.attrs?.type)) {
    return false
  }

  // 相邻列表只有在分界两侧的项处于同一视觉层级时才是同一个兄弟组。
  // 例如 [A(0), B(0)] + [C(0), D(0)] 必须合并成 1,2,3,4；
  // 而 [A(0)] + [B(1)] 是视觉上的父子组，不能因为同为有序列表就合并。
  const leftDepth = Number(left.lastChild?.attrs?.dvnIndentLevel) || 0
  const rightDepth = Number(right.firstChild?.attrs?.dvnIndentLevel) || 0
  if (leftDepth !== rightDepth) return false

  // 取消/局部转换会把列表拆成多段。若下一段原本从另一编号开始，
  // 直接合并会把用户没有选中的后续项重新编号；保留原 start 分界。
  if (preserveOrderedStart && left.type.name === 'orderedList') {
    const leftStart = Number(left.attrs?.start ?? 1)
    const rightStart = Number(right.attrs?.start ?? 1)
    if (leftStart + left.childCount !== rightStart) return false
  }
  return true
}

// 把右侧相邻列表挂回左侧最后一项后，整棵子树的结构层级会 +1。若其中的
// 后代原本已经到三级，仅把各项的 dvnIndentLevel 减一并不足以抵消这次结构
// 下沉（0 不能再减），会冒出四级列表。这里预估挂回后的最大视觉层级，超过
// 上限时保留“扁平列表 + dvnIndentLevel”的视觉父子关系，不强行改结构。
function maxVisualDepthAfterNestingList(node, structuralDepth) {
  if (!LIST_TYPES.has(node?.type?.name)) return 0
  let maxDepth = 0
  node.forEach((item) => {
    const currentIndent = Math.max(0, Math.floor(Number(item.attrs?.dvnIndentLevel) || 0))
    maxDepth = Math.max(maxDepth, structuralDepth + 1 + Math.max(0, currentIndent - 1))
    item.forEach((child) => {
      if (LIST_TYPES.has(child.type.name)) {
        maxDepth = Math.max(
          maxDepth,
          maxVisualDepthAfterNestingList(child, structuralDepth + 1),
        )
      }
    })
  })
  return maxDepth
}

// 局部取消/重应用后，会留下这种同一父级下的相邻列表：
//   [A(0), B(0)] [B1(1), B2(1)] [C(0), D(0)]
// 后一个列表的首项视觉更深，说明它属于前一个列表最后一项的子列表，
// 不能当成新的顶层列表，否则 C/D 会重新从 1 编号。把它挂回前一项，
// 同时把所有后代项的 dvnIndentLevel 减一，保持视觉层级不变。
export function nestAdjacentVisualLists(tr, { skipLists = null } = {}) {
  for (let round = 0; round < 100; round += 1) {
    let boundary = null
    const findBoundary = (parent, contentStart) => {
      let childPos = contentStart
      for (let index = 0; index < parent.childCount; index += 1) {
        const child = parent.child(index)
        const next = index + 1 < parent.childCount ? parent.child(index + 1) : null
        if (next && LIST_TYPES.has(child.type.name) && LIST_TYPES.has(next.type.name)) {
          // 本次工具栏操作刚重建的列表代表用户要应用/取消的目标行。
          // 即使它保留了取消前的视觉缩进，也不能在同一个操作里被自动
          // 挂到左侧项下面，否则用户看到的是“点按钮又执行了一次 Tab”。
          if (!skipLists?.has(next)) {
            const leftDepth = Number(child.lastChild?.attrs?.dvnIndentLevel) || 0
            const rightDepth = Number(next.firstChild?.attrs?.dvnIndentLevel) || 0
            const rightStructuralDepth = listItemDepthAtPosition(tr.doc, childPos + child.nodeSize + 1)
            const nextMaxVisualDepth = maxVisualDepthAfterNestingList(next, rightStructuralDepth)
            if (
              rightDepth > leftDepth
              && LIST_ITEM_TYPES.has(child.lastChild?.type?.name)
              && nextMaxVisualDepth <= MAX_LIST_NESTING_DEPTH
            ) {
              boundary = {
                left: child,
                right: next,
                leftPos: childPos,
                rightPos: childPos + child.nodeSize,
              }
              return true
            }
          }
        }
        if (findBoundary(child, childPos + 1)) return true
        childPos += child.nodeSize
      }
      return false
    }

    if (!findBoundary(tr.doc, 0)) break

    const leftItems = childNodesOf(boundary.left)
    const lastItem = leftItems[leftItems.length - 1]
    const nested = adjustListSubtreeVisualDepth(boundary.right, -1)
    const nextLastItem = lastItem.type.create(
      lastItem.attrs,
      [...childNodesOf(lastItem), nested],
      lastItem.marks,
    )
    leftItems[leftItems.length - 1] = nextLastItem
    const nextLeft = boundary.left.type.create(boundary.left.attrs, leftItems, boundary.left.marks)
    tr.replaceWith(
      boundary.leftPos,
      boundary.rightPos + boundary.right.nodeSize,
      nextLeft,
    )
  }
  return tr
}

export function joinAdjacentSameTypeLists(tr, {
  selectedLists = null,
  preserveOrderedStart = false,
} = {}) {
  const boundaries = []
  tr.doc.descendants((node, pos) => {
    if (!LIST_TYPES.has(node.type.name)) return true

    // 列表节点自身的位置就是它和前一个兄弟节点之间的分界位置，
    // 用 doc.resolve 取左侧兄弟即可，无需自己推导嵌套层级下的偏移。
    const previous = tr.doc.resolve(pos).nodeBefore
    if (!previous) return true

    const previousSelected = selectedLists?.has(previous) === true
    const currentSelected = selectedLists?.has(node) === true
    // 转换/恢复涉及本次选中段时，相邻同类型列表必须合并，并按合并后的
    // 最终位置重新编号。只有两侧都未选中时，才用 start 连续性判断它们
    // 是否本来就是同一个列表，避免擅自改写无关列表的编号。
    const keepOrderedStart = preserveOrderedStart
      && !previousSelected
      && !currentSelected
    if (listsCanMerge(previous, node, { preserveOrderedStart: keepOrderedStart })) {
      boundaries.push(pos)
    }
    return true
  })

  // 从后往前走：合并靠后的相邻列表不会让前面已算好的位置失效。
  for (const boundary of boundaries.sort((left, right) => right - left)) {
    if (canJoin(tr.doc, boundary)) tr.join(boundary)
  }
  return tr
}

function targetListItemTypeName(targetListType) {
  return targetListType === 'taskList' ? 'taskItem' : 'listItem'
}

function listAttrsFollowing(node) {
  if (node?.type?.name !== 'orderedList') return {}
  return listFragmentAttrs(node, node.childCount)
}

function wrapTextblockInList(editor, textblock, targetListType, previousAttrs = {}) {
  if (!textblock?.isTextblock) return null
  const itemTypeName = targetListItemTypeName(targetListType)
  const itemType = editor.schema.nodes[itemTypeName]
  if (!itemType) return null

  // 取消嵌套列表后，普通文本块的 dvnIndentLevel 承担“视觉层级”的存储。
  // 重新应用列表时必须把这份缩进转移到 listItem 上，否则会同时叠加
  // 文本块和列表项两份 margin，出现重复缩进。
  const itemAttrs = itemAttrsForType(itemTypeName, textblock.attrs)
  const textblockAttrs = { ...textblock.attrs, dvnIndentLevel: 0 }
  const itemTextblock = textblock.type.create(textblockAttrs, textblock.content, textblock.marks)
  const item = itemType.create(itemAttrs, [itemTextblock])
  return createListOfType(editor, targetListType, [item], previousAttrs)
}

// 把选区命中的“列表项自身那一行”作为转换单位，递归重建列表。
//
// 不能只看完整覆盖的列表节点：用户在混合类型、跨层级的列表中拖选一段文字时，
// 外层列表通常只是部分覆盖。此时只转换最内层列表会让外层仍是旧类型；若外层
// 恰好已经是目标类型，还会错误返回 false，落到 Tiptap 默认 toggleList 上，
// 表现成类似 Shift+Tab 的拆层级效果。
//
// 这里逐级处理：某个列表项自身文本被选区命中时，它和同级连续命中项组成目标
// 类型的列表段；没有命中但有子孙命中的项保持自身类型，只继续递归其子列表。
function transformListForSelection(editor, node, pos, selection, targetListType, state) {
  const originalListType = node.type.name
  const entries = []
  let childPos = pos + 1
  let hasSelectedDirectItem = false

  for (let index = 0; index < node.childCount; index += 1) {
    const item = node.child(index)
    const itemPos = childPos
    const ownSelected = LIST_ITEM_TYPES.has(item.type.name)
      && selectionCoversOwnTextblock(item, itemPos, selection)
    hasSelectedDirectItem ||= ownSelected

    const nextChildren = []
    const detachedBlocks = []
    let detachedTrailing = false
    let contentPos = itemPos + 1
    let childrenChanged = false
    for (let childIndex = 0; childIndex < item.childCount; childIndex += 1) {
      const child = item.child(childIndex)
      if (LIST_TYPES.has(child.type.name)) {
        const transformed = transformListForSelection(
          editor,
          child,
          contentPos,
          selection,
          targetListType,
          state,
        )
        if (detachedTrailing) {
          detachedBlocks.push({ node: transformed.length === 1 ? transformed[0] : child, target: false })
        } else {
          nextChildren.push(...transformed)
        }
        if (transformed.length !== 1 || !transformed[0].eq(child)) childrenChanged = true
      } else if (childIndex > 0
        && selectionCoversTextblock(child, contentPos, selection)) {
        // 列表项里的第二段及后续文本块不属于该项目自己的首行。它们不是列表项，
        // 但在文档里仍位于父项目下；工具栏按钮必须在这个位置包一层目标列表，
        // 不能把父项目本身当作列表目标（否则圆点会画到父项目那一行）。
        const previousSibling = nextChildren.at(-1)
        const previousAttrs = previousSibling?.type.name === targetListType
          ? listAttrsFollowing(previousSibling)
          : {}
        const wrapped = wrapTextblockInList(editor, child, targetListType, previousAttrs)
        if (!wrapped) return [node]
        // 当前列表项已在三级时，再在其内部包一层列表会直接产生视觉第
        // 四级。把命中的后续段落释放到当前列表项之后，作为同级的独立
        // 列表块：按钮仍然完成“应用目标列表”，但不额外缩进。
        if (listItemDepthAtPosition(editor.state.doc, itemPos) >= MAX_LIST_NESTING_DEPTH) {
          // 该段的后续段落不能留在原 item 里，否则生成顺序会变成
          // “目标段之前的文字、目标段之后的文字、目标段”。从命中位置起
          // 把 item 切开，后续内容跟随目标列表并保持原文顺序。
          detachedBlocks.push({ node: wrapped, target: true })
          detachedTrailing = true
          childrenChanged = true
        } else {
          state.selectedLists.push(wrapped)
          state.wrappedLists.push(wrapped)
          state.changed = true
          nextChildren.push(wrapped)
          childrenChanged = true
        }
        state.changed = true
      } else if (detachedTrailing) {
        detachedBlocks.push({ node: child, target: false })
      } else {
        nextChildren.push(child)
      }
      contentPos += child.nodeSize
    }

    let nextItem = childrenChanged
      ? item.type.create(item.attrs, nextChildren, item.marks)
      : item

    if (ownSelected) {
      const targetItemTypeName = targetListItemTypeName(targetListType)
      if (item.type.name !== targetItemTypeName) {
        const targetItemType = editor.schema.nodes[targetItemTypeName]
        if (!targetItemType) return [node]
        nextItem = targetItemType.create(
          itemAttrsForType(targetItemTypeName, item.attrs),
          nextItem.content,
          nextItem.marks,
        )
        state.changed = true
      }
    }

    entries.push({ node: nextItem, selected: ownSelected, detachedBlocks })
    childPos += item.nodeSize
  }

  const transformedItems = entries.map((entry) => entry.node)
  const hasDetachedBlocks = entries.some((entry) => entry.detachedBlocks.length)
  const merged = node.type.create(node.attrs, transformedItems, node.marks)

  if (!hasDetachedBlocks && targetListType === originalListType) {
    if (merged.eq(node)) {
      // 混合选区中已经属于目标类型的列表项不会再产生变更，但它们仍然属于
      // 用户最初选中的范围。必须记录原节点，否则最终只能恢复“实际换过类型”
      // 的那几段，全选后紧接着切换目标类型会只剩最后一小段被选中。
      if (hasSelectedDirectItem) state.selectedLists.push(node)
      return [node]
    }
    state.changed = true
    if (hasSelectedDirectItem) state.selectedLists.push(merged)
    return [merged]
  }

  if (!hasSelectedDirectItem && !hasDetachedBlocks
    && !entries.some((entry, index) => !entry.node.eq(node.child(index)))) {
    return [node]
  }

  // 三级 item 被切开后，目标段之后的普通段落必须继续紧跟目标列表，
  // 既不能跑回原 item，也不能排到别的后代块后面。直接挂到目标列表最后
  // 一个 item 的 content 尾部，保持文字顺序和视觉缩进。
  const materializeDetachedBlocks = (blocks) => {
    const output = []
    for (const block of blocks) {
      if (!block.target && output.length) {
        const previous = output[output.length - 1]
        const prevNode = previous.node
        if (LIST_TYPES.has(prevNode.type.name) && prevNode.lastChild
          && LIST_ITEM_TYPES.has(prevNode.lastChild.type.name)) {
          const items = childNodesOf(prevNode)
          const lastIndex = items.length - 1
          const lastItem = items[lastIndex]
          const nextLastItem = lastItem.type.create(
            lastItem.attrs,
            [...childNodesOf(lastItem), block.node],
            lastItem.marks,
          )
          items[lastIndex] = nextLastItem
          output[output.length - 1] = {
            node: prevNode.type.create(prevNode.attrs, items, prevNode.marks),
            target: previous.target,
          }
          continue
        }
      }
      output.push({ node: block.node, target: block.target })
    }
    for (const block of output) {
      if (!block.target) continue
      state.selectedLists.push(block.node)
      state.wrappedLists.push(block.node)
    }
    return output.map((block) => block.node)
  }

  // 按条目顺序生成列表段，并在被释放的后续段落之后插入独立列表块。
  // 不能等整张列表生成完再统一追加，否则用户在中间项转换时，文本顺序
  // 会从 A/B/C 变成 A/C/B。
  const result = []
  let segment = null
  const flushSegment = () => {
    if (!segment) return false
    const listNodeType = editor.schema.nodes[segment.listTypeName]
    if (!listNodeType) return false
    const attrs = segment.listTypeName === originalListType
      ? listFragmentAttrs(node, segment.startIndex, { restartOrdered: true })
      : listAttrsForType(segment.listTypeName, node.attrs)
    const created = listNodeType.create(attrs, segment.items, node.marks)
    if (segment.selected) state.selectedLists.push(created)
    result.push(created)
    segment = null
    return true
  }

  for (let index = 0; index < entries.length; index += 1) {
    const entry = entries[index]
    const listTypeName = targetListType !== originalListType && entry.selected
      ? targetListType
      : originalListType
    if (segment && segment.listTypeName !== listTypeName && !flushSegment()) return [node]
    if (!segment) {
      segment = { listTypeName, items: [], startIndex: index, selected: entry.selected }
    }
    if (entry.selected) segment.selected = true
    segment.items.push(entry.node)
    if (entry.detachedBlocks.length) {
      if (!flushSegment()) return [node]
      result.push(...materializeDetachedBlocks(entry.detachedBlocks))
    }
  }
  if (segment && !flushSegment()) return [node]

  if (result.length !== 1 || !result[0].eq(node)) state.changed = true
  return result
}

function listFragmentAttrs(node, itemOffset, { restartOrdered = false } = {}) {
  const attrs = { ...node.attrs }
  if (node.type.name === 'orderedList') {
    const start = restartOrdered
      ? 1
      : (Number.isFinite(Number(attrs.start)) ? Number(attrs.start) : 1) + itemOffset
    attrs.start = start
  }
  return attrs
}

// 嵌套列表被提升到祖父层级时，结构深度会少一层；用 dvnIndentLevel 补回
// 视觉深度，保证用户没有选中的兄弟/子树位置不漂移。递归处理是因为整段
// 子树一起被提升，所有后代都少了同一个结构祖先。
function shiftListVisualDepth(node, delta) {
  if (!LIST_TYPES.has(node.type.name) || delta === 0) return node
  const items = []
  node.forEach((item) => {
    const currentIndent = normalizedIndentLevel(
      item.attrs?.dvnIndentLevel,
      MAX_LIST_NESTING_DEPTH,
    )
    const children = []
    item.forEach((child) => {
      children.push(LIST_TYPES.has(child.type.name)
        ? shiftListVisualDepth(child, delta)
        : child)
    })
    items.push(item.type.create({
      ...item.attrs,
      dvnIndentLevel: Math.min(MAX_LIST_NESTING_DEPTH, currentIndent + delta),
    }, children, item.marks))
  })
  return node.type.create(node.attrs, items, node.marks)
}

// 工具栏的同类列表按钮是“取消应用”，不是 ProseMirror 的 toggleList。
// 后者在嵌套选区上会把子项提升到父列表里，看起来像又缩进/拆层了一次。
// 这里按文档顺序重建：选中的列表项退出列表并保留自身内容，未选中的兄弟仍留在
// 原列表中；跨过选中项后重新开一段列表，有序片段从 1 重新编号。
function cancelListSelection(editor, node, pos, selection, state) {
  const result = []
  let keptItems = []
  let keptStartOffset = 0
  let changed = false
  let itemOffset = 0

  const flushKeptItems = () => {
    if (!keptItems.length) return
    result.push(node.type.create(
      listFragmentAttrs(node, keptStartOffset, { restartOrdered: true }),
      keptItems,
      node.marks,
    ))
    keptItems = []
  }

  const keepItem = (item) => {
    if (!keptItems.length) keptStartOffset = itemOffset
    keptItems.push(item)
  }

  let childPos = pos + 1
  for (let index = 0; index < node.childCount; index += 1) {
    const item = node.child(index)
    const itemPos = childPos
    const ownSelected = LIST_ITEM_TYPES.has(item.type.name)
      && selectionCoversOwnTextblock(item, itemPos, selection)

    const processedChildren = []
    let childrenChanged = false
    let contentPos = itemPos + 1
    for (let childIndex = 0; childIndex < item.childCount; childIndex += 1) {
      const child = item.child(childIndex)
      if (LIST_TYPES.has(child.type.name)) {
        const nested = cancelListSelection(editor, child, contentPos, selection, state)
        processedChildren.push({ isList: true, ...nested })
        childrenChanged ||= nested.changed
      } else {
        processedChildren.push({ isList: false, node: child })
      }
      contentPos += child.nodeSize
    }

    if (ownSelected) {
      flushKeptItems()
      const ownTextblock = item.firstChild
      let selectedTextblock = ownTextblock
      if (ownTextblock?.isTextblock) {
        // 当前项退出列表后会落到顶层。保留它相对一级列表的视觉层级，
        // 否则三级列表中间项一取消就会直接跑到页面最左侧。
        const structuralDepth = listItemDepthAtDocPos(editor.state.doc, itemPos)
        const extraIndent = normalizedIndentLevel(
          item.attrs?.dvnIndentLevel,
          maxExtraIndentForStructuralDepth(structuralDepth),
        )
        const visualIndentLevel = Math.max(0, structuralDepth + extraIndent - 1)
        selectedTextblock = ownTextblock.type.create(
          {
            ...ownTextblock.attrs,
            dvnIndentLevel: Math.min(MAX_LIST_EXTRA_INDENT, visualIndentLevel),
          },
          ownTextblock.content,
          ownTextblock.marks,
        )
        state.selectedBlocks.push(selectedTextblock)
      }
      for (const child of processedChildren) {
        if (child.isList) {
          result.push(...child.blocks.map((block) => (
            LIST_TYPES.has(block.type.name) ? shiftListVisualDepth(block, 1) : block
          )))
        } else {
          result.push(child.node === ownTextblock ? selectedTextblock : child.node)
        }
      }
      changed = true
    } else if (childrenChanged) {
      // 祖先项自身没被选中、但子孙项被取消时，只把真正取消的文本块提出，
      // 不能把祖先项以及它前后的未选中子列表一起拍平。否则用户只取消一个
      // 待办，整棵嵌套树都会掉到顶层。
      //
      // 递归结果是一段有序的“保留列表 / 已取消文本块”。保留列表继续挂在
      // 当前祖先项下；遇到已取消文本块时先封口当前项和列表，把它提升一层。
      let itemChildren = []
      const flushCurrentItem = () => {
        if (!itemChildren.length) return
        keepItem(item.type.create(item.attrs, itemChildren, item.marks))
        flushKeptItems()
        itemChildren = []
      }
      const appendRetained = (child) => {
        // 提升点之后如果只剩子列表、没有父项自己的文本块，就把它作为独立
        // 列表块保留，避免构造出“列表开头”的非法列表项。此块同时少了一层
        // 结构祖先，必须补一级视觉缩进。
        if (!itemChildren.length && LIST_TYPES.has(child.type.name)) {
          result.push(shiftListVisualDepth(child, 1))
        } else {
          itemChildren.push(child)
        }
      }

      for (const child of processedChildren) {
        if (!child.isList) {
          itemChildren.push(child.node)
          continue
        }
        for (const block of child.blocks) {
          if (LIST_TYPES.has(block.type.name)) appendRetained(block)
          else {
            flushCurrentItem()
            result.push(block)
          }
        }
      }
      flushCurrentItem()
      changed = true
    } else {
      keepItem(item)
    }

    childPos += item.nodeSize
    itemOffset += 1
  }
  flushKeptItems()

  return { blocks: changed ? result : [node], changed }
}

function cancelSelectedListFormatting(editor, selection, cursorContext = null) {
  const { doc } = editor.state
  const state = { changed: false, selectedBlocks: [] }
  const blocks = []

  let pos = 0
  for (let index = 0; index < doc.childCount; index += 1) {
    const child = doc.child(index)
    if (LIST_TYPES.has(child.type.name)) {
      const transformed = cancelListSelection(editor, child, pos, selection, state)
      blocks.push(...transformed.blocks)
      state.changed ||= transformed.changed
    } else {
      blocks.push(child)
    }
    pos += child.nodeSize
  }
  if (!state.changed || !state.selectedBlocks.length) return false

  const nextDoc = editor.schema.topNodeType.create(doc.attrs, blocks, doc.marks)
  const tr = editor.state.tr.replaceWith(0, doc.content.size, nextDoc.content)
  const selectedRanges = state.selectedBlocks
    .map((node) => {
      const nodePos = findNodePosition(tr.doc, node)
      return nodePos == null ? null : textContentRangeOfNode(nodePos, node)
    })
    .filter(Boolean)
  // 取消嵌套项时可能为文本块补上 dvnIndentLevel，此时它是新节点，
  // 不能再拿变换前的 cursorContext.textblock 按对象身份定位。
  const nextCursorContext = cursorContext && state.selectedBlocks.length === 1
    ? { ...cursorContext, textblock: state.selectedBlocks[0] }
    : cursorContext
  if (!selectedRanges.length && !nextCursorContext) return false

  const mergeStartStep = tr.steps.length
  joinAdjacentSameTypeLists(tr, { preserveOrderedStart: true })
  const nextSelection = nextCursorContext
    ? cursorSelectionAfterTransform(tr.doc, nextCursorContext)
    : selectionAfterListConversion(
      tr.doc,
      selection,
      selectedRanges,
      tr.mapping.slice(mergeStartStep),
    )
  if (!nextSelection) return false
  tr.setSelection(nextSelection)
  // 工具栏取消列表是结构操作，不是用户点击任务复选框。任务级联插件若在本笔
  // 事务后按新的视觉父子关系重算 checked，会把未选中的祖先项一并改掉。
  tr.setMeta(taskListCascadePluginKey, true)
  editor.view.dispatch(tr.scrollIntoView())
  return true
}

function findNodePosition(doc, target) {
  let found = null
  doc.descendants((node, pos) => {
    if (found != null) return false
    if (node === target) {
      found = pos
      return false
    }
    return true
  })
  return found
}

// 列表应用/取消可能重建文本块节点（例如把段落缩进转移给 listItem），
// 对象身份无法跨变换保留。此时按“同文本第 N 次出现”恢复光标，和反缩进
// 恢复范围选区使用同一套无歧义定位方式。
function textblockOccurrenceBefore(doc, text, pos) {
  let count = 0
  doc.descendants((node, nodePos) => {
    if (nodePos >= pos) return false
    if (node.isTextblock && node.textContent === text) count += 1
    return true
  })
  return count
}

function findTextblockPosition(doc, text, occurrence) {
  let seen = 0
  let found = null
  doc.descendants((node, pos) => {
    if (found != null) return false
    if (!node.isTextblock || node.textContent !== text) return true
    if (seen === occurrence) {
      found = pos
      return false
    }
    seen += 1
    return true
  })
  return found
}

function cursorSelectionAfterTransform(doc, context) {
  let nodePos = findNodePosition(doc, context.textblock)
  if (nodePos == null && context.textblockText != null) {
    nodePos = findTextblockPosition(doc, context.textblockText, context.textblockOccurrence)
  }
  if (nodePos == null) {
    const text = context.textblock?.textContent ?? ''
    nodePos = findTextblockPosition(doc, text, context.textblockOccurrence ?? 0)
  }
  if (nodePos == null) return null
  const maxOffset = doc.nodeAt(nodePos)?.content?.size ?? context.textblock.content.size
  const offset = Math.max(0, Math.min(context.offset, maxOffset))
  return Selection.near(doc.resolve(nodePos + 1 + offset), 1)
}

function switchListTypeForSelection(editor, targetListType, selection, cursorContext = null) {
  const { doc } = editor.state
  if (!LIST_TYPES.has(targetListType)) return false

  const state = { changed: false, selectedLists: [], wrappedLists: [] }
  const blocks = []
  let pos = 0
  for (let index = 0; index < doc.childCount; index += 1) {
    const child = doc.child(index)
    if (LIST_TYPES.has(child.type.name)) {
      blocks.push(...transformListForSelection(editor, child, pos, selection, targetListType, state))
    } else if (selectionCoversTextblock(child, pos, selection)) {
      const previousBlock = blocks.at(-1)
      const previousAttrs = previousBlock?.type.name === targetListType
        ? listAttrsFollowing(previousBlock)
        : {}
      const wrapped = wrapTextblockInList(editor, child, targetListType, previousAttrs)
      if (!wrapped) blocks.push(child)
      else {
        blocks.push(wrapped)
        state.selectedLists.push(wrapped)
        state.wrappedLists.push(wrapped)
        state.changed = true
      }
    } else {
      blocks.push(child)
    }
    pos += child.nodeSize
  }

  // 选区命中的列表本来就全是目标类型：这是按钮的“取消列表”动作。
  // 不能交回默认 toggleList，它在嵌套列表中会提升子项而不是取消列表格式。
  if (!state.changed) return cancelSelectedListFormatting(editor, selection, cursorContext)

  const nextDoc = editor.schema.topNodeType.create(doc.attrs, blocks, doc.marks)
  const tr = editor.state.tr.replaceWith(0, doc.content.size, nextDoc.content)

  // 转换后仍要选中原来的整批列表项：按每个“选中段”的完整文本范围取并集。
  // 这里必须先于合并相邻同类型列表取范围，否则合并后的节点起点会吃掉前面
  // 未选中的段，导致选区向左漂移。
  const convertedRanges = state.selectedLists
    .map((node) => {
      const nodePos = findNodePosition(tr.doc, node)
      return nodePos == null ? null : textContentRangeOfNode(nodePos, node)
    })
    .filter(Boolean)
  // 空文本块没有文本范围，但折叠光标仍必须能应用/取消列表并保持光标。
  // 这种情况完全依赖 cursorContext 映射；范围选区才必须有 convertedRanges。
  if (!convertedRanges.length && !cursorContext) return false

  const mergeStartStep = tr.steps.length
  // 局部取消后重新应用时，选中的兄弟段会与被取消项的子列表、后续兄弟段
  // 形成“外层列表 / 更深列表 / 外层列表”的相邻结构。只在真正的重应用
  // 路径把更深列表挂回前一项，再合并外层同类型列表；不能在全局插件里做，
  // 否则取消操作还没完成时就会被错误归一。
  const selectedLists = new Set(state.selectedLists)
  const wrappedLists = new Set(state.wrappedLists)
  nestAdjacentVisualLists(tr, { skipLists: wrappedLists })
  joinAdjacentSameTypeLists(tr, {
    selectedLists,
    preserveOrderedStart: true,
  })
  const nextSelection = cursorContext
    ? cursorSelectionAfterTransform(tr.doc, cursorContext)
    : selectionAfterListConversion(
      tr.doc,
      selection,
      convertedRanges,
      tr.mapping.slice(mergeStartStep),
    )
  if (!nextSelection) return false
  tr.setSelection(nextSelection)
  tr.setMeta(taskListCascadePluginKey, true)
  editor.view.dispatch(tr.scrollIntoView())
  return true
}

export function switchSelectedListType(editor, targetListType) {
  const { selection } = editor.state
  if (selection.empty) return false
  return switchListTypeForSelection(editor, targetListType, selection)
}

// 折叠光标也必须走同一套显式重建。Tiptap 默认 toggleList 在“父列表项中的
// 第二个普通段落”上会把圆点画到父项目那一行，视觉上就像再次缩进/错位。
export function toggleActiveListFormatting(editor, targetListType) {
  if (restoreTopLevelLiftForListFormatting(editor, targetListType)) return true
  const { selection, doc } = editor.state
  if (!selection.empty) return switchSelectedListType(editor, targetListType)

  const { $from } = selection
  const textblock = $from.parent
  if (!textblock.isTextblock) return false

  const textblockPos = $from.before($from.depth)
  const from = textblockPos + 1
  const to = from + textblock.content.size
  return switchListTypeForSelection(
    editor,
    targetListType,
    { from, to, empty: from === to },
    {
      textblock,
      offset: selection.from - from,
      textblockText: textblock.textContent,
      textblockOccurrence: textblockOccurrenceBefore(doc, textblock.textContent, textblockPos),
    },
  )
}

function listItemSelectionOffset(info, pos = null) {
  const value = pos ?? info?.pos ?? 0
  return Math.max(1, value - info.pos)
}

function selectionOffsetRangeForInfo(editor, info) {
  return {
    from: listItemSelectionOffset(info, editor.state.selection.from),
    to: listItemSelectionOffset(info, editor.state.selection.to),
  }
}

function clampListItemOffset(offset, itemNode) {
  const maxOffset = Math.max(1, itemNode.nodeSize - 1)
  return Math.max(1, Math.min(maxOffset, offset))
}

function dispatchWithRestoredListItemSelection(editor, tr, itemPos, itemNode, offsets) {
  const from = itemPos + clampListItemOffset(offsets.from, itemNode)
  const to = itemPos + clampListItemOffset(offsets.to, itemNode)
  try {
    tr.setSelection(TextSelection.create(tr.doc, Math.min(from, to), Math.max(from, to)))
  } catch (_error) {
    tr.setSelection(TextSelection.near(tr.doc.resolve(Math.min(from, tr.doc.content.size)), 1))
  }
  editor.view.dispatch(tr.scrollIntoView())
  return true
}


function hasDirectChildList(node) {
  if (!node || !Number.isFinite(node.childCount)) return false
  for (let index = 0; index < node.childCount; index += 1) {
    if (LIST_TYPES.has(node.child(index)?.type?.name)) return true
  }
  return false
}

function childNodesOf(node) {
  const children = []
  for (let index = 0; index < node.childCount; index += 1) children.push(node.child(index))
  return children
}

// 选区终点落在内层列表里的连续项数：至少 1（即只有光标所在项）。
// Range selection 覆盖多个同级项时必须一起提升，否则“选中三项 Shift+Tab
// 只回退第一项”会在混合列表类型下再次出现。选区一旦越出当前父项就退回单项，
// 避免把不同父项的子列表混在一起处理。
function selectedInnerItemCount(editor, info) {
  const { selection, doc } = editor.state
  const $from = selection.$from
  const parentItemDepth = info.parentListDepth - 1
  const listStart = $from.before(info.parentListDepth)
  const parentItemEnd = parentItemDepth > 0 ? $from.end(parentItemDepth) : doc.content.size
  if (selection.to > parentItemEnd) return 1

  // pos 必须对齐到 info.childIndex 这一项的起点，否则会把前面的兄弟项算进来。
  let pos = listStart + 1
  for (let index = 0; index < info.childIndex; index += 1) {
    pos += info.parentList.child(index).nodeSize
  }

  let count = 0
  for (let index = info.childIndex; index < info.parentList.childCount; index += 1) {
    const item = info.parentList.child(index)
    count += 1
    if (selection.to < pos + item.nodeSize) break
    pos += item.nodeSize
  }
  return count
}

function rememberListLiftRoundTrip(
  editor,
  beforeDoc,
  targetItemPos,
  selectionOffsets,
  originalListType,
  originalItemTypeName,
) {
  if (!selectionOffsets || editor.state.doc === beforeDoc) return false
  const info = activeListItemInfo(editor)
  if (info) {
    listLiftRoundTripContexts.set(editor.state.doc, {
      beforeDoc,
      targetItemPos,
      selectionOffsets,
      postLiftTargetPos: info.pos,
      postLiftStructuralDepth: info.structuralDepth,
      postLiftIndentLevel: info.indentLevel,
      postLiftChildIndex: info.childIndex,
      postLiftParentTypeName: info.parentList.type.name,
      postLiftParentChildCount: info.parentList.childCount,
      postLiftSelectionOffset: editor.state.selection.$from.parentOffset,
      targetText: ownTextblockText(info.node),
      targetItemTypeName: info.node.type.name,
      originalListType,
    })
    return true
  }

  // 一级列表项 Shift+Tab 会退出列表变成普通文本块，此时 activeListItemInfo
  // 已经为空，但它同样属于一次可逆反缩进。记录文本块位置/内容，供用户随后
  // 点击原列表按钮恢复时精确还原，不能把前面的列表和后面的列表留成两段，
  // 导致后半段序号重新从 1 开始。
  const { selection } = editor.state
  if (!selection.empty || !(selection instanceof TextSelection)) return false
  const { $from } = selection
  const textblock = $from.parent
  if (!textblock?.isTextblock) return false
  const postLiftTargetPos = $from.before($from.depth)
  listLiftRoundTripContexts.set(editor.state.doc, {
    beforeDoc,
    targetItemPos,
    selectionOffsets,
    postLiftTargetPos,
    postLiftStructuralDepth: 0,
    postLiftIndentLevel: Number(textblock.attrs?.dvnIndentLevel) || 0,
    postLiftChildIndex: 0,
    postLiftParentTypeName: originalListType,
    postLiftParentChildCount: 0,
    targetText: textblock.textContent,
    targetItemTypeName: originalItemTypeName,
    originalListType,
    topLevelLift: true,
    postLiftSelectionOffset: $from.parentOffset,
  })
  return true
}

function restoreTopLevelLiftForListFormatting(editor, targetListType) {
  const context = listLiftRoundTripContexts.get(editor.state.doc)
  if (!context?.topLevelLift || context.originalListType !== targetListType) return false

  const { selection } = editor.state
  if (!selection.empty || !(selection instanceof TextSelection)) return false
  const { $from } = selection
  const textblock = $from.parent
  if (!textblock?.isTextblock) return false
  if ($from.before($from.depth) !== context.postLiftTargetPos) return false
  if (textblock.textContent !== context.targetText) return false
  const caretDelta = $from.parentOffset - context.postLiftSelectionOffset
  const restoredOffsets = {
    from: context.selectionOffsets.from + caretDelta,
    to: context.selectionOffsets.to + caretDelta,
  }

  const tr = editor.state.tr.replaceWith(
    0,
    editor.state.doc.content.size,
    context.beforeDoc.content,
  )
  tr.setMeta(taskListCascadePluginKey, true)
  const restoredTarget = tr.doc.nodeAt(context.targetItemPos)
  if (!restoredTarget || !LIST_ITEM_TYPES.has(restoredTarget.type.name)) return false
  return dispatchWithRestoredListItemSelection(
    editor,
    tr,
    context.targetItemPos,
    restoredTarget,
    restoredOffsets,
  )
}

function restoreListLiftRoundTrip(editor, info) {
  const selection = editor.state.selection
  if (!selection.empty || !(selection instanceof TextSelection)) return false

  const context = listLiftRoundTripContexts.get(editor.state.doc)
  if (!context || context.topLevelLift) return false
  if (info.pos !== context.postLiftTargetPos) return false
  if (info.structuralDepth !== context.postLiftStructuralDepth) return false
  if (info.indentLevel !== context.postLiftIndentLevel) return false
  if (info.childIndex !== context.postLiftChildIndex) return false
  if (info.parentList.type.name !== context.postLiftParentTypeName) return false
  if (info.parentList.childCount !== context.postLiftParentChildCount) return false
  if (info.node.type.name !== context.targetItemTypeName) return false
  if (ownTextblockText(info.node) !== context.targetText) return false

  // 反缩进后用户可能先移动光标，再按 Tab 恢复结构。整篇快照携带的是
  // 反缩进前的旧光标，直接使用会把用户刚移动的光标拉回去；按反缩进后
  // 同一文本块内的相对位移修正，既保留用户当前位置，又保留原选区宽度。
  const caretDelta = selection.$from.parentOffset - context.postLiftSelectionOffset
  const offsets = {
    from: context.selectionOffsets.from + caretDelta,
    to: context.selectionOffsets.to + caretDelta,
  }
  // 只替换反缩进时的外层列表不够：任务列表的级联插件会在这笔事务之后继续
  // 归一化祖先 taskItem 的 checked，因此它位于替换范围之外。往返恢复必须回到
  // 用户按键前的完整文档快照，否则深层结构虽然顺序恢复，祖先勾选状态仍会漂移。
  const tr = editor.state.tr.replaceWith(
    0,
    editor.state.doc.content.size,
    context.beforeDoc.content,
  )
  // 这笔事务是在恢复用户按键前的完整快照，不应再按“结构位置相同”的启发式
  // 重新级联 checked；深层项移动后，旧/新文档相同坐标可能对应不同任务项。
  tr.setMeta(taskListCascadePluginKey, true)
  const restoredTarget = tr.doc.nodeAt(context.targetItemPos)
  if (!restoredTarget || !LIST_ITEM_TYPES.has(restoredTarget.type.name)) return false

  return dispatchWithRestoredListItemSelection(editor, tr, context.targetItemPos, restoredTarget, offsets)
}

function liftMixedListItem(editor, info, options = {}) {
  const { $from } = editor.state.selection
  // 光标在目标项内部（尤其是行首）时，后面的普通坐标映射会把“本项内部偏移”
  // 错算到父项自己的文本上，最终选中父项那一行。先按文本块身份和块内偏移
  // 记住原光标，结构重建完成后精确还原。
  const pendingSelection = captureBlockTextSelection(editor, {
    kind: 'item',
    pos: info.pos,
    text: ownTextblockText(info.node),
  })
  const parentItemDepth = info.parentListDepth - 1
  const outerListDepth = info.parentListDepth - 2
  if (parentItemDepth <= 0 || outerListDepth <= 0) return false

  const parentItem = $from.node(parentItemDepth)
  const outerList = $from.node(outerListDepth)
  if (!LIST_ITEM_TYPES.has(parentItem.type.name) || !LIST_TYPES.has(outerList.type.name)) return false

  // 同类型列表也可能形成真实嵌套（混合转换、粘贴和连续缩进都会产生）。
  // ProseMirror 的 liftListItem 对这类结构可能直接返回 false，随后默认
  // Backspace 会把父项整行扩成选区。这里与跨类型嵌套共用显式重建路径，
  // 重建后的相邻同类型列表由 joinAdjacentSameTypeLists 合并。

  const parentItemIndex = $from.index(outerListDepth)
  const nestedListIndex = $from.index(parentItemDepth)
  if (parentItemIndex < 0 || parentItemIndex >= outerList.childCount) return false
  if (nestedListIndex < 0 || nestedListIndex >= parentItem.childCount) return false
  if (parentItem.child(nestedListIndex) !== info.parentList) return false

  // 提升的连续项范围：光标所在项起，到选区终点所在项为止（单选时就是 1 项）。
  const liftedCount = Math.max(1, Math.min(
    info.parentList.childCount - info.childIndex,
    selectedInnerItemCount(editor, info),
  ))
  const liftedItems = []
  for (let index = info.childIndex; index < info.childIndex + liftedCount; index += 1) {
    liftedItems.push(info.parentList.child(index))
  }
  if (!liftedItems.length) return false

  const beforeInnerItems = []
  const afterInnerItems = []
  for (let index = 0; index < info.parentList.childCount; index += 1) {
    if (index < info.childIndex) beforeInnerItems.push(info.parentList.child(index))
    if (index >= info.childIndex + liftedCount) afterInnerItems.push(info.parentList.child(index))
  }

  const beforeInnerList = createListOfType(
    editor,
    info.parentList.type.name,
    beforeInnerItems,
    info.parentList.attrs,
    info.parentList.marks,
  )
  const afterInnerList = createListOfType(
    editor,
    info.parentList.type.name,
    afterInnerItems,
    info.parentList.attrs,
    info.parentList.marks,
  )

  const modifiedParentChildren = []
  const trailingParentChildren = []
  for (let index = 0; index < parentItem.childCount; index += 1) {
    if (index === nestedListIndex) {
      if (beforeInnerList) modifiedParentChildren.push(beforeInnerList)
    } else if (index > nestedListIndex) {
      // 父项在目标子列表之后还有其它内容（例如另一种类型的子列表）时，
      // 提升目标项不能把它直接甩到这些后续内容后面。把它们按原顺序移到
      // 提升后的列表之后，并用视觉缩进补偿结构层级，文本顺序才能保持不变。
      trailingParentChildren.push(adjustListSubtreeVisualDepth(parentItem.child(index), 1))
    } else {
      modifiedParentChildren.push(parentItem.child(index))
    }
  }
  const modifiedParentItem = parentItem.type.create(parentItem.attrs, modifiedParentChildren, parentItem.marks)

  // - 当前项之前的兄弟项仍留在原父项下面；
  // - 当前项之后的兄弟项继续排在当前项之后。不能把它们塞进当前项子树，
  //   否则下一次 Tab 会把 B/C 一起带下去；
  // - 未提升的后续兄弟项结构上移到当前列表，用 dvnIndentLevel 补一级，
  //   既保持视觉层级，也让下一次 Tab 只移动当前项。
  const lastLiftedItem = liftedItems[liftedItems.length - 1]
  const activeItem = lastLiftedItem.type.create(
    lastLiftedItem.attrs,
    childNodesOf(lastLiftedItem),
    lastLiftedItem.marks,
  )
  const afterItems = afterInnerList
    ? childNodesOf(afterInnerList).map((node) => adjustListSubtreeVisualDepth(node, 1))
    : []

  const leftOuterItems = []
  for (let index = 0; index < parentItemIndex; index += 1) leftOuterItems.push(outerList.child(index))
  leftOuterItems.push(modifiedParentItem)

  const rightOuterItems = []
  for (let index = parentItemIndex + 1; index < outerList.childCount; index += 1) {
    rightOuterItems.push(outerList.child(index))
  }

  const leftOuterList = createListOfType(editor, outerList.type.name, leftOuterItems, outerList.attrs, outerList.marks)
  const activeList = createListOfType(
    editor,
    info.parentList.type.name,
    [...liftedItems.slice(0, -1), activeItem, ...afterItems],
    info.parentList.attrs,
    info.parentList.marks,
  )
  const rightOuterList = createListOfType(editor, outerList.type.name, rightOuterItems, outerList.attrs, outerList.marks)
  if (!leftOuterList || !activeList) return false

  const descendantVisualDepths = collectDescendantListVisualDepths(editor, info, options.selectedTexts)
  const replacement = [
    leftOuterList,
    activeList,
    ...trailingParentChildren,
    rightOuterList,
  ].filter(Boolean)
  const outerListPos = $from.before(outerListDepth)
  const activeListPos = outerListPos + leftOuterList.nodeSize
  const tr = editor.state.tr.replaceWith(outerListPos, outerListPos + outerList.nodeSize, replacement)

  // 选区整体平移到新的列表块上。
  //
  // 老实现拿**原文档坐标**的 selection 去减**新文档坐标**的 activeListPos，两套坐标
  // 混用后差值恒为负，被 clamp 成 1，选区于是塌到新列表第一项的边界上；边界不是行内
  // 位置，TextSelection.create 抛错 → 回退成光标。用户看到的就是"选中三项 Shift+Tab
  // 之后选区消失了"。这里改成相对"第一个被提升项"求偏移，两边同为原文档坐标。
  let beforeInnerSize = 0
  for (let index = 0; index < info.childIndex; index += 1) {
    beforeInnerSize += info.parentList.child(index).nodeSize
  }
  const firstLiftedItemPos = $from.before(info.parentListDepth) + 1 + beforeInnerSize
  const offsetFrom = Math.max(0, editor.state.selection.from - firstLiftedItemPos)
  const offsetTo = Math.max(offsetFrom, editor.state.selection.to - firstLiftedItemPos)
  const activeListStart = activeListPos + 1
  const activeListEnd = activeListPos + Math.max(1, activeList.nodeSize - 1)
  const resolveInline = (target) => {
    const $at = tr.doc.resolve(Math.max(activeListStart, Math.min(activeListEnd, target)))
    return $at.parent.inlineContent ? $at.pos : Selection.near($at, -1).from
  }
  const from = resolveInline(activeListStart + offsetFrom)
  const to = resolveInline(activeListStart + offsetTo)
  try {
    tr.setSelection(TextSelection.create(tr.doc, Math.min(from, to), Math.max(from, to)))
  } catch (_error) {
    tr.setSelection(TextSelection.near(tr.doc.resolve(activeListStart), 1))
  }
  editor.view.dispatch(tr.scrollIntoView())
  const preserved = preserveDescendantListVisualDepths(editor, descendantVisualDepths, activeListPos)
  restoreBlockTextSelection(editor, pendingSelection)
  return preserved
}

export function switchActiveListType(editor, targetListType) {
  const info = activeListItemInfo(editor)
  if (!info || !LIST_TYPES.has(targetListType)) return false
  if (info.parentList.type.name === targetListType) return false

  const targetItemTypeName = targetListType === 'taskList' ? 'taskItem' : 'listItem'
  const targetItemNodeType = editor.schema.nodes[targetItemTypeName]
  if (!targetItemNodeType) return false

  const beforeItems = []
  const afterItems = []
  let convertedItem = null

  for (let index = 0; index < info.parentList.childCount; index += 1) {
    const child = info.parentList.child(index)
    if (!LIST_ITEM_TYPES.has(child.type.name)) return false

    if (index < info.childIndex) {
      beforeItems.push(child)
    } else if (index > info.childIndex) {
      afterItems.push(child)
    } else {
      convertedItem = targetItemNodeType.create(
        itemAttrsForType(targetItemTypeName, child.attrs),
        child.content,
        child.marks,
      )
    }
  }

  if (!convertedItem) return false

  const beforeList = createListOfType(
    editor,
    info.parentList.type.name,
    beforeItems,
    info.parentList.attrs,
    info.parentList.marks,
  )
  const activeList = createListOfType(
    editor,
    targetListType,
    [convertedItem],
    info.parentList.attrs,
    info.parentList.marks,
  )
  const afterList = createListOfType(
    editor,
    info.parentList.type.name,
    afterItems,
    info.parentList.attrs,
    info.parentList.marks,
  )
  if (!activeList) return false

  const replacement = [beforeList, activeList, afterList].filter(Boolean)
  const offsets = selectionOffsetRangeForInfo(editor, info)
  const listPos = editor.state.selection.$from.before(info.parentListDepth)
  const activeItemPos = listPos + (beforeList?.nodeSize || 0) + 1
  const tr = editor.state.tr.replaceWith(listPos, listPos + info.parentList.nodeSize, replacement)

  return dispatchWithRestoredListItemSelection(editor, tr, activeItemPos, convertedItem, offsets)
}

export function canIndentActiveListItem(editor) {
  const itemType = activeListItemType(editor)
  const info = activeListItemInfo(editor)
  if (!itemType || !info || getActiveListNestingDepth(editor) >= MAX_LIST_NESTING_DEPTH) return false
  return editor.can().sinkListItem(itemType) || canUseExtraIndent(info)
}

export function canOutdentActiveListItem(editor) {
  return getActiveListNestingDepth(editor) > 1
}

export function sinkActiveListItem(editor) {
  // 范围选区：先走批量路径。**不能**先要求"光标所在的列表项"——全选/拖选全文时
  // $from 可能落在普通段落（被反缩进踢出列表的块）上，那样第二次 Tab 就失效了。
  // 原子节点选中不是文本范围选区，必须直接走单项路径，否则列表命令会把
  // NodeSelection 映射成 TextSelection，图片选中框消失。
  const pendingAtomicSelection = captureAtomicNodeSelection(editor)
  if (!pendingAtomicSelection && !editor.state.selection.empty && sinkSelectedListItems(editor)) return true
  return sinkSingleActiveListItem(editor, { pendingAtomicSelection })
}

// 跨类型反缩进后，当前项所在列表会变成前一个列表块之后的独立顶层列表。
// 例如“无序一级 > 有序 A/B”，Shift+Tab 后变成“无序一级”和“有序 A/B”两个块。
// 此时再按 Tab，ProseMirror 的 sinkListItem 因 A 是当前列表首项而直接返回 false。
// 这里把整个独立列表挂回前一个列表的最后一项；列表整体下降一层，所以同步减掉
// 各子项此前为保持视觉层级补上的 dvnIndentLevel，保证 A/B/C 的视觉位置不漂移。
function sinkDetachedListIntoPreviousItem(editor, info) {
  if (!info || info.indentLevel !== 0) return false
  if (info.childIndex !== 0) return false

  // 把整棵独立列表挂进前一项会让它所有后代的结构层级 +1。若当前子树
  // 已经达到三级，继续结构下沉必然制造第 4 层；改用当前项的视觉缩进
  // 表达这次 Tab，子树结构和视觉上限都保持不变。
  if (maxVisualDepthForListSubtree(info.parentList, info.structuralDepth + 1)
    > MAX_LIST_NESTING_DEPTH) {
    const maxExtraIndent = maxExtraIndentForStructuralDepth(info.structuralDepth)
    if (info.indentLevel >= maxExtraIndent) return true
    return setActiveListExtraIndent(editor, info.indentLevel + 1)
  }

  const { doc, selection } = editor.state
  const listPos = selection.$from.before(info.parentListDepth)
  const previousList = doc.resolve(listPos).nodeBefore
  if (!previousList || !LIST_TYPES.has(previousList.type.name)) return false
  if (previousList.type.name === info.parentList.type.name) return false
  if (!previousList.childCount) return false

  const previousLastItem = previousList.lastChild
  if (!LIST_ITEM_TYPES.has(previousLastItem.type.name)) return false

  const pendingSelection = captureActiveTextblockSelection(editor)

  const normalizedItems = childNodesOf(info.parentList)
    .map((item) => adjustListSubtreeVisualDepth(item, -1))
  const nestedList = info.parentList.type.create(
    info.parentList.attrs,
    normalizedItems,
    info.parentList.marks,
  )
  const nextLastItem = previousLastItem.type.create(
    previousLastItem.attrs,
    [...childNodesOf(previousLastItem), nestedList],
    previousLastItem.marks,
  )
  const previousItems = childNodesOf(previousList)
  previousItems[previousItems.length - 1] = nextLastItem
  const nextPreviousList = previousList.type.create(previousList.attrs, previousItems, previousList.marks)
  const previousListPos = listPos - previousList.nodeSize

  const tr = editor.state.tr.replaceWith(
    previousListPos,
    listPos + info.parentList.nodeSize,
    nextPreviousList,
  )
  editor.view.dispatch(tr.scrollIntoView())
  restoreTextblockSelection(editor, pendingSelection)
  return true
}

// 同类型列表 Tab 后，当前项已经真实下沉到父项的子列表，但此前为了保持视觉层级
// 拆到外层、带 dvnIndentLevel 的后续兄弟项仍在外面。把这段连续的视觉子项并入
// 当前项所在的真实子列表，避免最终出现“看起来二级、结构却是顶层”的混合状态。
// 子树下降一层，因此同步减一级 dvnIndentLevel；B/C 的视觉位置保持不变。
function mergeFollowingVisualSiblingsIntoCurrentList(editor) {
  const info = activeListItemInfo(editor)
  if (!info || info.structuralDepth < 2) return false

  const { $from } = editor.state.selection
  const parentItemDepth = info.parentListDepth - 1
  const outerListDepth = info.parentListDepth - 2
  if (parentItemDepth <= 0 || outerListDepth <= 0) return false

  const parentItem = $from.node(parentItemDepth)
  const outerList = $from.node(outerListDepth)
  if (!LIST_ITEM_TYPES.has(parentItem.type.name) || !LIST_TYPES.has(outerList.type.name)) return false

  const parentItemIndex = $from.index(outerListDepth)
  const nestedListIndex = $from.index(parentItemDepth)
  if (parentItemIndex < 0 || nestedListIndex < 0) return false
  if (parentItem.child(nestedListIndex) !== info.parentList) return false

  const followingItems = []
  for (let index = parentItemIndex + 1; index < outerList.childCount; index += 1) {
    const sibling = outerList.child(index)
    if (!LIST_ITEM_TYPES.has(sibling.type.name)) break
    if ((Number(sibling.attrs?.dvnIndentLevel) || 0) <= 0) break
    followingItems.push(sibling)
  }
  if (!followingItems.length) return false
  if (followingItems.some((item) => item.type !== info.node.type)) return false

  const movedItems = followingItems.map((item) => adjustListSubtreeVisualDepth(item, -1))
  const nextNestedList = info.parentList.type.create(
    info.parentList.attrs,
    [...childNodesOf(info.parentList), ...movedItems],
    info.parentList.marks,
  )
  const nextParentChildren = childNodesOf(parentItem)
  nextParentChildren[nestedListIndex] = nextNestedList
  const nextParentItem = parentItem.type.create(parentItem.attrs, nextParentChildren, parentItem.marks)

  const nextOuterItems = []
  for (let index = 0; index < outerList.childCount; index += 1) {
    if (index === parentItemIndex) nextOuterItems.push(nextParentItem)
    else if (index <= parentItemIndex || index > parentItemIndex + followingItems.length) {
      nextOuterItems.push(outerList.child(index))
    }
  }
  const nextOuterList = outerList.type.create(outerList.attrs, nextOuterItems, outerList.marks)
  const outerListPos = $from.before(outerListDepth)
  const tr = editor.state.tr.replaceWith(
    outerListPos,
    outerListPos + outerList.nodeSize,
    nextOuterList,
  )
  editor.view.dispatch(tr.scrollIntoView().setMeta('addToHistory', false))
  return true
}

function maxVisualDepthForListSubtree(list, itemStructuralDepth) {
  if (!LIST_TYPES.has(list?.type?.name)) return 0
  let maxDepth = 0
  list.forEach((item) => {
    maxDepth = Math.max(maxDepth, itemStructuralDepth + normalizedIndentLevel(
      item.attrs?.dvnIndentLevel,
      maxExtraIndentForStructuralDepth(itemStructuralDepth),
    ))
    item.forEach((child) => {
      if (LIST_TYPES.has(child.type.name)) {
        maxDepth = Math.max(
          maxDepth,
          maxVisualDepthForListSubtree(child, itemStructuralDepth + 1),
        )
      }
    })
  })
  return maxDepth
}

function maxVisualDepthForItemSubtree(item, structuralDepth) {
  let maxDepth = structuralDepth + normalizedIndentLevel(
    item?.attrs?.dvnIndentLevel,
    maxExtraIndentForStructuralDepth(structuralDepth),
  )
  item?.forEach?.((child) => {
    if (LIST_TYPES.has(child.type.name)) {
      maxDepth = Math.max(maxDepth, maxVisualDepthForListSubtree(child, structuralDepth + 1))
    }
  })
  return maxDepth
}

function sinkSingleActiveListItem(editor, options = {}) {
  const itemType = activeListItemType(editor)
  const info = activeListItemInfo(editor)
  if (!itemType || !info) return false

  // 反缩进后的第一次 Tab 优先精确还原按键前的结构。
  // 不能放在 sinkListItem 失败分支里：某些深层结构仍能被 sinkListItem 接受，
  // 但默认命令会改写父项 checked、拆分列表边界或丢失列表属性。
  if (restoreListLiftRoundTrip(editor, info)) {
    restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    return true
  }

  if (getActiveListNestingDepth(editor) >= MAX_LIST_NESTING_DEPTH) return true

  // Tab 的产品语义是“只修改当前项”，不能把当前项已有的子列表一起结构下沉。
  // 有前一个同级项时，用 dvnIndentLevel 只移动当前项；子项结构、属性和视觉深度
  // 全部保持不变。即使当前项还是一级，也有两级的视觉余量，不能套用只针对
  // 游离子树的 canUseExtraIndent() 限制。
  if (hasDirectChildList(info.node)) {
    // 当前项是列表首项时没有 previousSibling，但前面若已有另一个列表块，
    // 它仍然有合法的视觉父级上下文。跨类型列表尤其不能走 sinkDetachedList，
    // 否则会把当前项和已有子列表整体挂到前一项下面。
    const listPos = editor.state.selection.$from.before(info.parentListDepth)
    const previousBlock = editor.state.doc.resolve(listPos).nodeBefore
    const hasPreviousList = LIST_TYPES.has(previousBlock?.type?.name)
    if (info.previousSibling || hasPreviousList) {
      const maxExtraIndent = maxExtraIndentForStructuralDepth(info.structuralDepth)
      if (info.indentLevel < maxExtraIndent) {
        const indented = setActiveListExtraIndent(editor, info.indentLevel + 1)
        if (indented) restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
      }
    }
    // 已是视觉三级或没有可挂靠的前项时保持原样，绝不能落入 sinkListItem
    // 把父项及其整棵子列表一起结构下沉。
    return true
  }

  if (info.indentLevel === 0
    && !editor.can().sinkListItem(itemType)
    && sinkDetachedListIntoPreviousItem(editor, info)) {
    restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    return true
  }

  if (info.indentLevel === 0 && editor.can().sinkListItem(itemType)) {
    // 项后已有子列表时，结构性 sink 会把整棵子树再压深一层。若子树当前
    // 已到三级，优先用 dvnIndentLevel 表达本次缩进，保住子孙的三级上限；
    // 若当前层不允许额外缩进，则宁可不移动，也不能制造四级结构。
    if (
      hasDirectChildList(info.node)
      && maxVisualDepthForItemSubtree(info.node, info.structuralDepth + 1) > MAX_LIST_NESTING_DEPTH
    ) {
      if (!canUseExtraIndent(info)) return true
      const indented = setActiveListExtraIndent(editor, info.indentLevel + 1)
      if (indented) restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
      return true
    }
    const previousIndent = normalizedIndentLevel(
      info.previousSibling?.attrs?.dvnIndentLevel,
      maxExtraIndentForStructuralDepth(info.structuralDepth),
    )
    if (previousIndent > 0 && canUseExtraIndent(info)) {
      const indented = setActiveListExtraIndent(editor, previousIndent)
      if (indented) restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
      return indented
    }
    // 同反缩进：项内子区间的选区不能在 sinkListItem 之后塌掉。
    const pendingSelection = captureBlockTextSelection(editor, {
      kind: 'item',
      pos: info.pos,
      text: ownTextblockText(info.node),
    })
    const sunk = editor.chain().focus().sinkListItem(itemType).run()
    if (sunk) {
      mergeFollowingVisualSiblingsIntoCurrentList(editor)
      restoreBlockTextSelection(editor, pendingSelection)
      restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    }
    return sunk
  }

  if (canUseExtraIndent(info)) {
    const indented = setActiveListExtraIndent(editor, info.indentLevel + 1)
    if (indented) restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    return indented
  }

  return true
}


// 列表项自身的文本（第一个文本块），用于还原视觉层级时校验身份。
function ownTextblockText(node) {
  if (!node) return ''
  for (let index = 0; index < node.childCount; index += 1) {
    const child = node.child(index)
    if (child.isTextblock) return child.textContent
  }
  return ''
}

// 反缩进当前项时，ProseMirror 会把原本排在它后面的兄弟项收进当前项的子树。
// 这只是为了保持文本顺序的临时结构，不能当成“当前项本来就有这些子项”。
// 先按 (类型, 自身文本) 记住这些兄弟项，反缩进完成后再从子树末尾拆出来、
// 恢复成当前项的兄弟。这样下一次 Tab 只会下沉当前项，不会把 B/C 一起带走。
function trailingSiblingSignatures(editor, info) {
  const { doc, selection } = editor.state
  const listPos = selection.$from.before(info.parentListDepth)
  const signatures = []
  let childPos = listPos + 1
  for (let index = 0; index < info.parentList.childCount; index += 1) {
    const sibling = info.parentList.child(index)
    if (index > info.childIndex) {
      const text = ownTextblockText(sibling)
      const textStart = childPos + 2
      const textEnd = textStart + (sibling.firstChild?.content.size || 0)
      signatures.push({
        typeName: sibling.type.name,
        text,
        occurrence: blockOccurrenceBefore(doc, text, childPos),
        selected: selection.from <= textEnd && selection.to >= textStart,
      })
    }
    childPos += sibling.nodeSize
  }
  return signatures
}

function childListItems(node) {
  const items = []
  for (let index = 0; index < node.childCount; index += 1) items.push(node.child(index))
  return items
}

function signaturesMatch(items, start, signatures) {
  if (start < 0 || start + signatures.length > items.length) return false
  return signatures.every((signature, offset) => {
    const item = items[start + offset]
    return item.type.name === signature.typeName && ownTextblockText(item) === signature.text
  })
}

function findTrailingSiblingSuffix(node, signatures) {
  if (!node || !signatures.length) return null
  for (let childIndex = node.childCount - 1; childIndex >= 0; childIndex -= 1) {
    const child = node.child(childIndex)
    if (!LIST_TYPES.has(child.type.name)) continue
    const items = childListItems(child)
    const start = items.length - signatures.length
    if (signaturesMatch(items, start, signatures)) return { childIndex, child, items, start }
  }
  return null
}

// 子树整体上移/下移结构层级时，用 dvnIndentLevel 反向补偿，保持视觉层级不漂移。
function adjustListSubtreeVisualDepth(node, delta) {
  if (node.isText) return node
  const children = []
  for (let index = 0; index < node.childCount; index += 1) {
    children.push(adjustListSubtreeVisualDepth(node.child(index), delta))
  }
  if (!LIST_ITEM_TYPES.has(node.type.name)) {
    return node.type.create(node.attrs, children, node.marks)
  }
  const current = Number(node.attrs?.dvnIndentLevel) || 0
  return node.type.create({
    ...node.attrs,
    dvnIndentLevel: Math.max(0, current + delta),
  }, children, node.marks)
}

function separateTrailingSiblingsFromItem(editor, itemNode, itemPos, signatures) {
  const suffix = findTrailingSiblingSuffix(itemNode, signatures)
  if (!suffix) return false

  const keptItems = suffix.items.slice(0, suffix.start)
  const movedItems = suffix.items.slice(suffix.start)
    .map((node) => adjustListSubtreeVisualDepth(node, 1))
  if (!movedItems.length) return true

  // 同类型列表的下沉只允许移动当前项。跨类型的独立列表由另一条路径处理，
  // 这里若强行把不同 item 类型塞进父列表会破坏 schema 内容表达式。
  const activeItemType = itemNode.type
  if (movedItems.some((item) => item.type !== activeItemType)) return true

  const nextChildren = []
  for (let index = 0; index < itemNode.childCount; index += 1) {
    if (index !== suffix.childIndex) {
      nextChildren.push(itemNode.child(index))
    } else if (keptItems.length) {
      nextChildren.push(suffix.child.type.create(suffix.child.attrs, keptItems, suffix.child.marks))
    }
  }
  const nextActiveItem = itemNode.type.create(itemNode.attrs, nextChildren, itemNode.marks)

  // 选区仍映射在原项里；原兄弟项作为该项之后的同级项插回父列表。
  // 追加事务不进历史，撤销仍然一步完成。
  const tr = editor.state.tr.replaceWith(
    itemPos,
    itemPos + itemNode.nodeSize,
    [nextActiveItem, ...movedItems],
  )
  editor.view.dispatch(tr.scrollIntoView().setMeta('addToHistory', false))
  return true
}

function separateTrailingSiblingsAfterLift(editor, signatures) {
  if (!signatures.length) return true
  const info = activeListItemInfo(editor)
  if (!info) return true

  // 光标只提升当前项时，临时收养的后续兄弟在 active item 的子树里。
  if (separateTrailingSiblingsFromItem(editor, info.node, info.pos, signatures)) return true

  // 范围提升时，ProseMirror 会把未选中的尾部兄弟并进“最后一个被提升项”的子树，
  // 而当前 activeItemInfo 仍是选区第一项。按提升前记录的 selected 标记找到最后
  // 一个被提升项，再从它的子树尾部拆出未选中兄弟。
  const doc = editor.state.doc
  for (let index = signatures.length - 1; index >= 0; index -= 1) {
    const signature = signatures[index]
    if (!signature.selected) continue
    const entry = findBlockEntry(doc, signature.text, signature.occurrence)
    if (!entry || entry.kind !== 'item') continue
    const itemNode = doc.nodeAt(entry.pos)
    const trailing = signatures.slice(index + 1)
    if (itemNode && findTrailingSiblingSuffix(itemNode, trailing)) {
      return separateTrailingSiblingsFromItem(editor, itemNode, entry.pos, trailing)
    }
  }
  return true
}

// 收集被提升项的子孙列表项的"视觉层级"记录，供提升后按原样还原。
//
// 额外记录 `selected`：**正在被反缩进的选中项不能进保持名单**。提升祖先会让整棵
// 子树的结构层级 -1，选中子孙的视觉层级因此自动 -1（正是我们想要的"各降一级"）；
// 如果还把它们的原视觉层级写回去，就会出现"选中了却没动"。未选中的子孙才需要保持。
// selectedTexts：**按文档顺序**排好的"选中项自身文本"序列（只含本项子树里的）。
// 批量反缩进会在每次提升前把选区塌成光标，所以不能读 editor.state.selection 判断
// "这个子孙是不是也被选中"；用文本序列按序遍历对齐即可（重复文本也没问题，
// 因为两侧都是文档顺序）。为 null 表示不是批量场景，全部按"未选中"处理（保持原行为）。
function collectDescendantListVisualDepths(editor, info, selectedTexts = null) {
  const visualDepths = []
  let selectedCursor = 0

  function walk(node, basePos, structuralDepth) {
    if (!node || !node.childCount) return
    let offset = 1
    for (let index = 0; index < node.childCount; index += 1) {
      const child = node.child(index)
      const childPos = basePos + offset
      const isListItem = LIST_ITEM_TYPES.has(child.type.name)
      const childStructuralDepth = isListItem ? structuralDepth + 1 : structuralDepth
      if (isListItem) {
        const text = ownTextblockText(child)
        const selected = Boolean(selectedTexts) && selectedTexts[selectedCursor] === text
        if (selected) selectedCursor += 1
        visualDepths.push({
          visualDepth: Math.min(
            MAX_LIST_NESTING_DEPTH,
            childStructuralDepth + normalizedIndentLevel(
              child.attrs?.dvnIndentLevel,
              maxExtraIndentForStructuralDepth(childStructuralDepth),
            ),
          ),
          text,
          selected,
        })
      }
      walk(child, childPos, childStructuralDepth)
      offset += child.nodeSize
    }
  }

  walk(info.node, info.pos, info.structuralDepth)
  return visualDepths
}

function setListItemVisualDepthInTransaction(tr, node, pos, structuralDepth, desiredVisualDepth) {
  const nextIndentLevel = normalizedIndentLevel(
    desiredVisualDepth - structuralDepth,
    maxExtraIndentForStructuralDepth(structuralDepth),
  )
  if ((Number(node.attrs?.dvnIndentLevel) || 0) === nextIndentLevel) return false

  tr.setNodeMarkup(pos, undefined, {
    ...node.attrs,
    dvnIndentLevel: nextIndentLevel,
  })
  return true
}

// 反缩进把一级项踢出列表时，它原来的子列表会紧跟在“该项变成的那个顶层块”后面。
// 用这个位置当锚点，才能准确落在那批后代项上；旧实现用选区末端当锚点，范围选区下
// 选区落点会漂到后代项之后，于是把后面**无关的兄弟列表项**也按记录值改了缩进
// （用户表现：选中三项连续 Shift+Tab，未选中的丙三/丁四被缩进成三级）。
function liftFallbackAnchor(editor) {
  const { doc, selection } = editor.state
  const target = Math.max(0, Math.min(selection.from, doc.content.size))
  try {
    const $from = doc.resolve(target)
    if ($from.depth >= 1) return $from.after(1)
  } catch (_error) {
    // 落到文档边界时退回原行为
  }
  return Math.max(selection.from, selection.to)
}

function preserveListVisualDepthsAfterPosition(editor, visualDepths, fromPos) {
  let index = 0
  let tr = null

  editor.state.doc.descendants((node, pos) => {
    if (index >= visualDepths.length) return false
    if (!LIST_ITEM_TYPES.has(node.type.name)) return true
    if (pos < fromPos) return true

    // 记录值只对“确实是记录下来的那批后代项”有效。锚点漂移时会先碰到无关的
    // 兄弟项，文本对不上就立刻停手：宁可少补一次视觉层级，也不能把无关列表项改掉。
    if (visualDepths[index].text !== ownTextblockText(node)) return false

    // 与 preserveDescendantListVisualDepths 同口径：正在被反缩进的选中项不能再按
    // 原视觉层级写回，否则"选中了却不降级"（跨类型嵌套切线最容易踩到）。
    if (!visualDepths[index].selected) {
      tr ||= editor.state.tr
      const structuralDepth = listItemDepthAtPosition(editor.state.doc, pos)
      setListItemVisualDepthInTransaction(tr, node, pos, structuralDepth, visualDepths[index].visualDepth)
    }
    index += 1
    return true
  })

  if (tr?.docChanged) editor.view.dispatch(tr.scrollIntoView())
  return true
}

function listItemDepthAtPosition(doc, pos) {
  const $pos = doc.resolve(Math.min(pos + 1, doc.content.size))
  let depth = 0
  for (let index = 1; index <= $pos.depth; index += 1) {
    if (LIST_ITEM_TYPES.has($pos.node(index).type.name)) depth += 1
  }
  return depth
}

function preserveDescendantListVisualDepths(editor, visualDepths, fallbackFromPos = 0) {
  if (!visualDepths.length) return true

  const info = activeListItemInfo(editor)
  if (!info) return preserveListVisualDepthsAfterPosition(editor, visualDepths, fallbackFromPos)

  let index = 0
  let tr = null

  function walk(node, basePos, structuralDepth) {
    if (!node || !node.childCount) return

    let offset = 1
    for (let childIndex = 0; childIndex < node.childCount; childIndex += 1) {
      const child = node.child(childIndex)
      const childPos = basePos + offset
      const isListItem = LIST_ITEM_TYPES.has(child.type.name)
      const childStructuralDepth = isListItem ? structuralDepth + 1 : structuralDepth
      if (isListItem && index < visualDepths.length) {
        // 被选中的子孙项正在反缩进，结构层级已经各降一级，不能再按原视觉层级写回。
        if (!visualDepths[index].selected) {
          tr ||= editor.state.tr
          setListItemVisualDepthInTransaction(
            tr,
            child,
            childPos,
            childStructuralDepth,
            visualDepths[index].visualDepth,
          )
        }
        index += 1
      }
      walk(child, childPos, childStructuralDepth)
      offset += child.nodeSize
    }
  }

  walk(info.node, info.pos, info.structuralDepth)
  if (tr?.docChanged) editor.view.dispatch(tr.scrollIntoView())
  return true
}

// 深度优先找节点里最后一个叶子文本块（列表项末尾可能挂着嵌套子列表）。
function lastTextblockInNode(node) {
  if (!node) return null
  if (node.isTextblock) return node
  for (let index = node.childCount - 1; index >= 0; index -= 1) {
    const found = lastTextblockInNode(node.child(index))
    if (found) return found
  }
  return null
}

// 光标在“紧跟在列表块之后的普通段落”行首按 Backspace：
// ProseMirror 默认的 joinBackward 会把这个段落重新包成列表项，于是出现
// “退出列表 → 又变回列表项”的死循环（用户表现为删不掉这一行、列表项又冒出来）。
// 这里按文本编辑器的通用语义处理：删掉段落间隔，把该段落的文字并入前一个列表项
// 的最后一段（段落本身为空即等于删除这一空行），光标停在并入位置。
// 只处理“上一块是列表”的情况，段落跟在普通段落后仍交回默认行为。
export function joinParagraphBackwardIntoPreviousList(editor) {
  const { selection } = editor.state
  if (!selection.empty) return false

  const $from = selection.$from
  const paragraph = $from.parent
  if (!paragraph.isTextblock || paragraph.type.name !== 'paragraph') return false
  if ($from.parentOffset !== 0) return false

  const depth = $from.depth
  const parent = $from.node(depth - 1)
  const index = $from.index(depth - 1)
  if (index <= 0) return false

  const previous = parent.child(index - 1)
  if (!LIST_TYPES.has(previous.type.name)) return false

  const lastItem = previous.lastChild
  if (!lastItem || !LIST_ITEM_TYPES.has(lastItem.type.name)) return false
  // 上一项的最后一块不一定是文本块：列表项末尾可以挂嵌套子列表（例如
  // “1. 飒飒”下面是二级/三级列表），此时 lastChild 是 list，旧代码会误判为
  // “没有可并入的文本块”并回退到 core joinBackward，空段落又被重新包成列表项，
  // 表现为“删掉序号、再删又冒出来”的横跳。这里改为深度优先找该项最后一个
  // 叶子文本块，真正的插入位置仍由 Selection.near(..., -1) 落到它的末尾。
  if (!lastTextblockInNode(lastItem)) return false

  const blockStart = $from.before(depth)
  // 段落起始位置往前找最近的合法文本位置 = 上一块文本内容的末尾。
  const insertPos = Selection.near(editor.state.doc.resolve(blockStart), -1).from
  const content = paragraph.content
  // 合并后光标应停在两块文字的接缝处（旧文本末尾、被并入文本开头），
  // 不能落到插入内容的末尾。空段落时 insertPos 与 insertPos + 0 相同，
  // 因此这里直接使用 insertPos 不会改变空行删除行为。
  const caretPos = insertPos

  const tr = editor.state.tr
  tr.delete(blockStart, blockStart + paragraph.nodeSize)
  if (content.size) tr.insert(insertPos, content)
  tr.setSelection(TextSelection.create(tr.doc, caretPos))
  editor.view.dispatch(tr.scrollIntoView())

  // 删除这一行后，原本分居两侧的同类型列表可能变成相邻列表块：合并回一个列表，
  // 否则有序列表会重新从 1 开始编号。这一步不进历史，撤销仍一步回到删除前。
  //
  // 结构归一（挂回深缩进子列表、合并根列表）会重建列表节点，ProseMirror
  // 默认映射会把刚才停在拼接处的光标推到下一项开头。先按文本块身份和偏移
  // 记住接缝位置，归一完成后在最终文档中显式恢复。
  const caretTextblock = editor.state.selection.$from.parent
  const caretText = caretTextblock.textContent
  const caretOffset = editor.state.selection.$from.parentOffset
  const caretTextblockPos = editor.state.selection.$from.before(editor.state.selection.$from.depth)
  const caretOccurrence = textblockOccurrenceBefore(editor.state.doc, caretText, caretTextblockPos)
  const joinTr = editor.state.tr
  // 被删掉的普通段落两侧可能夹着“视觉上更深、但结构上仍并列”的子列表。
  // 先把它挂回前一个列表项，两侧同类型根列表才会真正相邻并被合并；
  // 否则后续有序列表仍会从 1 重新编号。
  nestAdjacentVisualLists(joinTr)
  joinAdjacentSameTypeLists(joinTr)
  if (joinTr.steps.length) {
    const caretNodePos = findTextblockPosition(joinTr.doc, caretText, caretOccurrence)
    if (caretNodePos != null) {
      const maxOffset = joinTr.doc.nodeAt(caretNodePos)?.content?.size ?? caretOffset
      const target = caretNodePos + 1 + Math.min(caretOffset, maxOffset)
      joinTr.setSelection(TextSelection.create(joinTr.doc, target))
    }
    editor.view.dispatch(joinTr.setMeta('addToHistory', false).scrollIntoView())
  }
  return true
}

// 光标在“列表块末项文本末尾”按 Delete：
// ProseMirror 默认的 joinForward 会把紧随列表之后的普通段落并进列表，生成一个新的
// 列表项/待办项。若该段落为空，用户会看到“多出一个空项再消失”的闪跳，待办列表还会
// 凭空冒出一条未勾选项。与 Backspace 方向对称，这里直接删掉两块之间的段落边界，把
// 后一段落的文字并入当前列表项（段落为空即单纯删除该空行），光标停在并入位置。
// 只处理“当前项是本列表最后一项、当前文本块是该列表项最后一个子块、后一块是普通段落”。
export function joinParagraphForwardIntoCurrentList(editor) {
  const { selection } = editor.state
  if (!selection.empty) return false

  const $from = selection.$from
  const currentBlock = $from.parent
  if (!currentBlock.isTextblock) return false
  if ($from.parentOffset !== currentBlock.content.size) return false

  // 普通段落末尾紧邻列表时，ProseMirror 的 joinForward 会把下一整个列表
  // 变成范围选区（混合嵌套待办尤其明显）。这里无法在不破坏层级的前提下
  // 无损拼接，保持光标不动，用户仍可从列表首行用 Backspace 调整。
  if (currentBlock.type.name === 'paragraph') {
    const parentDepth = $from.depth - 1
    const parent = $from.node(parentDepth)
    const blockIndex = $from.index(parentDepth)
    const nextSibling = blockIndex + 1 < parent.childCount ? parent.child(blockIndex + 1) : null
    if (nextSibling && LIST_TYPES.has(nextSibling.type.name)) return true
  }

  const itemDepth = $from.depth - 1
  if (itemDepth < 1) return false
  const item = $from.node(itemDepth)
  if (!LIST_ITEM_TYPES.has(item.type.name)) return false

  const blockIndex = $from.index(itemDepth)
  if (blockIndex < item.childCount - 1) {
    // 当前行后面紧跟本项目自己的子列表时，Delete 没有可安全合并的兄弟文本块。
    // 交回 ProseMirror 会把子项拆成当前项的第二段（待办混合嵌套时甚至会选中
    // 整段子树）。这里保持结构不变，用户可先用 Shift+Tab/Backspace 调整层级。
    const nextBlock = item.child(blockIndex + 1)
    if (LIST_TYPES.has(nextBlock.type.name)) return true
    return false
  }

  const listDepth = itemDepth - 1
  const list = $from.node(listDepth)
  if (!LIST_TYPES.has(list.type.name)) return false

  const paragraphStart = $from.after(listDepth)
  const paragraph = editor.state.doc.resolve(paragraphStart).nodeAfter
  const itemIndex = $from.index(listDepth)

  // 列表尚有后续项，且列表后面还有普通段落时，ProseMirror 的 joinForward
  // 会越过后续列表项，直接把列表后的段落并进当前项，造成文字顺序跳变。
  // 明确把紧邻的下一列表项并进当前项，保持列表项之间的顺序，尾部段落不动。
  if (itemIndex < list.childCount - 1) {
    const nextItem = list.child(itemIndex + 1)
    const nextTextblock = nextItem.firstChild
    if (!nextTextblock?.isTextblock) return false

    const itemPos = $from.before(itemDepth)
    const nextItemPos = itemPos + item.nodeSize
    const blockIndex = $from.index(itemDepth)

    // Delete 是“合并下一行”，不是只删掉下一项的列表标记。把两项的首段
    // 内联合并成一个段落；下一项剩余的段落/子列表整体挂到当前项后面，
    // 从而保留原文字内联标记和整棵子树。
    const mergedTextblock = currentBlock.type.create(
      currentBlock.attrs,
      currentBlock.content.append(nextTextblock.content),
      currentBlock.marks,
    )
    const mergedChildren = []
    item.forEach((child, _offset, index) => {
      mergedChildren.push(index === blockIndex ? mergedTextblock : child)
    })
    nextItem.forEach((child, _offset, index) => {
      if (index > 0) mergedChildren.push(child)
    })

    const mergedItem = item.type.create(item.attrs, mergedChildren, item.marks)
    const tr = editor.state.tr.replaceWith(
      itemPos,
      nextItemPos + nextItem.nodeSize,
      mergedItem,
    )
    tr.setSelection(TextSelection.create(tr.doc, selection.from))
    editor.view.dispatch(tr.scrollIntoView())
    return true
  }

  // 列表块后面的紧邻兄弟块不是普通段落时，默认 joinForward 会越过
  // 列表边界选中下一棵列表项（多层待办时会连子孙一起选中）。这里没有
  // 不改变层级的安全拼接语义，明确保持光标不动。
  //
  // 深层列表最后一项的 nodeAfter 常常直接是 null（后面是祖先列表的兄弟项，
  // 并没有同级的普通段落）。这条路径也必须接管：回退 false 会让 ProseMirror
  // 的默认 joinForward 把后续整棵列表子树选成范围。
  if (!paragraph) return true
  if (paragraph.type.name !== 'paragraph') return true

  const paragraphEnd = paragraphStart + paragraph.nodeSize
  const insertPos = selection.from
  const content = paragraph.content
  const tr = editor.state.tr
  tr.delete(paragraphStart, paragraphEnd)
  if (content.size) tr.insert(insertPos, content)
  // 与 Backspace 方向对称：Delete 删除段落边界后，光标应停在两段文字的
  // 拼接处，而不是插入内容末尾。否则用户继续输入会跑到下一段文字之后。
  tr.setSelection(TextSelection.create(tr.doc, insertPos))
  editor.view.dispatch(tr.scrollIntoView())
  return true
}

// Ctrl+A 产生的是 AllSelection：它没有"光标所在的列表项"（$from 落在文档起点，
// depth 为 0），所有以 $from 为入口的缩进逻辑都会直接失效——表现为"拖选全文按
// Shift+Tab 有反应，Ctrl+A 全选后按同一按键毫无反应"。
// 这里把它等价改写成"覆盖整篇的普通文本选区"，让 Ctrl+A 与拖选全文走同一条路径。
export function normalizeWholeDocumentSelection(editor) {
  const { selection, doc } = editor.state
  if (!(selection instanceof AllSelection)) return false

  const from = Selection.near(doc.resolve(1), 1).from
  const to = Selection.near(doc.resolve(Math.max(1, doc.content.size - 1)), -1).from
  return editor.commands.setTextSelection({
    from: Math.min(from, to),
    to: Math.max(from, to),
  })
}

export function liftActiveListItem(editor) {
  // 范围选区：先走批量路径，理由同 sinkActiveListItem。
  const pendingAtomicSelection = captureAtomicNodeSelection(editor)
  if (!pendingAtomicSelection && !editor.state.selection.empty && liftSelectedListItems(editor)) return true
  return liftSingleActiveListItem(editor, { pendingAtomicSelection })
}

// 单个（光标所在）列表项的反缩进。
function liftSingleActiveListItem(editor, options = {}) {
  const itemType = activeListItemType(editor)
  const info = activeListItemInfo(editor)
  if (!itemType || !info) return false

  const beforeDoc = editor.state.doc
  const targetItemPos = info.pos
  const selectionOffsets = editor.state.selection.empty
    ? selectionOffsetRangeForInfo(editor, info)
    : null
  const originalListType = info.parentList.type.name
  const originalItemTypeName = info.node.type.name
  const rememberRoundTrip = () => rememberListLiftRoundTrip(
    editor,
    beforeDoc,
    targetItemPos,
    selectionOffsets,
    originalListType,
    originalItemTypeName,
  )

  if (info.indentLevel > 0) {
    const outdented = setActiveListExtraIndent(editor, info.indentLevel - 1)
    if (outdented) {
      rememberRoundTrip()
      restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    }
    return outdented
  }

  // 反缩进统一走 ProseMirror 的 liftListItem：
  // - 二级/三级项：提升到上一层列表；
  // - 一级项：退出列表、该项自身变成普通文本块（2026-10-15 产品确认，取消旧的
  //   “一级项 Shift+Tab 什么都不做”规则），与工具栏“再次点击当前列表按钮取消
  //   列表”一致。
  // 两种情况都必须按原有视觉层级补偿它下面的子列表，否则一级项退出列表时，
  // 它下面的子列表会整体上浮一级（结构降了 2 级、视觉只该降 1 级）。
  // 一级项仍带子列表时，ProseMirror 的 liftListItem 会先把子列表拆成独立
  // 列表块，再把当前项标成视觉缩进；连续 Backspace 的下一步便落到默认
  // joinBackward，把当前项和上一级拼成一行。这里复用工具栏“取消当前列表”
  // 的结构重建逻辑：只把光标项变成普通文本块，父项、前后兄弟和子列表都保留。
  if (!options.selectedTexts?.length && info.structuralDepth === 1 && hasDirectChildList(info.node)) {
    const exited = toggleActiveListFormatting(editor, originalListType)
    if (exited) {
      rememberRoundTrip()
      restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    }
    return exited
  }

  if (liftMixedListItem(editor, info, options)) {
    rememberRoundTrip()
    restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    return true
  }

  // 选区只落在光标项内部的某个子区间时（例如选中一项里的一段字），liftListItem 会把
  // 选区塌成光标。这里先记下相对偏移，变换后按 (文本, 出现序号) 把同一段选回来。
  const pendingSelection = captureBlockTextSelection(editor, {
    kind: 'item',
    pos: info.pos,
    text: ownTextblockText(info.node),
  })

  const siblingSignatures = trailingSiblingSignatures(editor, info)
  const descendantVisualDepths = collectDescendantListVisualDepths(editor, info, options.selectedTexts)
  const lifted = editor.chain().focus().liftListItem(itemType).run()
  if (!lifted) return false
  const preserved = preserveDescendantListVisualDepths(
    editor,
    descendantVisualDepths,
    liftFallbackAnchor(editor),
  )
  separateTrailingSiblingsAfterLift(editor, siblingSignatures)
  restoreBlockTextSelection(editor, pendingSelection)
  restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
  rememberRoundTrip()
  return preserved
}
