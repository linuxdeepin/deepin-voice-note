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

test('normal body text remains searchable while voice blocks are excluded', () => {
  const editor = createEditor()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'paragraph',
      content: [{ type: 'text', text: '正文关键词可以被搜索' }],
    }, {
      type: 'voiceBlock',
      attrs: {
        voiceId: 'voice-search-js',
        voicePath: 'voicenote/search.wav',
        voiceSize: 1000,
        title: '20260920 13.42.36',
        text: '语音转写关键词也不应该被搜索',
        translateUnfold: false,
      },
    }],
  })

  const translateText = editor.view.dom.querySelector('.translateText')
  const voiceInfoBox = editor.view.dom.querySelector('.voiceInfoBox')
  assert.ok(translateText, 'voice transcript should render')
  assert.ok(voiceInfoBox, 'voice playback box should render')
  assert.equal(translateText.style.display, 'none', 'folded transcript remains hidden')

  // Ordinary note text must keep the normal inline search highlight.
  setTiptapSearchQuery(editor, '正文关键词')
  let state = searchPluginKey.getState(editor.state)
  assert.equal(state.matches.length, 1)
  const bodyMatch = editor.view.dom.querySelector('.dvn-search-match')
  assert.ok(bodyMatch, 'ordinary body text should still be highlighted')
  assert.equal(bodyMatch.textContent, '正文关键词')

  // Nothing rendered by the voice block is searchable: not its bar metadata
  // and not its transcript.
  for (const query of ['20260920', '语音转写关键词']) {
    setTiptapSearchQuery(editor, query)
    state = searchPluginKey.getState(editor.state)
    assert.equal(state.matches.length, 0, `voice block must not match query: ${query}`)
    assert.equal(state.matchedVoiceIds.size, 0)
    assert.equal(state.matchedVoiceTranscriptIds.size, 0)
    assert.equal(translateText.querySelector('.dvn-search-match'), null)
    assert.equal(translateText.querySelector('.dvn-search-current'), null)
    assert.equal(voiceInfoBox.classList.contains('dvn-search-voice-match'), false)
    assert.equal(voiceInfoBox.style.outline, '', 'search must not add an outline to the voice bar')
    assert.equal(translateText.style.display, 'none', 'search must not unfold voice transcript')
  }

  // Clearing remains effective for ordinary body highlights.
  setTiptapSearchQuery(editor, '正文关键词')
  assert.ok(editor.view.dom.querySelector('.dvn-search-match'))
  clearTiptapSearch(editor)
  assert.equal(editor.view.dom.querySelector('.dvn-search-match'), null)
  assert.equal(translateText.style.display, 'none')
  editor.destroy()
})
