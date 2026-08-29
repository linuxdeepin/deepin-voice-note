// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createEmptyDoc } from '../src/schema/document-envelope.js'
import { setTiptapSearchQuery, clearTiptapSearch, searchPluginKey } from '../src/runtime/search-extension.js'

function defineGlobal(name, value) {
  Object.defineProperty(globalThis, name, {
    value,
    writable: true,
    configurable: true,
  })
}

function setupDom() {
  const window = new Window()
  defineGlobal('window', window)
  defineGlobal('document', window.document)
  defineGlobal('navigator', window.navigator)
  defineGlobal('Node', window.Node)
  defineGlobal('Element', window.Element)
  defineGlobal('HTMLElement', window.HTMLElement)
  defineGlobal('DocumentFragment', window.DocumentFragment)
  defineGlobal('Event', window.Event)
  defineGlobal('CustomEvent', window.CustomEvent)
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('getComputedStyle', window.getComputedStyle.bind(window))
  defineGlobal('requestAnimationFrame', (cb) => setTimeout(cb, 0))
  defineGlobal('cancelAnimationFrame', (id) => clearTimeout(id))
  return window
}

function createEditor() {
  setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
  return editor
}

test('search extension highlights only document text without changing JSON', () => {
  const editor = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{ type: 'paragraph', content: [{ type: 'text', text: 'hello Tiptap search' }] }],
  })
  const before = JSON.stringify(editor.getJSON())

  assert.equal(setTiptapSearchQuery(editor, 'tiptap'), true)
  const state = searchPluginKey.getState(editor.state)
  assert.equal(state.matches.length, 1)
  assert.ok(editor.view.dom.querySelector('.dvn-search-match'))
  assert.equal(JSON.stringify(editor.getJSON()), before, 'search must not mutate persisted document')

  clearTiptapSearch(editor)
  assert.equal(searchPluginKey.getState(editor.state).matches.length, 0)
  assert.equal(editor.view.dom.querySelector('.dvn-search-match'), null)
  editor.destroy()
})

test('search extension highlights voice transcript at runtime and restores folded state', () => {
  const editor = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'voiceBlock',
      attrs: {
        voiceId: 'voice-search-js',
        voicePath: 'voicenote/search.wav',
        voiceSize: 1000,
        title: '会议录音',
        text: '这是一段可以搜索的转写内容',
        translateUnfold: false,
      },
    }],
  })

  const translateText = editor.view.dom.querySelector('.translateText')
  assert.ok(translateText, 'voice transcript should render')
  assert.equal(translateText.style.display, 'none', 'folded transcript is hidden before search')

  setTiptapSearchQuery(editor, '搜索')
  const state = searchPluginKey.getState(editor.state)
  assert.equal(state.matchedVoiceTranscriptIds.has('voice-search-js'), true)
  assert.notEqual(translateText.style.display, 'none', 'matched folded transcript is shown at runtime')
  assert.ok(translateText.querySelector('.dvn-search-match'), 'matched transcript text is highlighted')

  clearTiptapSearch(editor)
  assert.equal(translateText.style.display, 'none', 'clearing search restores folded transcript visibility')
  assert.equal(translateText.querySelector('.dvn-search-match'), null)
  editor.destroy()
})
