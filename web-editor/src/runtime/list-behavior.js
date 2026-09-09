// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { TextSelection } from '@tiptap/pm/state'

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
  return activeListItemInfo(editor)?.parentList?.type?.name || null
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
      hasPreviousTopLevelBlock: depth - 1 === 1 && $from.index(0) > 0,
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

export function getActiveListNestingDepth(editor) {
  const info = activeListItemInfo(editor)
  if (!info) return 0
  return Math.min(MAX_LIST_NESTING_DEPTH, info.structuralDepth + info.indentLevel)
}

export function isInListItem(editor) {
  return getActiveListNestingDepth(editor) > 0
}

function isDetachedTopLevelListItem(info) {
  // ProseMirror 在顶层列表项继续 Shift+Tab 时会把当前项变成普通段落，
  // 同时把后续子列表拆成新的顶层列表块。这个新块没有同级前置项，
  // 但它不是用户新建的普通首项，而是从原三级结构中“游离”出来的项。
  // 这种项即使已降回一级，也必须允许 Tab 继续恢复到二/三级。
  return info.structuralDepth === 1
    && info.childIndex === 0
    && info.hasPreviousTopLevelBlock
}

function canUseExtraIndent(info) {
  // 普通文档开头的一级首项仍遵守列表语义，不能凭空缩进；进入子级、
  // 已带 dvnIndentLevel、或从旧子树游离出来的顶层项，则用额外缩进
  // 补齐/恢复最多三级的视觉层级。
  return (info.structuralDepth > 1 || info.indentLevel > 0 || isDetachedTopLevelListItem(info))
    && info.indentLevel < maxExtraIndentForStructuralDepth(info.structuralDepth)
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

function liftMixedListItem(editor, info) {
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

  const beforeInnerItems = []
  const afterInnerItems = []
  for (let index = 0; index < info.parentList.childCount; index += 1) {
    if (index < info.childIndex) beforeInnerItems.push(info.parentList.child(index))
    if (index > info.childIndex) afterInnerItems.push(info.parentList.child(index))
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
  const activeItemChildren = childNodesOf(info.node)
  if (afterInnerList) activeItemChildren.push(afterInnerList)
  const activeItem = info.node.type.create(info.node.attrs, activeItemChildren, info.node.marks)

  const leftOuterItems = []
  for (let index = 0; index < parentItemIndex; index += 1) leftOuterItems.push(outerList.child(index))
  leftOuterItems.push(modifiedParentItem)

  const rightOuterItems = []
  for (let index = parentItemIndex + 1; index < outerList.childCount; index += 1) {
    rightOuterItems.push(outerList.child(index))
  }

  const leftOuterList = createListOfType(editor, outerList.type.name, leftOuterItems, outerList.attrs, outerList.marks)
  const activeList = createListOfType(editor, info.parentList.type.name, [activeItem], info.parentList.attrs, info.parentList.marks)
  const rightOuterList = createListOfType(editor, outerList.type.name, rightOuterItems, outerList.attrs, outerList.marks)
  if (!leftOuterList || !activeList) return false

  const descendantVisualDepths = collectDescendantListVisualDepths(info)
  const replacement = [leftOuterList, activeList, rightOuterList].filter(Boolean)
  const offsets = selectionOffsetRangeForInfo(editor, info)
  const outerListPos = $from.before(outerListDepth)
  const activeItemPos = outerListPos + leftOuterList.nodeSize + 1
  const tr = editor.state.tr.replaceWith(outerListPos, outerListPos + outerList.nodeSize, replacement)

  dispatchWithRestoredListItemSelection(editor, tr, activeItemPos, activeItem, offsets)
  return preserveDescendantListVisualDepths(editor, descendantVisualDepths, activeItemPos)
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
      return setActiveListExtraIndent(editor, previousIndent)
    }
    return editor.chain().focus().sinkListItem(itemType).run()
  }

  if (canUseExtraIndent(info)) {
    return setActiveListExtraIndent(editor, info.indentLevel + 1)
  }

  return true
}


function collectDescendantListVisualDepths(info) {
  const visualDepths = []

  function walk(node, structuralDepth) {
    if (!node || !node.childCount) return
    for (let index = 0; index < node.childCount; index += 1) {
      const child = node.child(index)
      const isListItem = LIST_ITEM_TYPES.has(child.type.name)
      const childStructuralDepth = isListItem ? structuralDepth + 1 : structuralDepth
      if (isListItem) {
        visualDepths.push(Math.min(
          MAX_LIST_NESTING_DEPTH,
          childStructuralDepth + normalizedIndentLevel(
            child.attrs?.dvnIndentLevel,
            maxExtraIndentForStructuralDepth(childStructuralDepth),
          ),
        ))
      }
      walk(child, childStructuralDepth)
    }
  }

  walk(info.node, info.structuralDepth)
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

function preserveListVisualDepthsAfterPosition(editor, visualDepths, fromPos) {
  let index = 0
  let tr = null

  editor.state.doc.descendants((node, pos) => {
    if (index >= visualDepths.length) return false
    if (!LIST_ITEM_TYPES.has(node.type.name)) return true
    if (pos < fromPos) return true

    tr ||= editor.state.tr
    const structuralDepth = listItemDepthAtPosition(editor.state.doc, pos)
    setListItemVisualDepthInTransaction(tr, node, pos, structuralDepth, visualDepths[index])
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
        tr ||= editor.state.tr
        setListItemVisualDepthInTransaction(
          tr,
          child,
          childPos,
          childStructuralDepth,
          visualDepths[index],
        )
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

export function liftActiveListItem(editor) {
  const itemType = activeListItemType(editor)
  const info = activeListItemInfo(editor)
  if (!itemType || !info) return false
  if (info.indentLevel > 0) return setActiveListExtraIndent(editor, info.indentLevel - 1)

  // 产品规则：Tab / Shift+Tab 只负责在 1~3 级列表间调整层级；
  // 一级列表项不再通过 Shift+Tab 退出列表，避免把后续子项拆成
  // 没有结构父节点的“游离三级项”。取消列表交给工具栏列表按钮。
  if (getActiveListNestingDepth(editor) <= 1) return true

  if (liftMixedListItem(editor, info)) return true

  const descendantVisualDepths = collectDescendantListVisualDepths(info)
  const lifted = editor.chain().focus().liftListItem(itemType).run()
  if (!lifted) return false
  return preserveDescendantListVisualDepths(
    editor,
    descendantVisualDepths,
    Math.max(editor.state.selection.from, editor.state.selection.to),
  )
}
