// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { history, redo, undo } from '@tiptap/pm/history'
import { keymap } from '@tiptap/pm/keymap'

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

export function replaceEditorContentWithoutHistory(editor, content) {
  editor
    .chain()
    .setMeta('addToHistory', false)
    .setContent(content, { emitUpdate: false })
    .run()
  clearUndoRedoHistory(editor)
}
