// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { Plugin } from '@tiptap/pm/state'

import {
  isInListItem,
  joinParagraphBackwardIntoPreviousList,
  joinParagraphForwardIntoCurrentList,
  liftActiveListItem,
  normalizeWholeDocumentSelection,
  selectionCoversListItems,
  sinkActiveListItem,
} from './list-behavior.js'

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
      // 入口门槛不能用 isInListItem()：全选 / 拖选全文后，文档首块可能已经是
      // 被反缩进踢出列表的普通段落，$from 不在 listItem 里，但选区仍然覆盖着
      // 一批列表项，必须继续处理，否则第一次 Shift+Tab 之后就再也按不动。
      Tab: () => {
        normalizeWholeDocumentSelection(this.editor)
        if (!isInListItem(this.editor) && !selectionCoversListItems(this.editor)) return false
        return sinkActiveListItem(this.editor)
      },
      'Shift-Tab': () => {
        normalizeWholeDocumentSelection(this.editor)
        if (!isInListItem(this.editor) && !selectionCoversListItems(this.editor)) return false
        return liftActiveListItem(this.editor)
      },
      // 行首 Backspace 与 Shift+Tab 同语义：逐级反缩进，一级时退出列表
      // （变成普通文本块）。旧实现走 core Keymap 的 joinBackward，会把当前项
      // 并进上一个列表项、生成“一个 listItem 多个段落”的结构，于是无论按多少
      // 次都降不到一级、也取消不掉列表属性。
      // 只在“无选区 + 光标已在文本块行首”时接管，行内删除仍交回默认行为。
      // Delete 方向与 Backspace 对称：光标在列表末项文本末尾时，ProseMirror 默认
      // joinForward 会把紧随其后的普通段落包成新的列表项，空段落场景表现为“多出
      // 一个空项再消失”，待办列表还会凭空多出一条未勾选项。这里改为把后一段落
      // 直接并入当前项（空段落即删除该空行），保持三种列表行为一致。
      Delete: () => joinParagraphForwardIntoCurrentList(this.editor),
      Backspace: () => {
        if (!isInListItem(this.editor)) {
          // 紧跟列表块之后的普通段落：行首 Backspace 并入上一个列表项（删除该行），
          // 不再让 ProseMirror 默认的 joinBackward 把它重新包成列表项。
          return joinParagraphBackwardIntoPreviousList(this.editor)
        }
        const { selection } = this.editor.state
        if (!selection.empty || selection.$from.parentOffset !== 0) return false
        // 刚由输入规则（"- " / "1. " 等）生成的列表：优先撤销输入规则，
        // 否则会把刚生成的列表项直接弹成段落，用户看不到自己输入的记号。
        if (this.editor.commands.undoInputRule()) return true
        return liftActiveListItem(this.editor)
      },
    }
  },
})
