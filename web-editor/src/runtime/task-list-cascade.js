// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { Plugin, PluginKey } from '@tiptap/pm/state'

const pluginKey = new PluginKey('taskListCascade')

function isTaskItem(node) {
  return node?.type?.name === 'taskItem'
}

function checkedOf(node) {
  return Boolean(node?.attrs?.checked)
}

function normalizedIndentLevel(node, structuralDepth) {
  const level = Number(node?.attrs?.dvnIndentLevel)
  const maxExtraIndent = Math.max(0, 3 - structuralDepth)
  if (!Number.isFinite(level) || level <= 0 || maxExtraIndent <= 0) return 0
  return Math.min(maxExtraIndent, Math.floor(level))
}

function taskItemStructuralDepthAt(doc, pos) {
  const $pos = doc.resolve(Math.min(pos + 1, doc.content.size))
  let depth = 0
  for (let index = 1; index <= $pos.depth; index += 1) {
    if (isTaskItem($pos.node(index))) depth += 1
  }
  return depth
}

function taskItemVisualDepthAt(doc, pos, node = safeNodeAt(doc, pos)) {
  if (!isTaskItem(node)) return 0
  const structuralDepth = taskItemStructuralDepthAt(doc, pos)
  return Math.min(3, structuralDepth + normalizedIndentLevel(node, structuralDepth))
}

function parentListInfoAt(doc, itemPos) {
  const $pos = doc.resolve(Math.min(itemPos + 1, doc.content.size))
  for (let depth = $pos.depth; depth > 0; depth -= 1) {
    if (!isTaskItem($pos.node(depth))) continue

    const listDepth = depth - 1
    const listNode = $pos.node(listDepth)
    if (listNode?.type?.name !== 'taskList') return null

    return {
      listNode,
      listPos: $pos.before(listDepth),
      itemIndex: $pos.index(listDepth),
    }
  }
  return null
}

function collectTaskItemsFromNestedTaskLists(doc, node, basePos, parentVisualDepth, mode) {
  const children = []
  if (!isTaskItem(node)) return children

  node.forEach((child, childOffset) => {
    if (child?.type?.name !== 'taskList') return

    const listPos = basePos + 1 + childOffset
    child.forEach((listChild, listChildOffset) => {
      if (!isTaskItem(listChild)) return

      const childPos = listPos + 1 + listChildOffset
      const childVisualDepth = taskItemVisualDepthAt(doc, childPos, listChild)
      if (childVisualDepth <= parentVisualDepth) return
      if (mode === 'descendants' || childVisualDepth === parentVisualDepth + 1) {
        children.push({ node: listChild, pos: childPos })
      }
    })
  })

  return children
}

function collectTaskItemsFromFollowingVisualSiblings(doc, basePos, parentVisualDepth, mode) {
  const children = []
  const parentList = parentListInfoAt(doc, basePos)
  if (!parentList) return children

  let offset = 1
  for (let index = 0; index < parentList.listNode.childCount; index += 1) {
    const sibling = parentList.listNode.child(index)
    const siblingPos = parentList.listPos + offset
    offset += sibling.nodeSize

    if (index <= parentList.itemIndex || !isTaskItem(sibling)) continue

    const siblingVisualDepth = taskItemVisualDepthAt(doc, siblingPos, sibling)
    if (siblingVisualDepth <= parentVisualDepth) break
    if (mode === 'descendants' || siblingVisualDepth === parentVisualDepth + 1) {
      children.push({ node: sibling, pos: siblingPos })
    }
  }

  return children
}

function uniqueTaskItems(items) {
  const seen = new Set()
  return items.filter((item) => {
    if (seen.has(item.pos)) return false
    seen.add(item.pos)
    return true
  })
}

function visualTaskItemsUnder(doc, node, basePos, mode) {
  if (!isTaskItem(node)) return []

  const parentVisualDepth = taskItemVisualDepthAt(doc, basePos, node)
  return uniqueTaskItems([
    ...collectTaskItemsFromNestedTaskLists(doc, node, basePos, parentVisualDepth, mode),
    ...collectTaskItemsFromFollowingVisualSiblings(doc, basePos, parentVisualDepth, mode),
  ])
}

function taskItemDescendants(doc, node, basePos) {
  const descendants = []
  if (!isTaskItem(node)) return descendants

  node.descendants((child, relativePos) => {
    if (!isTaskItem(child)) return true
    descendants.push({ node: child, pos: basePos + 1 + relativePos })
    return true
  })

  // dvnIndentLevel 会把同一层 taskList 中的后续兄弟项显示成当前项的
  // 子项；这些“视觉子项”不是 ProseMirror 结构 descendants，手动勾选
  // 父项时也必须一起级联。
  const parentVisualDepth = taskItemVisualDepthAt(doc, basePos, node)
  for (const sibling of collectTaskItemsFromFollowingVisualSiblings(doc, basePos, parentVisualDepth, 'descendants')) {
    descendants.push(sibling)
    sibling.node.descendants((child, relativePos) => {
      if (!isTaskItem(child)) return true
      descendants.push({ node: child, pos: sibling.pos + 1 + relativePos })
      return true
    })
  }

  return uniqueTaskItems(descendants)
}

function directChildTaskItems(doc, node, basePos) {
  return visualTaskItemsUnder(doc, node, basePos, 'children')
}

function collectTaskItems(doc) {
  const items = []
  doc.descendants((node, pos) => {
    if (isTaskItem(node)) items.push({ node, pos })
    return true
  })
  return items
}

function setTaskItemChecked(tr, pos, node, checked) {
  if (!isTaskItem(node) || checkedOf(node) === checked) return false

  tr.setNodeMarkup(pos, undefined, {
    ...node.attrs,
    checked,
  })
  return true
}

function safeNodeAt(doc, pos) {
  if (!doc || pos < 0 || pos > doc.content.size) return null
  try {
    return doc.nodeAt(pos)
  } catch {
    return null
  }
}

function isTaskRelatedTransaction(transaction) {
  if (!transaction.docChanged) return false

  return transaction.steps.some((step) => {
    try {
      const json = JSON.stringify(step.toJSON())
      return json.includes('taskItem') || json.includes('taskList') || json.includes('checked')
    } catch {
      return true
    }
  })
}

function checkedChangedTaskItems(oldDoc, newDoc) {
  const changed = []
  newDoc.descendants((node, pos) => {
    if (!isTaskItem(node)) return true

    const oldNode = safeNodeAt(oldDoc, pos)
    if (isTaskItem(oldNode) && checkedOf(oldNode) !== checkedOf(node)) {
      changed.push({ node, pos, checked: checkedOf(node) })
    }
    return true
  })
  return changed
}

function cascadeManualParentChanges(tr, oldDoc, newDoc) {
  let changed = false

  for (const item of checkedChangedTaskItems(oldDoc, newDoc)) {
    const currentNode = safeNodeAt(tr.doc, item.pos)
    const descendants = taskItemDescendants(tr.doc, currentNode, item.pos)
    if (!descendants.length) continue

    for (const descendant of descendants) {
      const node = safeNodeAt(tr.doc, descendant.pos)
      changed = setTaskItemChecked(tr, descendant.pos, node, item.checked) || changed
    }
  }

  return changed
}

function normalizeParentCheckedStates(tr) {
  let changed = false

  // 子节点 position 总是大于父节点，倒序可保证先归一化子树，再回推父级。
  // 父级完成态只由直接子级决定；孙级先影响其父级，再逐层向上冒泡。
  // 部分子项完成时父级保持未完成，不展示半选态。
  const taskItems = collectTaskItems(tr.doc).sort((left, right) => right.pos - left.pos)
  for (const item of taskItems) {
    const node = safeNodeAt(tr.doc, item.pos)
    const children = directChildTaskItems(tr.doc, node, item.pos)
    if (!children.length) continue

    const allChildrenChecked = children.every((child) => checkedOf(safeNodeAt(tr.doc, child.pos)))
    changed = setTaskItemChecked(tr, item.pos, node, allChildrenChecked) || changed
  }

  return changed
}

export const TaskListCascade = Extension.create({
  name: 'taskListCascade',

  addProseMirrorPlugins() {
    return [
      new Plugin({
        key: pluginKey,

        appendTransaction: (transactions, oldState, newState) => {
          if (!transactions.some((transaction) => isTaskRelatedTransaction(transaction))) return null
          if (transactions.some((transaction) => transaction.getMeta(pluginKey))) return null

          const tr = newState.tr
          let changed = cascadeManualParentChanges(tr, oldState.doc, newState.doc)
          changed = normalizeParentCheckedStates(tr) || changed
          if (!changed) return null

          tr.setMeta(pluginKey, true)
          return tr
        },
      }),
    ]
  },
})
