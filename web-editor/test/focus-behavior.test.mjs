// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'

import { shouldFocusEditorOnDocumentMouseDown } from '../src/runtime/focus-behavior.js'

test('document mousedown focus helper ignores editor and toolbar clicks', () => {
  const window = new Window()
  const document = window.document
  document.body.innerHTML = `
    <div class="tiptap-toolbar"><button id="toolbarButton">无序</button></div>
    <div id="note-title-host"><input id="note-title-input" /></div>
    <div class="ProseMirror"><p id="editorText">text</p></div>
    <div id="outside"></div>
  `

  assert.equal(shouldFocusEditorOnDocumentMouseDown(document.getElementById('toolbarButton')), false)
  assert.equal(shouldFocusEditorOnDocumentMouseDown(document.querySelector('.tiptap-toolbar')), false)
  assert.equal(shouldFocusEditorOnDocumentMouseDown(document.getElementById('editorText')), false)
  assert.equal(shouldFocusEditorOnDocumentMouseDown(document.getElementById('note-title-input')), false)
  assert.equal(shouldFocusEditorOnDocumentMouseDown(document.getElementById('note-title-host')), false)
  assert.equal(shouldFocusEditorOnDocumentMouseDown(document.getElementById('outside')), true)
  assert.equal(shouldFocusEditorOnDocumentMouseDown(document.body), true)
})
