// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import { readFileSync } from 'node:fs'
import test from 'node:test'
import { Window } from 'happy-dom'

// Execute the actual QML probe and callback, including the clipboard fallback.
const qml = readFileSync(new URL('../../src/gui/mainwindow/WebEngineView.qml', import.meta.url), 'utf8')
const contextMenu = qml.slice(qml.indexOf('var nativeEditFlags = Number(req.editFlags);'))
const expression = contextMenu.split('tiptapWebView.runJavaScript(')[1].split(',\n                            function(result)')[0]
const probe = new Function('sx', 'sy', `return (${expression})`)('200', '400')
const callback = contextMenu.slice(contextMenu.indexOf('function(result)')).split('\n                            });')[0] + '}'

function setup({ clipboard = true, editable = true, nativeFlags = 0 } = {}) {
  const window = new Window()
  const document = window.document
  document.body.innerHTML = `
    <div id="toolbar-host"><button id="toolbar-button"></button></div>
    <div id="content-scroll">
      <div id="note-title-host"><textarea id="note-title-input"></textarea></div>
      <div id="app"><div class="ProseMirror" contenteditable="true">
        <p id="text">text</p>
        <div class="voiceInfoBox" data-type="voice-block" data-voice-meta="voice-meta"></div>
        <div class="translateText" contenteditable="false">transcript</div>
        <img data-rel-path="images/test.png" src="test.png">
      </div></div>
    </div>`
  let focusCount = 0
  const selection = { empty: true, from: 3, to: 3 }
  window.__dvnTiptapEditor = {
    isEditable: editable,
    state: { selection },
    view: { focus() { focusCount++ } },
  }
  const actions = {}
  const actionManager = {
    resetCtxMenu() {}, visibleAction() {},
    enableAction(action, enabled) { actions[action] = enabled },
  }
  for (const name of ['TxtSelectAll', 'TxtCopy', 'TxtCut', 'TxtPaste', 'TxtDelete', 'TxtSpeech', 'TxtDictation']) actionManager[name] = name
  let clipboardQueries = 0
  const runCallback = new Function('ActionManager', 'TiptapChannel', 'nativeEditFlags', 'handler', 'txtCtxMenu', 'Qt', 'rawX', 'rawY', 'req', `return (${callback})`)(
    actionManager, { clipboardHasPasteContent() { clipboardQueries++; return clipboard } }, nativeFlags,
    { onSaveMenuParam() {}, onContextMenuRequested() {} }, { popup() {} }, { point() {} }, 200, 400, {},
  )
  return {
    window, document, selection, actions,
    get focusCount() { return focusCount },
    get clipboardQueries() { return clipboardQueries },
    menu(selector) {
      document.elementFromPoint = () => document.querySelector(selector)
      const result = new Function('window', 'document', 'Range', `return ${probe}`)(window, document, window.Range)
      runCallback(result)
      return JSON.parse(result)
    },
  }
}

for (const selector of ['#app', '#content-scroll', '#text']) {
  test(`paste is available with clipboard content at ${selector}, even without native CanPaste`, () => {
    const env = setup()
    env.menu(selector)
    assert.equal(env.actions.TxtPaste, true)
    assert.equal(env.actions.TxtSelectAll, true)
    assert.equal(env.actions.TxtCut, false)
    assert.equal(env.actions.TxtDelete, false)
    assert.equal(env.clipboardQueries, 1)
  })
  test(`empty clipboard keeps paste disabled at ${selector}`, () => {
    const env = setup({ clipboard: false })
    env.menu(selector)
    assert.equal(env.actions.TxtPaste, false)
  })
}

test('blank context restores focus without moving the existing editor selection', () => {
  const env = setup()
  env.menu('#app')
  assert.equal(env.focusCount, 1)
  assert.deepEqual(env.selection, { empty: true, from: 3, to: 3 })
})

test('native CanPaste fallback still works on blank body', () => {
  const env = setup({ clipboard: false, nativeFlags: 8 })
  env.menu('#app')
  assert.equal(env.actions.TxtPaste, true)
})

for (const selector of ['#note-title-input', '#note-title-host', '#toolbar-button', 'body', '.translateText']) {
  test(`body paste fallback does not enable ${selector}`, () => {
    const env = setup()
    env.menu(selector)
    assert.equal(env.actions.TxtPaste, false)
    assert.equal(env.actions.TxtCut, false)
    assert.equal(env.actions.TxtDelete, false)
    assert.equal(env.focusCount, 0)
    assert.equal(env.clipboardQueries, 0)
  })
}

for (const selector of ['#app', '#content-scroll', '#text']) {
  test(`read-only editor does not enable paste at ${selector}`, () => {
    const env = setup({ editable: false })
    env.menu(selector)
    assert.equal(env.actions.TxtPaste, false)
    assert.equal(env.focusCount, 0)
  })
}

test('selected body text retains copy/cut/delete on blank body', () => {
  const env = setup()
  const range = env.document.createRange()
  range.selectNodeContents(env.document.querySelector('#text'))
  env.window.getSelection().addRange(range)
  env.menu('#app')
  for (const action of ['TxtCopy', 'TxtCut', 'TxtDelete']) assert.equal(env.actions[action], true)
})

for (const [selector, type] of [['img', 0], ['.voiceInfoBox', 1]]) {
  test(`${selector} still uses its dedicated menu`, () => {
    const env = setup()
    assert.equal(env.menu(selector).type, type)
    assert.equal(env.focusCount, 0)
    assert.equal(env.clipboardQueries, 0)
  })
}
