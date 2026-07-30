// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 图片粘贴决策单测：剪贴板图片落盘 / 远程图片阻止 / 笔记内默认保留 attrs

import assert from 'node:assert/strict'
import test, { before } from 'node:test'
import { Window } from 'happy-dom'
import {
  analyzePaste,
  extractImageSrcs,
  hasRemoteImageInHtml,
  isRemoteImageSrc,
} from '../src/runtime/image-interactions.js'

function clipboard({ items = [], html = '', text = '' } = {}) {
  return {
    items,
    getData(type) {
      if (type === 'text/html') return html
      if (type === 'text/plain') return text
      return ''
    },
  }
}

function imageItem(type = 'image/png') {
  return { kind: 'file', type, getAsFile: () => ({ name: 'pasted.png' }) }
}

function defineGlobal(name, value) {
  Object.defineProperty(globalThis, name, { value, writable: true, configurable: true })
}

// 提供全局 DOM / DOMParser，供基于 DOM 解析的图片 src 提取使用
function setupDom() {
  const window = new Window()
  defineGlobal('window', window)
  defineGlobal('document', window.document)
  defineGlobal('DOMParser', window.DOMParser)
}
before(setupDom)

test('analyzePaste: clipboard image item → saveImage', () => {
  const cd = clipboard({ items: [imageItem('image/png')] })
  const decision = analyzePaste(cd)
  assert.equal(decision.action, 'saveImage')
  assert.ok(decision.imageItem)
})

test('analyzePaste: remote http img in HTML without image item → blockRemote', () => {
  const cd = clipboard({ html: '<p><img src="https://example.com/a.png"></p>' })
  const decision = analyzePaste(cd)
  assert.equal(decision.action, 'blockRemote')
})

test('analyzePaste: remote https img in HTML → blockRemote', () => {
  const cd = clipboard({ html: '<img src=\'https://cdn.example.com/b.jpg\'>' })
  assert.equal(analyzePaste(cd).action, 'blockRemote')
})

test('analyzePaste: local file:// img in HTML (intra-note) → default', () => {
  const cd = clipboard({ html: '<img src="file:///usr/share/x/images/a.png" data-rel-path="images/a.png">' })
  assert.equal(analyzePaste(cd).action, 'default')
})

test('analyzePaste: relative images/ img in HTML → default', () => {
  const cd = clipboard({ html: '<img src="images/a.png">' })
  assert.equal(analyzePaste(cd).action, 'default')
})

test('analyzePaste: plain text only → default', () => {
  const cd = clipboard({ text: 'hello', html: '' })
  assert.equal(analyzePaste(cd).action, 'default')
})

test('analyzePaste: null clipboardData → default', () => {
  assert.equal(analyzePaste(null).action, 'default')
})

test('analyzePaste: image item takes precedence over remote HTML (local data is safe)', () => {
  const cd = clipboard({
    items: [imageItem('image/png')],
    html: '<img src="https://example.com/a.png">',
  })
  assert.equal(analyzePaste(cd).action, 'saveImage')
})

// ---------------------------------------------------------------------------
// 远程图片绕过场景：协议相对路径 // 与无引号 src
// ---------------------------------------------------------------------------

test('analyzePaste: protocol-relative // img in HTML → blockRemote', () => {
  const cd = clipboard({ html: '<img src="//evil.com/track.png">' })
  assert.equal(analyzePaste(cd).action, 'blockRemote')
})

test('analyzePaste: unquoted http src → blockRemote', () => {
  const cd = clipboard({ html: '<img src=http://evil.com/track.png>' })
  assert.equal(analyzePaste(cd).action, 'blockRemote')
})

test('analyzePaste: unquoted https src → blockRemote', () => {
  const cd = clipboard({ html: "<img src=https://evil.com/track.png>" })
  assert.equal(analyzePaste(cd).action, 'blockRemote')
})

test('analyzePaste: unquoted protocol-relative // src → blockRemote', () => {
  const cd = clipboard({ html: "<img src=//evil.com/track.png>" })
  assert.equal(analyzePaste(cd).action, 'blockRemote')
})

test('analyzePaste: mixed local and remote imgs in HTML → blockRemote', () => {
  const cd = clipboard({ html: '<img src="images/a.png"><img src="//evil.com/b.png">' })
  assert.equal(analyzePaste(cd).action, 'blockRemote')
})

test('analyzePaste: intra-note copy keeps attrs (file:// src → default)', () => {
  const cd = clipboard({ html: '<img src="file:///usr/share/x/images/a.png" data-rel-path="images/a.png">' })
  assert.equal(analyzePaste(cd).action, 'default')
})

test('analyzePaste: img without src attribute → default', () => {
  const cd = clipboard({ html: '<img alt="no src">' })
  assert.equal(analyzePaste(cd).action, 'default')
})

test('analyzePaste: empty html string → default', () => {
  const cd = clipboard({ html: '' })
  assert.equal(analyzePaste(cd).action, 'default')
})

// ---------------------------------------------------------------------------
// isRemoteImageSrc / extractImageSrcs 单元断言
// ---------------------------------------------------------------------------

test('isRemoteImageSrc: http/https/protocol-relative are remote', () => {
  assert.equal(isRemoteImageSrc('http://x.com/a.png'), true)
  assert.equal(isRemoteImageSrc('https://x.com/a.png'), true)
  assert.equal(isRemoteImageSrc('//evil.com/a.png'), true)
})

test('isRemoteImageSrc: images/ relative and file:// are local', () => {
  assert.equal(isRemoteImageSrc('images/a.png'), false)
  assert.equal(isRemoteImageSrc('file:///usr/share/x/images/a.png'), false)
})

test('isRemoteImageSrc: empty / null / non-string are not remote', () => {
  assert.equal(isRemoteImageSrc(''), false)
  assert.equal(isRemoteImageSrc(null), false)
  assert.equal(isRemoteImageSrc(undefined), false)
})

test('extractImageSrcs: extracts quoted, unquoted and protocol-relative srcs', () => {
  const html = '<img src="https://a.com/1.png"><img src=//evil.com/2.png><img src="images/3.png">'
  const srcs = extractImageSrcs(html)
  assert.deepEqual(srcs, ['https://a.com/1.png', '//evil.com/2.png', 'images/3.png'])
})

test('hasRemoteImageInHtml: true when any remote src present', () => {
  assert.equal(hasRemoteImageInHtml('<img src="images/a.png"><img src="//evil.com/b.png">'), true)
  assert.equal(hasRemoteImageInHtml('<img src="images/a.png"><img src="file:///x/a.png">'), false)
  assert.equal(hasRemoteImageInHtml('no images here'), false)
})

// ---------------------------------------------------------------------------
// 图片查看原图 / 右键菜单交互（happy-dom）
// ---------------------------------------------------------------------------

import { Editor } from '@tiptap/core'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createEmptyDoc } from '../src/schema/document-envelope.js'
import { setupImageViewAndMenu } from '../src/runtime/image-interactions.js'

function setupEditorWithImage() {
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

  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
  editor.commands.insertContent({
    type: 'image',
    attrs: { src: 'file:///usr/share/x/images/photo.png', relPath: 'images/photo.png', alt: '', title: null },
  })

  const viewCalls = []
  const bridge = { jsRequestViewPicture(url) { viewCalls.push(url) } }
  const destroy = setupImageViewAndMenu(editor, bridge)

  const img = editor.view.dom.querySelector('img[data-rel-path]')
  return { editor, img, bridge, viewCalls, destroy }
}

test('dblclick on image requests view original with img src', () => {
  const { editor, img, viewCalls, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')
  img.dispatchEvent(new Event('dblclick', { bubbles: true, cancelable: true }))
  assert.equal(viewCalls.length, 1)
  assert.equal(viewCalls[0], img.getAttribute('src'))
  destroy()
  editor.destroy()
})

test('contextmenu on image shows menu with view and delete actions', () => {
  const { editor, img, viewCalls, destroy } = setupEditorWithImage()
  img.dispatchEvent(new Event('contextmenu', { bubbles: true, cancelable: true }))

  const menu = document.querySelector('[data-testid="tiptap-image-menu"]')
  assert.ok(menu, 'context menu should be mounted')

  // 查看原图
  menu.querySelector('[data-action="view-original"]').click()
  assert.equal(viewCalls.length, 1)
  assert.equal(viewCalls[0], img.getAttribute('src'))
  destroy()
  editor.destroy()
})

test('contextmenu delete action removes the image node', () => {
  const { editor, img, destroy } = setupEditorWithImage()
  img.dispatchEvent(new Event('contextmenu', { bubbles: true, cancelable: true }))

  const menu = document.querySelector('[data-testid="tiptap-image-menu"]')
  menu.querySelector('[data-action="delete-image"]').click()

  assert.ok(!editor.getJSON().content.find((n) => n.type === 'image'), 'image should be deleted')
  destroy()
  editor.destroy()
})

test('dblclick on non-image does not request view', () => {
  const { editor, viewCalls, destroy } = setupEditorWithImage()
  editor.view.dom.dispatchEvent(new Event('dblclick', { bubbles: true, cancelable: true }))
  assert.equal(viewCalls.length, 0)
  destroy()
  editor.destroy()
})

// ---------------------------------------------------------------------------
// destroy 恢复原 handlePaste（#5）
// ---------------------------------------------------------------------------

import { setupImagePaste } from '../src/runtime/image-interactions.js'

test('setupImagePaste destroy restores default paste (not permanently blocked)', () => {
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

  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })

  // 保存原有 handlePaste（可能为 undefined）
  const originalHandlePaste = editor.view.props.handlePaste

  const bridge = { jsPasteImage() {}, jsRequestViewPicture() {} }
  const destroy = setupImagePaste(editor, bridge)

  // destroy 后 handlePaste 应恢复为原值，而非永久关闭（() => false）
  destroy()
  assert.equal(editor.view.props.handlePaste, originalHandlePaste, 'destroy should restore original handlePaste')
  assert.notEqual(typeof editor.view.props.handlePaste, 'function' === false, 'destroyed handlePaste should not be () => false')
  editor.destroy()
})
