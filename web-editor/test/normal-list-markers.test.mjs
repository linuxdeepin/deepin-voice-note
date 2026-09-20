// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { syncOrderedListMarkers } from '../src/runtime/normal-list-markers.js'

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
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('requestAnimationFrame', (cb) => setTimeout(cb, 0))
  defineGlobal('cancelAnimationFrame', (id) => clearTimeout(id))
}

test('ordered list marker sync writes runtime counter reset from start attr', () => {
  setupDom()
  document.body.innerHTML = '<div class="ProseMirror"><ol start="3"><li><p>a</p></li><li><p>b</p></li></ol></div>'

  syncOrderedListMarkers(document)

  const list = document.querySelector('ol')
  assert.equal(list.style.counterReset, 'dvn-ordered-list 2')
  assert.equal(document.querySelectorAll('ol > li').length, 2)
})
