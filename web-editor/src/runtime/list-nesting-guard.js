// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { Plugin } from '@tiptap/pm/state'

import { isInListItem, liftActiveListItem, sinkActiveListItem } from './list-behavior.js'

const LIST_ITEM_TYPES = new Set(['listItem', 'taskItem'])
const MAX_LIST_NESTING_DEPTH = 3

function listItemDepthAt(doc, pos) {
  const $pos = doc.resolve(Math.min(pos + 1, doc.content.size))
  let depth = 0
  for (let index = 1; index <= $pos.depth; index += 1) {
    if (LIST_ITEM_TYPES.has($pos.node(index).type.name)) depth += 1
  }
  return depth
}

function normalizedIndentLevel(value, structuralDepth) {
  const level = Number(value)
  const maxExtraIndent = Math.max(0, MAX_LIST_NESTING_DEPTH - structuralDepth)
  if (!Number.isFinite(level) || level <= 0 || maxExtraIndent <= 0) return 0
  return Math.min(maxExtraIndent, Math.floor(level))
}

// 统一列表/待办列表键盘缩进规则：Tab 最多进入三层，Shift+Tab 反缩进。
export const ListNestingGuard = Extension.create({
  name: 'listNestingGuard',
  priority: 1000,

  addGlobalAttributes() {
    return [{
      types: ['listItem', 'taskItem'],
      attributes: {
        dvnIndentLevel: {
          default: 0,
          parseHTML: (element) => Number(element.getAttribute('data-dvn-indent-level')) || 0,
          renderHTML: (attributes) => {
            const level = Number(attributes.dvnIndentLevel) || 0
            return level > 0 ? { 'data-dvn-indent-level': String(level) } : {}
          },
        },
      },
    }]
  },


  addProseMirrorPlugins() {
    return [new Plugin({
      appendTransaction: (transactions, _oldState, newState) => {
        if (!transactions.some((transaction) => transaction.docChanged)) return null

        let tr = null
        newState.doc.descendants((node, pos) => {
          if (!LIST_ITEM_TYPES.has(node.type.name)) return true

          const nextIndentLevel = normalizedIndentLevel(
            node.attrs?.dvnIndentLevel,
            listItemDepthAt(newState.doc, pos),
          )
          if ((Number(node.attrs?.dvnIndentLevel) || 0) === nextIndentLevel) return true

          tr ||= newState.tr
          tr.setNodeMarkup(pos, undefined, {
            ...node.attrs,
            dvnIndentLevel: nextIndentLevel,
          })
          return true
        })

        return tr
      },
    })]
  },

  addKeyboardShortcuts() {
    return {
      Tab: () => {
        if (!isInListItem(this.editor)) return false
        return sinkActiveListItem(this.editor)
      },
      'Shift-Tab': () => {
        if (!isInListItem(this.editor)) return false
        return liftActiveListItem(this.editor)
      },
    }
  },
})
