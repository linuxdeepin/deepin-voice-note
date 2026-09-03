// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 图片粘贴决策单测：剪贴板图片落盘 / 远程图片阻止 / 笔记内默认保留 attrs

import assert from 'node:assert/strict'
import { readFileSync } from 'node:fs'
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
import { NodeSelection } from '@tiptap/pm/state'
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
  defineGlobal('MouseEvent', window.MouseEvent)
  defineGlobal('KeyboardEvent', window.KeyboardEvent)
  defineGlobal('FileReader', window.FileReader)
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


test('image insertion and click expose theme-color selected overlay bound to image node', () => {
  const { editor, img, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')

  // The selected cover must be part of the image node itself, not a manually
  // positioned global overlay.  This makes the cover follow the current visual
  // image box through zoom, scroll and relayout without coordinate conversion.
  const style = document.getElementById('dvn-tiptap-image-block-style')
  assert.ok(style, 'image selection style should be injected')
  const imageBlockCss = readFileSync(new URL('../src/extensions/image-block.css', import.meta.url), 'utf8')
  assert.match(imageBlockCss, /dvn-image-node-selection/)
  assert.match(imageBlockCss, /--dvn-active-selection-bg/)
  assert.doesNotMatch(imageBlockCss, /control-holder|control-sizing|dvn-image-selection-bg/)
  assert.doesNotMatch(
    imageBlockCss,
    /p:has\([^{}]+\)\s*\{[^}]*line-height:\s*0/s,
    'pure image paragraph must keep normal line-height so typing after image still creates text',
  )

  const wrapper = img.closest('[data-dvn-image-node]')
  assert.ok(wrapper, 'image should be rendered by the image node view')
  const overlay = wrapper.querySelector('[data-testid="tiptap-image-selection"]')
  assert.ok(overlay, 'image selection overlay should be mounted inside the image node')
  assert.equal(overlay.parentNode, wrapper)

  img.dispatchEvent(new Event('click', { bubbles: true, cancelable: true }))
  assert.equal(editor.state.selection.node?.type?.name, 'image')
  assert.ok(wrapper.classList.contains('dvn-image-node-selected'), 'clicked image node should expose selected state')
  destroy()
  editor.destroy()
})


test('image selection overlay is node-bound and does not store stale zoom geometry', () => {
  const { editor, img, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')

  const wrapper = img.closest('[data-dvn-image-node]')
  const overlay = wrapper?.querySelector('[data-testid="tiptap-image-selection"]')
  assert.ok(wrapper, 'image wrapper should exist')
  assert.ok(overlay, 'node-bound selection overlay should exist')

  img.dispatchEvent(new Event('click', { bubbles: true, cancelable: true }))
  window.dispatchEvent(new CustomEvent('dvn-tiptap-content-zoom-changed', { detail: { zoom: 1.5 } }))

  assert.equal(overlay.parentNode, wrapper)
  assert.equal(overlay.style.left, '', 'node-bound overlay should not cache viewport left')
  assert.equal(overlay.style.top, '', 'node-bound overlay should not cache viewport top')
  assert.equal(overlay.style.width, '', 'node-bound overlay should not cache viewport width')
  assert.equal(overlay.style.height, '', 'node-bound overlay should not cache viewport height')
  assert.ok(wrapper.classList.contains('dvn-image-node-selected'))
  destroy()
  editor.destroy()
})

test('mousedown on the right side of an image moves caret after the image', () => {
  const { editor, img, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')

  Object.defineProperty(img, 'getBoundingClientRect', {
    configurable: true,
    value: () => ({ left: 10, top: 20, width: 120, height: 80, right: 130, bottom: 100 }),
  })

  const event = new MouseEvent('mousedown', {
    bubbles: true,
    cancelable: true,
    clientX: 150,
    clientY: 40,
  })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, true, 'runtime should replace default image node selection')
  assert.equal(editor.state.selection.constructor.name, 'TextSelection')
  assert.equal(editor.state.selection.from, 2, 'caret should be in the paragraph after image')
  assert.deepEqual(editor.getJSON().content.map(node => node.type), ['paragraph'])
  assert.equal(editor.getJSON().content[0].content[0].type, 'image')
  destroy()
  editor.destroy()
})

test('typing after a clicked image keeps text after the image in the same paragraph', () => {
  const { editor, img, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')

  Object.defineProperty(img, 'getBoundingClientRect', {
    configurable: true,
    value: () => ({ left: 10, top: 20, width: 120, height: 80, right: 130, bottom: 100 }),
  })

  const event = new MouseEvent('mousedown', {
    bubbles: true,
    cancelable: true,
    clientX: 150,
    clientY: 40,
  })
  editor.view.dom.dispatchEvent(event)
  editor.commands.insertContent('hello')

  const doc = editor.getJSON()
  assert.equal(doc.content.length, 1, 'text typed after an image must not create a new paragraph')
  assert.equal(doc.content[0].type, 'paragraph')
  assert.deepEqual(doc.content[0].content.map(node => node.type), ['image', 'text'])
  assert.equal(doc.content[0].content[1].text, 'hello')
  destroy()
  editor.destroy()
})

test('mousedown in editor right padding of a full-width image moves caret after image', () => {
  const { editor, img, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')

  editor.view.dom.style.paddingLeft = '30px'
  editor.view.dom.style.paddingRight = '20px'
  editor.view.dom.getBoundingClientRect = () => ({ left: 0, right: 320, top: 0, bottom: 200, width: 320, height: 200 })
  const paragraph = img.closest('p')
  paragraph.getBoundingClientRect = () => ({ left: 30, right: 300, top: 20, bottom: 100, width: 270, height: 80 })
  img.getBoundingClientRect = () => ({ left: 30, right: 300, top: 20, bottom: 100, width: 270, height: 80 })

  const event = new MouseEvent('mousedown', {
    bubbles: true,
    cancelable: true,
    clientX: 315,
    clientY: 40,
  })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, true, 'runtime should treat editor right padding as the image-line tail')
  assert.equal(editor.state.selection.constructor.name, 'TextSelection')
  assert.equal(editor.state.selection.from, 2, 'caret should be after the full-width image')
  destroy()
  editor.destroy()
})

test('mousedown on the right side of an image ignores trailing hardBreak', () => {
  const { editor, destroy } = setupEditorWithImage()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'paragraph',
      content: [
        { type: 'image', attrs: { src: 'file:///usr/share/x/images/photo.png', relPath: 'images/photo.png', alt: '', title: null } },
        { type: 'hardBreak' },
      ],
    }],
  })
  const img = editor.view.dom.querySelector('img[data-rel-path]')
  assert.ok(img, 'image element should be rendered')
  img.getBoundingClientRect = () => ({ left: 0, right: 100, top: 0, bottom: 40, width: 100, height: 40 })

  const event = new MouseEvent('mousedown', { clientX: 120, clientY: 20, bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, true, 'runtime should treat trailing hardBreak as empty image-line tail')
  assert.equal(editor.state.selection.constructor.name, 'TextSelection')
  assert.equal(editor.state.selection.from, 2, 'caret should be after image and before hardBreak')
  destroy()
  editor.destroy()
})

test('ArrowRight from selected image moves caret after the image', () => {
  const { editor, destroy } = setupEditorWithImage()
  editor.view.dispatch(editor.state.tr.setSelection(NodeSelection.create(editor.state.doc, 1)))

  const event = new KeyboardEvent('keydown', { key: 'ArrowRight', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, true)
  assert.equal(editor.state.selection.constructor.name, 'TextSelection')
  assert.equal(editor.state.selection.from, 2, 'caret should be in the paragraph after image')
  assert.deepEqual(editor.getJSON().content.map(node => node.type), ['paragraph'])
  assert.equal(editor.getJSON().content[0].content[0].type, 'image')
  destroy()
  editor.destroy()
})

test('ArrowDown is not intercepted when adjacent line is normal text', () => {
  const { editor, destroy } = setupEditorWithImage()
  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'first' }] },
      { type: 'paragraph', content: [{ type: 'text', text: 'second' }] },
    ],
  })
  editor.commands.setTextSelection(3)

  const event = new KeyboardEvent('keydown', { key: 'ArrowDown', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, false, 'normal text navigation must be left to ProseMirror/browser')
  destroy()
  editor.destroy()
})

test('ArrowDown from text line moves caret to adjacent bare image paragraph', () => {
  const { editor, destroy } = setupEditorWithImage()
  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'first' }] },
      { type: 'paragraph', content: [{ type: 'image', attrs: { src: 'file:///x/a.png', relPath: 'images/a.png', alt: '', title: null } }] },
      { type: 'paragraph', content: [
        { type: 'image', attrs: { src: 'file:///x/b.png', relPath: 'images/b.png', alt: '', title: null } },
        { type: 'text', text: 'third' },
      ] },
    ],
  })
  editor.commands.setTextSelection(3)

  const event = new KeyboardEvent('keydown', { key: 'ArrowDown', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, true)
  assert.equal(editor.state.selection.constructor.name, 'TextSelection')
  assert.equal(editor.state.selection.from, 9, 'caret should land after image in the second line, not third-line text')
  destroy()
  editor.destroy()
})

test('ArrowUp from text line moves caret to adjacent bare image paragraph', () => {
  const { editor, destroy } = setupEditorWithImage()
  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'first' }] },
      { type: 'paragraph', content: [{ type: 'image', attrs: { src: 'file:///x/a.png', relPath: 'images/a.png', alt: '', title: null } }] },
      { type: 'paragraph', content: [
        { type: 'image', attrs: { src: 'file:///x/b.png', relPath: 'images/b.png', alt: '', title: null } },
        { type: 'text', text: 'third' },
      ] },
    ],
  })
  editor.commands.setTextSelection(11)

  const event = new KeyboardEvent('keydown', { key: 'ArrowUp', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, true)
  assert.equal(editor.state.selection.constructor.name, 'TextSelection')
  assert.equal(editor.state.selection.from, 9, 'caret should land after image in the second line')
  destroy()
  editor.destroy()
})

test('ArrowUp and ArrowDown preserve caret side between consecutive image paragraphs', () => {
  const { editor, destroy } = setupEditorWithImage()
  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'image', attrs: { src: 'file:///x/a.png', relPath: 'images/a.png', alt: '', title: null } }] },
      { type: 'paragraph', content: [{ type: 'image', attrs: { src: 'file:///x/b.png', relPath: 'images/b.png', alt: '', title: null } }] },
    ],
  })

  editor.commands.setTextSelection(1)
  const downFromBefore = new KeyboardEvent('keydown', { key: 'ArrowDown', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(downFromBefore)
  assert.equal(downFromBefore.defaultPrevented, true)
  assert.equal(editor.state.selection.from, 4, 'before first image + ArrowDown should land before second image')

  editor.commands.setTextSelection(5)
  const upFromAfter = new KeyboardEvent('keydown', { key: 'ArrowUp', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(upFromAfter)
  assert.equal(upFromAfter.defaultPrevented, true)
  assert.equal(editor.state.selection.from, 2, 'after second image + ArrowUp should land after first image')

  destroy()
  editor.destroy()
})

test('ArrowLeft and ArrowRight walk predictable before/after image positions', () => {
  const { editor, destroy } = setupEditorWithImage()
  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'image', attrs: { src: 'file:///x/a.png', relPath: 'images/a.png', alt: '', title: null } }] },
      { type: 'paragraph', content: [{ type: 'image', attrs: { src: 'file:///x/b.png', relPath: 'images/b.png', alt: '', title: null } }] },
    ],
  })

  editor.commands.setTextSelection(1)
  const rightInsideLine = new KeyboardEvent('keydown', { key: 'ArrowRight', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(rightInsideLine)
  assert.equal(rightInsideLine.defaultPrevented, true)
  assert.equal(editor.state.selection.from, 2, 'ArrowRight before image should move after the same image')

  const rightToNextLine = new KeyboardEvent('keydown', { key: 'ArrowRight', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(rightToNextLine)
  assert.equal(rightToNextLine.defaultPrevented, true)
  assert.equal(editor.state.selection.from, 4, 'ArrowRight after image should move before the next image line')

  const leftToPreviousLine = new KeyboardEvent('keydown', { key: 'ArrowLeft', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(leftToPreviousLine)
  assert.equal(leftToPreviousLine.defaultPrevented, true)
  assert.equal(editor.state.selection.from, 2, 'ArrowLeft before image should move after the previous image line')

  const leftInsideLine = new KeyboardEvent('keydown', { key: 'ArrowLeft', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(leftInsideLine)
  assert.equal(leftInsideLine.defaultPrevented, true)
  assert.equal(editor.state.selection.from, 1, 'ArrowLeft after image should move before the same image')

  destroy()
  editor.destroy()
})

test('mousedown on text after image is not intercepted by trailing image caret helper', () => {
  const { editor, img, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')

  editor.commands.insertContent({ type: 'text', text: 'abc' })
  Object.defineProperty(img, 'getBoundingClientRect', {
    configurable: true,
    value: () => ({ left: 10, top: 20, width: 120, height: 80, right: 130, bottom: 100 }),
  })

  const event = new MouseEvent('mousedown', {
    bubbles: true,
    cancelable: true,
    clientX: 150,
    clientY: 40,
  })
  editor.view.dom.dispatchEvent(event)

  assert.equal(event.defaultPrevented, false, 'text after image should use normal ProseMirror caret hit testing')
  assert.equal(editor.state.selection.constructor.name, 'TextSelection')
  assert.equal(editor.getJSON().content[0].content[1].text, 'abc')
  destroy()
  editor.destroy()
})

test('dblclick on image requests view original with img src', () => {
  const { editor, img, viewCalls, destroy } = setupEditorWithImage()
  assert.ok(img, 'image element should be rendered')
  img.dispatchEvent(new Event('dblclick', { bubbles: true, cancelable: true }))
  assert.equal(viewCalls.length, 1)
  assert.equal(viewCalls[0], img.getAttribute('src'))
  destroy()
  editor.destroy()
})

test('contextmenu on image selects node and leaves host picture menu available', () => {
  const { editor, img, viewCalls, destroy } = setupEditorWithImage()
  const event = new Event('contextmenu', { bubbles: true, cancelable: true })
  img.dispatchEvent(event)

  assert.equal(event.defaultPrevented, false, 'contextmenu should not be swallowed by a web menu')
  assert.equal(viewCalls.length, 0, 'contextmenu should not trigger view-original directly')
  assert.equal(document.querySelector('[data-testid="tiptap-image-menu"]'), null, 'web image menu should not be mounted')
  assert.equal(editor.state.selection.node?.type?.name, 'image', 'image should be selected for host menu actions')
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
  defineGlobal('MouseEvent', window.MouseEvent)
  defineGlobal('KeyboardEvent', window.KeyboardEvent)
  defineGlobal('FileReader', window.FileReader)
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



test('setupImagePaste captures selection before async image roundtrip', () => {
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
  defineGlobal('MouseEvent', window.MouseEvent)
  defineGlobal('KeyboardEvent', window.KeyboardEvent)
  defineGlobal('FileReader', window.FileReader)
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

  let captures = 0
  const bridge = { jsPasteImage() {}, jsRequestViewPicture() {} }
  const destroy = setupImagePaste(editor, bridge, {
    captureInsertionSelection: () => { captures += 1 },
  })

  const event = {
    clipboardData: clipboard({
      items: [{
        kind: 'file',
        type: 'image/png',
        getAsFile: () => new window.File(['x'], 'pasted.png', { type: 'image/png' }),
      }],
    }),
    preventDefault() { this.defaultPrevented = true },
    defaultPrevented: false,
  }

  assert.equal(editor.view.props.handlePaste(editor.view, event), true)
  assert.equal(event.defaultPrevented, true)
  assert.equal(captures, 1)

  destroy()
  editor.destroy()
})
