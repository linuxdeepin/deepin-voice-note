// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

function findEmptyTextBlockFromSelection(editor) {
  const { $from } = editor.state.selection
  for (let depth = $from.depth; depth > 0; depth--) {
    const node = $from.node(depth)
    if (node.isTextblock && node.content.size === 0) {
      return node
    }
  }
  return null
}

function findSingleEmptyTextBlock(editor) {
  const doc = editor.state.doc
  if (!doc || doc.childCount !== 1) return null
  const child = doc.firstChild
  return child?.isTextblock && child.content.size === 0 ? child : null
}

export function describeEmptyPlaceholderBlock(editor) {
  if (!editor?.state?.doc) return { empty: false, block: '', level: '' }

  const blockNode = findSingleEmptyTextBlock(editor)
  if (!blockNode) return { empty: false, block: '', level: '' }

  const selectedBlockNode = findEmptyTextBlockFromSelection(editor) || blockNode
  const block = selectedBlockNode.type?.name || ''
  const level = block === 'heading' ? String(selectedBlockNode.attrs?.level || '') : ''
  return { empty: true, block, level }
}

export function syncEmptyPlaceholderState(editor, appElement) {
  if (!appElement) return

  const state = describeEmptyPlaceholderBlock(editor)
  appElement.classList.toggle('is-empty', state.empty)
  appElement.dataset.emptyBlock = state.empty ? state.block : ''
  appElement.dataset.emptyHeadingLevel = state.empty ? state.level : ''
}

export function installEmptyPlaceholderState(editor, appElement) {
  const sync = () => syncEmptyPlaceholderState(editor, appElement)
  editor.on('create', sync)
  editor.on('update', sync)
  // setContent(..., { emitUpdate: false }) is used when loading a note.
  // It still changes the ProseMirror transaction state, so sync here as well;
  // otherwise the empty placeholder can remain visible over loaded/typed text.
  editor.on('transaction', sync)
  sync()

  return () => {
    editor.off?.('create', sync)
    editor.off?.('update', sync)
    editor.off?.('transaction', sync)
  }
}
