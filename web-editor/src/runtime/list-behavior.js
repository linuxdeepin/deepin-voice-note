// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

export const MAX_LIST_NESTING_DEPTH = 3

const LIST_ITEM_TYPES = new Set(['listItem', 'taskItem'])

export function activeListItemType(editor) {
  if (editor.isActive('taskItem')) return 'taskItem'
  if (editor.isActive('listItem')) return 'listItem'
  return null
}

export function getActiveListNestingDepth(editor) {
  const { $from } = editor.state.selection
  let depth = 0
  for (let index = 1; index <= $from.depth; index += 1) {
    if (LIST_ITEM_TYPES.has($from.node(index).type.name)) depth += 1
  }
  return depth
}

export function isInListItem(editor) {
  return getActiveListNestingDepth(editor) > 0
}

export function canIndentActiveListItem(editor) {
  return !!activeListItemType(editor) && getActiveListNestingDepth(editor) < MAX_LIST_NESTING_DEPTH
}

export function canOutdentActiveListItem(editor) {
  return !!activeListItemType(editor)
}

export function sinkActiveListItem(editor) {
  const itemType = activeListItemType(editor)
  if (!itemType) return false
  if (getActiveListNestingDepth(editor) >= MAX_LIST_NESTING_DEPTH) return true
  return editor.chain().focus().sinkListItem(itemType).run()
}

export function liftActiveListItem(editor) {
  const itemType = activeListItemType(editor)
  if (!itemType) return false
  return editor.chain().focus().liftListItem(itemType).run()
}
