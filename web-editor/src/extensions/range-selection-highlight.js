// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { Plugin, PluginKey } from '@tiptap/pm/state'
import { Decoration, DecorationSet } from '@tiptap/pm/view'

const RANGE_SELECTED_NODE_TYPES = new Set(['image', 'voiceBlock'])

function createRangeSelectionDecorations(doc, selection) {
  if (selection.empty) return DecorationSet.empty

  const decorations = []
  doc.descendants((node, pos) => {
    if (!RANGE_SELECTED_NODE_TYPES.has(node.type.name)) return

    const nodeEnd = pos + node.nodeSize
    if (selection.from < nodeEnd && selection.to > pos) {
      decorations.push(Decoration.node(pos, nodeEnd, { class: 'dvn-range-selected' }))
    }
  })

  return DecorationSet.create(doc, decorations)
}

/**
 * 给选区范围内的图片、语音块补上统一的可视选中态。
 *
 * NodeView 的 selectNode() 只会响应 NodeSelection；Ctrl+A、右键全选以及
 * 跨资源拖选产生的是普通范围选区或 AllSelection，不会触发该方法。这里用
 * 节点 Decoration 表达“资源位于非折叠选区内”，避免只选中文字时资源无反馈。
 */
export const RangeSelectionHighlight = Extension.create({
  name: 'rangeSelectionHighlight',

  addProseMirrorPlugins() {
    return [
      new Plugin({
        key: new PluginKey('dvnRangeSelectionHighlight'),
        props: {
          decorations(state) {
            return createRangeSelectionDecorations(state.doc, state.selection)
          },
        },
      }),
    ]
  },
})
