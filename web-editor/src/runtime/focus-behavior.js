// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

function closestElement(target, selector) {
  if (!target) return null
  if (typeof target.closest === 'function') return target.closest(selector)
  return target.parentElement?.closest?.(selector) || null
}

export function shouldFocusEditorOnDocumentMouseDown(target, button = 0) {
  // Opening a context menu must not move the saved caret/selection to the end.
  return button !== 2
    && !closestElement(target, '.ProseMirror')
    && !closestElement(target, '.tiptap-toolbar')
    && !closestElement(target, '#note-title-input')
    && !closestElement(target, '#note-title-host')
}
