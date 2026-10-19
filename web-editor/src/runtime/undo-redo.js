// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { history, redo, undo } from '@tiptap/pm/history'
import { keymap } from '@tiptap/pm/keymap'
import { AllSelection, Selection } from '@tiptap/pm/state'

export const UndoRedo = Extension.create({
  name: 'undoRedo',

  addCommands() {
    return {
      undo:
        () =>
        ({ state, dispatch }) => undo(state, dispatch),
      redo:
        () =>
        ({ state, dispatch }) => redo(state, dispatch),
    }
  },

  addProseMirrorPlugins() {
    return [
      history(),
      keymap({
        'Mod-z': undo,
        'Shift-Mod-z': redo,
        'Mod-y': redo,
      }),
    ]
  },
})

export function clearUndoRedoHistory(editor) {
  const historyPlugin = editor.state.plugins.find(plugin => plugin.key === 'history$')
  const initHistory = historyPlugin?.spec?.state?.init
  if (!historyPlugin || typeof initHistory !== 'function') return false

  const emptyHistory = initHistory(null, editor.state)
  const tr = editor.state.tr
    .setMeta(historyPlugin, { historyState: emptyHistory })
    .setMeta('addToHistory', false)
    .setMeta('preventUpdate', true)
  editor.view.dispatch(tr)
  return true
}

// setContent 只替换文档内容，旧选区会按位置映射进新文档。Ctrl+A 产生的
// AllSelection.map() 直接返回“新文档的全选”，于是切换笔记后新笔记整篇呈选中态
// （看上去像上一个笔记的全选没有取消）。加载完成后把非空选区收敛到文首，
// 避免任何选区跨笔记残留。
function collapseSelectionAfterContentReplace(editor) {
  const { selection } = editor.state
  if (selection.empty && !(selection instanceof AllSelection)) return

  const start = Selection.atStart(editor.state.doc)
  editor.view.dispatch(
    editor.state.tr
      .setSelection(start)
      .setMeta('addToHistory', false)
      .setMeta('preventUpdate', true),
  )

  // 编辑器未持有键盘焦点时，ProseMirror 不会把状态选区同步回 DOM，浏览器会继续
  // 绘制上一个笔记的选区（表现为高亮不消失），这里手动清掉这段残留。
  if (!editor.isFocused && typeof window !== 'undefined' && window.getSelection) {
    window.getSelection()?.removeAllRanges()
  }
}

export function replaceEditorContentWithoutHistory(editor, content) {
  editor
    .chain()
    .setMeta('addToHistory', false)
    .setContent(content, { emitUpdate: false })
    .run()
  collapseSelectionAfterContentReplace(editor)
  clearUndoRedoHistory(editor)
}
