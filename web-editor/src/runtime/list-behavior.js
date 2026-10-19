// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { AllSelection, NodeSelection, Selection, TextSelection } from '@tiptap/pm/state'
import { canJoin } from '@tiptap/pm/transform'

export const MAX_LIST_NESTING_DEPTH = 3
export const MAX_LIST_EXTRA_INDENT = MAX_LIST_NESTING_DEPTH - 1

const LIST_ITEM_TYPES = new Set(['listItem', 'taskItem'])
const LIST_TYPES = new Set(['bulletList', 'orderedList', 'taskList'])

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
  const start = pos + 2
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
  if (selection.empty) return null
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

  // 1) 视觉额外缩进项：只改属性，节点尺寸不变，位置与选区都保持有效。
  if (visual.length) {
    const tr = editor.state.tr
    for (const { pos, node, indentLevel } of visual) {
      tr.setNodeMarkup(pos, undefined, { ...node.attrs, dvnIndentLevel: indentLevel - 1 })
    }
    tr.setSelection(editor.state.selection.map(tr.doc, tr.mapping))
    editor.view.dispatch(tr.scrollIntoView())
  }

  // 2) 结构嵌套项：从后往前逐个提升，前面项的位置不会被后面的操作打乱。
  for (let index = structural.length - 1; index >= 0; index -= 1) {
    const entry = structural[index]
    const size = editor.state.doc.content.size
    if (entry.pos > size) continue
    const inside = Selection.near(editor.state.doc.resolve(entry.pos + 1), 1).from
    editor.commands.setTextSelection(inside)
    // 该目标子树里同样被选中的项（文档顺序）：它们跟着祖先一起上移一级即已达成
    // "各降一级"，不能再按原视觉层级写回。
    const selectedTexts = items
      .filter((item) => (
        item !== entry
        && item.pos > entry.pos
        && item.pos < entry.pos + entry.node.nodeSize
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
      editor.chain().focus().sinkListItem(itemType).run()
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

  const tr = editor.state.tr.setNodeMarkup(info.pos, undefined, {
    ...info.node.attrs,
    dvnIndentLevel: nextLevel,
  })
  editor.view.dispatch(tr.scrollIntoView())
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

function convertListNodeToType(editor, node, targetListType) {
  const targetItemTypeName = targetListType === 'taskList' ? 'taskItem' : 'listItem'

  if (LIST_TYPES.has(node.type.name)) {
    const children = []
    for (let index = 0; index < node.childCount; index += 1) {
      children.push(convertListNodeToType(editor, node.child(index), targetListType))
    }
    return editor.schema.nodes[targetListType].create(
      listAttrsForType(targetListType, node.attrs),
      children,
      node.marks,
    )
  }

  if (LIST_ITEM_TYPES.has(node.type.name)) {
    const children = []
    for (let index = 0; index < node.childCount; index += 1) {
      const child = node.child(index)
      children.push(LIST_TYPES.has(child.type.name)
        ? convertListNodeToType(editor, child, targetListType)
        : child)
    }
    return editor.schema.nodes[targetItemTypeName].create(
      itemAttrsForType(targetItemTypeName, node.attrs),
      children,
      node.marks,
    )
  }

  return node
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

function selectionFullyCoversNode(selection, pos, node) {
  const textRange = textContentRangeOfNode(pos, node)
  if (!textRange) return selection.from <= pos + 1 && selection.to >= pos + node.nodeSize - 1
  return selection.from <= textRange.from && selection.to >= textRange.to
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

function listIntersectsSelection(selection, pos, node) {
  return selection.to > pos && selection.from < pos + node.nodeSize
}

function listNestingDepth(doc, pos) {
  let depth = 0
  const $pos = doc.resolve(Math.min(pos + 1, doc.content.size))
  for (let level = 1; level <= $pos.depth; level += 1) {
    if (LIST_TYPES.has($pos.node(level).type.name)) depth += 1
  }
  return depth
}

function collectFullyCoveredLists(selection, doc) {
  const lists = []
  doc.descendants((node, pos) => {
    if (!LIST_TYPES.has(node.type.name)) return true
    if (!selectionFullyCoversNode(selection, pos, node)) return true

    lists.push({ node, pos })
    return false
  })
  return lists
}

// 转换后相邻的同类型列表必须合并回一个列表：ProseMirror/Tiptap 自己的 toggleList
// 也是靠 joinListBackwards / joinListForwards 合并的。否则每次按块转换都会留下
// 多个独立的有序列表块，每块各自从 1 开始编号，表现为序号不连续。
function normalizedNumberingType(type) {
  return !type || type === '1' ? null : type
}

function listsCanMerge(left, right) {
  return LIST_TYPES.has(left.type.name)
    && left.type.name === right.type.name
    && normalizedNumberingType(left.attrs?.type) === normalizedNumberingType(right.attrs?.type)
}

function joinAdjacentSameTypeLists(tr) {
  const boundaries = []
  tr.doc.descendants((node, pos) => {
    if (!LIST_TYPES.has(node.type.name)) return true

    // 列表节点自身的位置就是它和前一个兄弟节点之间的分界位置，
    // 用 doc.resolve 取左侧兄弟即可，无需自己推导嵌套层级下的偏移。
    const previous = tr.doc.resolve(pos).nodeBefore
    if (previous && listsCanMerge(previous, node)) boundaries.push(pos)
    return true
  })

  // 从后往前走：合并靠后的相邻列表不会让前面已算好的位置失效。
  for (const boundary of boundaries.sort((left, right) => right - left)) {
    if (canJoin(tr.doc, boundary)) tr.join(boundary)
  }
  return tr
}

// 选区只覆盖列表的一部分时，取选区所在的最内层列表：外层列表项保持原类型，
// 只把命中的那几项拿出来转换，避免整张列表被换掉。
function collectPartialSelectionLists(selection, doc, coveredRanges) {
  const intersecting = []
  doc.descendants((node, pos) => {
    if (!LIST_TYPES.has(node.type.name)) return true
    if (!listIntersectsSelection(selection, pos, node)) return true
    if (selectionFullyCoversNode(selection, pos, node)) return true
    if (coveredRanges.some(([from, to]) => pos > from && pos < to)) return true

    // 外层列表与选区的交集完全落在某个“已被完整选中”的子列表内部时，外层列表
    // 自身没有需要单独转换的内容：真正要转的是那个子列表（已收进 coveredLists）。
    // 若仍把外层列表收进来，它会生成一个只包含该子列表的 partial split，和
    // coveredLists 的处理范围重叠，后续“目标类型是否与当前一致”的判断也会失真。
    const overlapFrom = Math.max(selection.from, pos)
    const overlapTo = Math.min(selection.to, pos + node.nodeSize)
    if (coveredRanges.some(([from, to]) => overlapFrom >= from && overlapTo <= to)) return true

    intersecting.push({ node, pos, depth: listNestingDepth(doc, pos) })
    return true
  })

  if (!intersecting.length) return []

  // 选区部分覆盖时，真正要转换的是“包含选区边界”的最内层列表。
  // 跨层级选中子列表内容时（例如选中的待办项内部还挂着子待办列表），外层列表
  // 虽然与选区相交，但它自身和选区外的兄弟项都不该被转换；只按 depth 取最深会
  // 把外层列表当成转换目标，使“目标类型是否与当前一致”的判断失真，最终落到
  // Tiptap 默认 toggle 命令，把整个列表结构拆散、选区扩到外层全部内容。
  const insideSelectionEdge = (entry) => {
    const end = entry.pos + entry.node.nodeSize
    return (selection.from > entry.pos && selection.from < end)
      || (selection.to > entry.pos && selection.to < end)
  }
  const edgeLists = intersecting.filter(insideSelectionEdge)
  const pool = edgeLists.length ? edgeLists : intersecting
  const deepest = Math.max(...pool.map((entry) => entry.depth))
  return pool.filter((entry) => entry.depth === deepest)
}

// 把列表按选区切成 before | selected | after，只有 selected 换成目标类型，
// before/after 保持原类型（选区落在列表中间时就地拆成三段）。
function splitListAroundSelection(editor, entry, selection, targetListType) {
  const items = childNodesOf(entry.node)
  const selectedIndexes = []
  let itemPos = entry.pos + 1

  for (let index = 0; index < items.length; index += 1) {
    const item = items[index]
    if (selection.to > itemPos && selection.from < itemPos + item.nodeSize) {
      selectedIndexes.push(index)
    }
    itemPos += item.nodeSize
  }

  if (!selectedIndexes.length) return null

  const firstIndex = selectedIndexes[0]
  const lastIndex = selectedIndexes[selectedIndexes.length - 1]
  const convertedItems = items
    .slice(firstIndex, lastIndex + 1)
    .map((item) => convertListNodeToType(editor, item, targetListType))
  const selectedList = createListOfType(editor, targetListType, convertedItems, entry.node.attrs)
  if (!selectedList) return null

  const beforeList = createListOfType(
    editor,
    entry.node.type.name,
    items.slice(0, firstIndex),
    entry.node.attrs,
    entry.node.marks,
  )
  const afterList = createListOfType(
    editor,
    entry.node.type.name,
    items.slice(lastIndex + 1),
    entry.node.attrs,
    entry.node.marks,
  )

  return {
    content: [beforeList, selectedList, afterList].filter(Boolean),
    // selected 段在替换内容里的起始偏移 = before 段长度
    anchorOffset: beforeList ? beforeList.nodeSize : 0,
  }
}

export function switchSelectedListType(editor, targetListType) {
  const { selection, doc } = editor.state
  if (selection.empty || !LIST_TYPES.has(targetListType)) return false

  const allCoveredLists = collectFullyCoveredLists(selection, doc)
  const coveredRanges = allCoveredLists.map(({ node, pos }) => [pos, pos + node.nodeSize])
  const partialLists = collectPartialSelectionLists(selection, doc, coveredRanges)

  const splits = []
  for (const entry of partialLists) {
    const split = splitListAroundSelection(editor, entry, selection, targetListType)
    if (split) {
      splits.push({
        ...split,
        pos: entry.pos,
        size: entry.node.nodeSize,
        sourceTypeName: entry.node.type.name,
      })
    }
  }

  // 部分选中的父列表（如二级列表的选中项）在 replaceWith 时会连同它内部的子列表
  // 一起走 convertListNodeToType 的递归转换；而子列表自己如果也被 collectFullyCoveredLists
  // 收进来，就会和父列表产生“位置重叠的两次 replaceWith”：后执行的父级替换用替换前
  // 构造好的节点内容覆盖掉子列表刚做完的转换。表现为选区起点/终点向外漂移，甚至下一次
  // 切换时整表被选中。这里把已被某个 split 范围完全包住的 coveredList 剔除，交给父级
  // 的递归转换处理。
  const splitRanges = splits.map((split) => [split.pos, split.pos + split.size])
  const coveredLists = allCoveredLists.filter(({ node, pos }, index) => {
    const from = pos
    const to = pos + node.nodeSize
    // 父级 partial split 会递归转换它内部被完整选中的子列表。
    if (splitRanges.some(([start, end]) => from >= start && to <= end)) return false
    // 父子列表同时被完整选中（例如选中的待办项内部还挂着子待办列表）时，只留最外层；
    // 否则两次 replaceWith 范围重叠，后执行的父级替换会用旧内容覆盖子级刚做的转换。
    return !allCoveredLists.some((other, otherIndex) => otherIndex !== index
      && other.pos <= from
      && to <= other.pos + other.node.nodeSize
      && (other.pos < from || to < other.pos + other.node.nodeSize))
  })

  const involvedTypeNames = [
    ...coveredLists.map(({ node }) => node.type.name),
    ...splits.map((split) => split.sourceTypeName),
  ]
  if (!involvedTypeNames.length) return false
  // 目标类型与当前一致时交回默认命令（同类 = 取消这些项的列表），不改文档。
  if (involvedTypeNames.every((name) => name === targetListType)) return false

  const operations = [
    ...coveredLists.map(({ node, pos }) => ({
      pos,
      size: node.nodeSize,
      content: convertListNodeToType(editor, node, targetListType),
      anchorOffset: 0,
    })),
    ...splits,
  ].sort((left, right) => right.pos - left.pos)

  let tr = editor.state.tr
  for (const operation of operations) {
    tr = tr.replaceWith(operation.pos, operation.pos + operation.size, operation.content)
  }

  // replaceWith 会把选区映射到替换后的边界（选区塌缩，表现为紫色高亮消失），
  // 所以先按“转换后的列表节点仍然被完整选中”算出文本范围；
  // 必须在合并相邻同类型列表之前取，否则合并后范围会缩水。
  const convertedRanges = operations
    .map((operation) => {
      const pos = tr.mapping.map(operation.pos, -1) + operation.anchorOffset
      return textContentRangeOfNode(pos, tr.doc.nodeAt(pos))
    })
    .filter(Boolean)

  const mergeStartStep = tr.steps.length
  joinAdjacentSameTypeLists(tr)

  const nextSelection = selectionAfterListConversion(
    tr.doc,
    selection,
    convertedRanges,
    tr.mapping.slice(mergeStartStep),
  )
  if (nextSelection) tr = tr.setSelection(nextSelection)

  editor.view.dispatch(tr.scrollIntoView())
  return true
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

function liftMixedListItem(editor, info, options = {}) {
  const { $from } = editor.state.selection
  const parentItemDepth = info.parentListDepth - 1
  const outerListDepth = info.parentListDepth - 2
  if (parentItemDepth <= 0 || outerListDepth <= 0) return false

  const parentItem = $from.node(parentItemDepth)
  const outerList = $from.node(outerListDepth)
  if (!LIST_ITEM_TYPES.has(parentItem.type.name) || !LIST_TYPES.has(outerList.type.name)) return false
  if (outerList.type.name === info.parentList.type.name) return false

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
  for (let index = 0; index < parentItem.childCount; index += 1) {
    if (index === nestedListIndex) {
      if (beforeInnerList) modifiedParentChildren.push(beforeInnerList)
    } else {
      modifiedParentChildren.push(parentItem.child(index))
    }
  }
  const modifiedParentItem = parentItem.type.create(parentItem.attrs, modifiedParentChildren, parentItem.marks)

  // 与 ProseMirror 同类型列表的 liftListItem 行为保持一致：
  // - 当前项之前的兄弟项仍留在原父项下面；
  // - 当前项之后的兄弟项继续排在当前项之后，并作为当前项的子列表保留原视觉层级；
  // 否则混合列表（如待办 > 无序）反缩进中间项时会把后续项留在父项下面，
  // 导致屏幕顺序变成 parent / before / after / active。
  // 未提升的后续兄弟项挂到最后一个被提升项下面，保持原视觉层级。
  const lastLiftedItem = liftedItems[liftedItems.length - 1]
  const activeItemChildren = childNodesOf(lastLiftedItem)
  if (afterInnerList) activeItemChildren.push(afterInnerList)
  const activeItem = lastLiftedItem.type.create(lastLiftedItem.attrs, activeItemChildren, lastLiftedItem.marks)

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
    [...liftedItems.slice(0, -1), activeItem],
    info.parentList.attrs,
    info.parentList.marks,
  )
  const rightOuterList = createListOfType(editor, outerList.type.name, rightOuterItems, outerList.attrs, outerList.marks)
  if (!leftOuterList || !activeList) return false

  const descendantVisualDepths = collectDescendantListVisualDepths(editor, info, options.selectedTexts)
  const replacement = [leftOuterList, activeList, rightOuterList].filter(Boolean)
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
  return preserveDescendantListVisualDepths(editor, descendantVisualDepths, activeListPos)
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

function sinkSingleActiveListItem(editor, options = {}) {
  const itemType = activeListItemType(editor)
  const info = activeListItemInfo(editor)
  if (!itemType || !info) return false

  if (getActiveListNestingDepth(editor) >= MAX_LIST_NESTING_DEPTH) return true

  if (info.indentLevel === 0 && editor.can().sinkListItem(itemType)) {
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
  const joinTr = editor.state.tr
  joinAdjacentSameTypeLists(joinTr)
  if (joinTr.steps.length) {
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

  const itemDepth = $from.depth - 1
  if (itemDepth < 1) return false
  const item = $from.node(itemDepth)
  if (!LIST_ITEM_TYPES.has(item.type.name)) return false
  if ($from.index(itemDepth) !== item.childCount - 1) return false

  const listDepth = itemDepth - 1
  const list = $from.node(listDepth)
  if (!LIST_TYPES.has(list.type.name)) return false

  // 列表块后面的紧邻兄弟块必须就是普通段落。
  const paragraphStart = $from.after(listDepth)
  const paragraph = editor.state.doc.resolve(paragraphStart).nodeAfter
  if (!paragraph || paragraph.type.name !== 'paragraph') return false

  const paragraphEnd = paragraphStart + paragraph.nodeSize
  const insertPos = selection.from
  const content = paragraph.content
  const tr = editor.state.tr
  tr.delete(paragraphStart, paragraphEnd)
  if (content.size) tr.insert(insertPos, content)
  tr.setSelection(TextSelection.create(tr.doc, insertPos + content.size))
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

  if (info.indentLevel > 0) {
    const outdented = setActiveListExtraIndent(editor, info.indentLevel - 1)
    if (outdented) restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
    return outdented
  }

  // 反缩进统一走 ProseMirror 的 liftListItem：
  // - 二级/三级项：提升到上一层列表；
  // - 一级项：退出列表、该项自身变成普通文本块（2026-10-15 产品确认，取消旧的
  //   “一级项 Shift+Tab 什么都不做”规则），与工具栏“再次点击当前列表按钮取消
  //   列表”一致。
  // 两种情况都必须按原有视觉层级补偿它下面的子列表，否则一级项退出列表时，
  // 它下面的子列表会整体上浮一级（结构降了 2 级、视觉只该降 1 级）。
  if (liftMixedListItem(editor, info, options)) {
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

  const descendantVisualDepths = collectDescendantListVisualDepths(editor, info, options.selectedTexts)
  const lifted = editor.chain().focus().liftListItem(itemType).run()
  if (!lifted) return false
  const preserved = preserveDescendantListVisualDepths(
    editor,
    descendantVisualDepths,
    liftFallbackAnchor(editor),
  )
  restoreBlockTextSelection(editor, pendingSelection)
  restoreAtomicNodeSelection(editor, options.pendingAtomicSelection)
  return preserved
}
