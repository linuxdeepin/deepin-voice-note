// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { Mark } from '@tiptap/pm/model'
import { Plugin, PluginKey } from '@tiptap/pm/state'

export const emptyBlockTypingMarksPluginKey = new PluginKey('emptyBlockTypingMarks')
const emptyBlockTypingMarksUpdateKey = new PluginKey('emptyBlockTypingMarksUpdate')

const TEXTBLOCK_TYPES = new Set(['paragraph', 'heading'])
const MARK_TYPES = new Set([
  'bold', 'italic', 'underline', 'strike', 'color', 'highlight',
  'fontFamily', 'fontSize', 'fontWeight',
])

function primitiveAttrs(attrs) {
  if (!attrs || typeof attrs !== 'object' || Array.isArray(attrs)) return {}
  return Object.fromEntries(
    Object.entries(attrs)
      .filter(([, value]) => value == null || ['string', 'number', 'boolean'].includes(typeof value))
      .sort(([left], [right]) => left.localeCompare(right)),
  )
}

export function normalizeTypingMarksSnapshot(snapshot) {
  if (!Array.isArray(snapshot)) return null
  const byType = new Map()
  for (const item of snapshot) {
    if (!item || typeof item !== 'object' || !MARK_TYPES.has(item.type)) continue
    byType.set(item.type, {
      type: item.type,
      attrs: primitiveAttrs(item.attrs),
    })
  }
  const normalized = [...byType.values()].sort((left, right) => left.type.localeCompare(right.type))
  return normalized.length ? normalized : null
}

function snapshotFromMarks(marks) {
  return normalizeTypingMarksSnapshot((marks || []).map((mark) => ({
    type: mark.type.name,
    attrs: mark.attrs,
  })))
}

function marksFromSnapshot(state, snapshot) {
  const normalized = normalizeTypingMarksSnapshot(snapshot)
  if (!normalized) return []
  const marks = normalized.flatMap(({ type, attrs }) => {
    const markType = state.schema.marks[type]
    return markType ? [markType.create(attrs)] : []
  })
  return marks.length ? Mark.setFrom(marks) : []
}

function sameSnapshot(left, right) {
  return JSON.stringify(normalizeTypingMarksSnapshot(left))
    === JSON.stringify(normalizeTypingMarksSnapshot(right))
}

function selectedEmptyTextblock(state) {
  const { selection } = state
  if (!selection.empty) return null
  const { $from } = selection
  if (!TEXTBLOCK_TYPES.has($from.parent?.type?.name) || $from.parent.content.size !== 0) return null
  return {
    node: $from.parent,
    pos: $from.before($from.depth),
  }
}

function blockSnapshot(info) {
  return normalizeTypingMarksSnapshot(info?.node?.attrs?.dvnTypingMarks)
}

function marksAtOldSelection(state) {
  const info = selectedEmptyTextblock(state)
  if (info?.node?.attrs?.dvnTypingMarks) return marksFromSnapshot(state, blockSnapshot(info))
  if (state.storedMarks) return Mark.setFrom(state.storedMarks)

  const { selection, doc } = state
  if (!selection.empty) {
    let firstTextMarks = null
    doc.nodesBetween(selection.from, selection.to, (node) => {
      if (!node.isText) return true
      firstTextMarks = node.marks
      return false
    })
    if (firstTextMarks) return Mark.setFrom(firstTextMarks)
  }
  return Mark.setFrom(selection.$from.marks() || [])
}

function restoreEmptyBlockTypingMarks(view) {
  const info = selectedEmptyTextblock(view.state)
  if (!info) return
  const snapshot = blockSnapshot(info)
  const currentSnapshot = snapshotFromMarks(view.state.storedMarks || [])
  if (sameSnapshot(currentSnapshot, snapshot)) return
  view.dispatch(
    view.state.tr
      .setStoredMarks(snapshot ? marksFromSnapshot(view.state, snapshot) : null)
      .setMeta(emptyBlockTypingMarksUpdateKey, true),
  )
}

function parseTypingMarksAttribute(element) {
  const raw = element.getAttribute('data-dvn-typing-marks')
  if (!raw) return null
  try {
    return normalizeTypingMarksSnapshot(JSON.parse(raw))
  } catch (_error) {
    return null
  }
}

export const EmptyBlockTypingMarks = Extension.create({
  name: 'emptyBlockTypingMarks',

  addGlobalAttributes() {
    return [{
      types: [...TEXTBLOCK_TYPES],
      attributes: {
        dvnTypingMarks: {
          default: undefined,
          keepOnSplit: true,
          parseHTML: parseTypingMarksAttribute,
          renderHTML: (attributes) => {
            const snapshot = normalizeTypingMarksSnapshot(attributes.dvnTypingMarks)
            return snapshot ? { 'data-dvn-typing-marks': JSON.stringify(snapshot) } : {}
          },
        },
      },
    }]
  },

  addProseMirrorPlugins() {
    return [new Plugin({
      key: emptyBlockTypingMarksPluginKey,

      view(view) {
        queueMicrotask(() => {
          if (!view.isDestroyed) restoreEmptyBlockTypingMarks(view)
        })
        return {
          update(nextView) {
            queueMicrotask(() => {
              if (!nextView.isDestroyed) restoreEmptyBlockTypingMarks(nextView)
            })
          },
        }
      },

      appendTransaction: (transactions, oldState, newState) => {
        if (transactions.some((transaction) => transaction.getMeta(emptyBlockTypingMarksUpdateKey))) return null
        if (transactions.some((transaction) => transaction.getMeta('history$') != null)) return null
        if (transactions.some((transaction) => transaction.getMeta('preventUpdate'))) return null

        const current = selectedEmptyTextblock(newState)
        if (!current) return null

        const docChanged = transactions.some((transaction) => transaction.docChanged)
        const explicitMarksSet = transactions.some((transaction) => transaction.storedMarksSet && !transaction.docChanged)
        const currentSnapshot = blockSnapshot(current)

        // 选区命令会清空 ProseMirror 的临时 storedMarks。必须在同一事务链中
        // 恢复空段落持久化的输入格式，保证下一个输入字符直接继承该格式。
        if (!explicitMarksSet && !docChanged) {
          const currentTypingSnapshot = snapshotFromMarks(newState.storedMarks || [])
          if (sameSnapshot(currentTypingSnapshot, currentSnapshot)) return null
          const tr = newState.tr
            .setStoredMarks(currentSnapshot ? marksFromSnapshot(newState, currentSnapshot) : null)
            .setMeta(emptyBlockTypingMarksUpdateKey, true)
            .setMeta('preventClearDocument', true)
          return tr
        }

        let nextSnapshot = currentSnapshot
        let nextMarks = marksFromSnapshot(newState, currentSnapshot)
        if (explicitMarksSet) {
          nextSnapshot = snapshotFromMarks(newState.storedMarks)
          nextMarks = marksFromSnapshot(newState, nextSnapshot)
        } else if (docChanged) {
          const oldInfo = selectedEmptyTextblock(oldState)
          const blockCreatedOrEmptied = !oldInfo || oldInfo.pos !== current.pos
          if (!blockCreatedOrEmptied) return null
          const inheritedMarks = marksAtOldSelection(oldState)
          nextSnapshot = snapshotFromMarks(inheritedMarks)
          nextMarks = inheritedMarks
        }

        if (sameSnapshot(currentSnapshot, nextSnapshot)) return null
        const tr = newState.tr.setNodeMarkup(current.pos, undefined, {
          ...current.node.attrs,
          dvnTypingMarks: nextSnapshot ?? undefined,
        })
        tr.setStoredMarks(nextMarks)
        tr.setMeta(emptyBlockTypingMarksUpdateKey, true)
        tr.setMeta('preventClearDocument', true)
        return tr
      },
    })]
  },
})
