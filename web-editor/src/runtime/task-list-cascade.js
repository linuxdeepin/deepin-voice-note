// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { Plugin, PluginKey } from '@tiptap/pm/state'
import { taskCheckboxIntentKey } from '../extensions/task-checkbox-intent.js'

export const taskListCascadePluginKey = new PluginKey('taskListCascade')
export const taskListNormalizationDoneKey = new PluginKey('taskListNormalizationDone')

const LIST_TYPES = new Set(['bulletList', 'orderedList', 'taskList'])
const LIST_ITEM_TYPES = new Set(['listItem', 'taskItem'])

function isTaskItem(node) {
  return node?.type?.name === 'taskItem'
}

function isListItem(node) {
  return LIST_ITEM_TYPES.has(node?.type?.name)
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
    if (isListItem($pos.node(index))) depth += 1
  }
  return depth
}

function taskItemVisualDepthAt(doc, pos, node = safeNodeAt(doc, pos)) {
  if (!isTaskItem(node)) return 0
  const structuralDepth = taskItemStructuralDepthAt(doc, pos)
  return Math.min(3, structuralDepth + normalizedIndentLevel(node, structuralDepth))
}

// 连续的顶层列表块共用一棵视觉树：列表类型切换会生成多个兄弟块，
// 但视觉缩进仍可能表示同一任务分支。普通段落/其他非列表块是硬边界，
// 会打断父子关系；重新转回列表后，相邻列表块才能重新建立视觉父子关系。
// 所有 listItem/taskItem 都参与缩进栈，有序/无序项可作为任务的边界。
function buildVisualTaskGraph(doc) {
  const byPos = new Map()
  const stack = []

  doc.forEach((scope, scopePos) => {
    if (!LIST_TYPES.has(scope.type?.name)) {
      stack.length = 0
      return
    }

    const entries = []
    scope.descendants((node, relativePos) => {
      if (!isListItem(node)) return true
      const pos = scopePos + 1 + relativePos
      const structuralDepth = taskItemStructuralDepthAt(doc, pos)
      const visualDepth = Math.min(3, structuralDepth + normalizedIndentLevel(node, structuralDepth))
      entries.push({
        node,
        pos,
        visualDepth,
        children: [],
      })
      return true
    })

    // 只有前一列表块的末尾本来就处于嵌套视觉层，且后一列表的首项
    // 仍显示为子级，才允许在块边界恢复父子关系。相邻的独立根级列表
    // 不能仅因 Tab 将下一块首项下沉就反向改动前块的待办状态。
    if (stack.length && (!entries.length || stack[stack.length - 1].visualDepth < 2
        || entries[0].visualDepth < 2)) stack.length = 0
    for (const entry of entries) {
      while (stack.length && stack[stack.length - 1].visualDepth >= entry.visualDepth) {
        stack.pop()
      }
      const parent = stack[stack.length - 1]
      if (parent) parent.children.push(entry)
      stack.push(entry)
      byPos.set(entry.pos, entry)
    }
  })

  return byPos
}

function taskChildren(entry) {
  return entry?.children?.filter((child) => isTaskItem(child.node)) || []
}

function taskItemDescendants(doc, node, basePos) {
  const graph = buildVisualTaskGraph(doc)
  const descendants = []
  const seen = new Set()

  const visit = (pos) => {
    const entry = graph.get(pos)
    for (const child of taskChildren(entry)) {
      if (seen.has(child.pos)) continue
      seen.add(child.pos)
      descendants.push({ node: child.node, pos: child.pos })
      visit(child.pos)
    }
  }

  visit(basePos)
  return descendants
}

function directChildTaskItems(doc, node, basePos) {
  const graph = buildVisualTaskGraph(doc)
  return taskChildren(graph.get(basePos)).map((child) => ({ node: child.node, pos: child.pos }))
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

function cascadeExplicitCheckboxChanges(tr, transactions) {
  let changed = false
  transactions.forEach((transaction, index) => {
    for (const intent of transaction.getMeta(taskCheckboxIntentKey) || []) {
      // 位置仅定位这次明确操作；通过事务映射跟随后续编辑，不充当节点身份。
      let mapped = transaction.mapping.slice(intent.mappingIndex).mapResult(intent.pos)
      if (mapped.deleted) continue
      let pos = mapped.pos
      let deleted = false
      for (const later of transactions.slice(index + 1)) {
        mapped = later.mapping.mapResult(pos)
        if (mapped.deleted) { deleted = true; break }
        pos = mapped.pos
      }
      const currentNode = safeNodeAt(tr.doc, pos)
      if (deleted || !isTaskItem(currentNode) || checkedOf(currentNode) !== intent.checked) continue
      for (const descendant of taskItemDescendants(tr.doc, currentNode, pos)) {
        changed = setTaskItemChecked(tr, descendant.pos, safeNodeAt(tr.doc, descendant.pos), intent.checked) || changed
      }
    }
  })
  return changed
}

export function normalizeTaskParentCheckedStates(tr) {
  let changed = false

  // 子节点 position 总是大于父节点，倒序可保证先归一化子树，再回推父级。
  // 父级完成态只由直接子级决定；孙级先影响其父级，再逐层向上冒泡。
  // 部分子项完成时父级保持未完成，不展示半选态。
  const graph = buildVisualTaskGraph(tr.doc)
  const taskEntries = [...graph.values()]
    .filter((entry) => isTaskItem(entry.node))
    .sort((left, right) => right.pos - left.pos)
  for (const entry of taskEntries) {
    const children = taskChildren(entry)
    if (!children.length) continue

    const allChildrenChecked = children.every((child) => checkedOf(safeNodeAt(tr.doc, child.pos)))
    changed = setTaskItemChecked(tr, entry.pos, safeNodeAt(tr.doc, entry.pos), allChildrenChecked) || changed
  }

  return changed
}

export const TaskListCascade = Extension.create({
  name: 'taskListCascade',

  addProseMirrorPlugins() {
    return [
      new Plugin({
        key: taskListCascadePluginKey,

        appendTransaction: (transactions, _oldState, newState) => {
          // Undo/Redo 必须原样恢复历史中的 checked 状态。历史事务里的
          // true -> false 不是用户手动取消父项，若再跑一次级联会把子项
          // 一并取消，导致一次 Undo 连上一次子项勾选也回退。
          if (transactions.some((transaction) => transaction.getMeta('history$') != null)) return null
          if (transactions.some((transaction) => transaction.getMeta(taskListNormalizationDoneKey))) return null
          if (!transactions.some((transaction) => transaction.docChanged)) return null

          const manualCascadeDisabled = transactions.some(
            (transaction) => transaction.getMeta(taskListCascadePluginKey),
          )
          const tr = newState.tr
          let changed = false
          if (!manualCascadeDisabled) {
            changed = cascadeExplicitCheckboxChanges(tr, transactions)
          }
          changed = normalizeTaskParentCheckedStates(tr) || changed
          if (!changed) return null

          // splitListItem 会通过 ensureMarks 记录下一次输入的文字格式。
          // 父项因新建未完成待办而归一化时，此追加事务只修改勾选状态，
          // 不能把原事务的 storedMarks 清掉，否则已完成项 Enter 后格式丢失。
          if (newState.selection.empty && newState.storedMarks !== null) {
            tr.setStoredMarks(newState.storedMarks)
          }
          tr.setMeta(taskListCascadePluginKey, true)
          tr.setMeta(taskListNormalizationDoneKey, true)
          return tr
        },
      }),
    ]
  },
})
