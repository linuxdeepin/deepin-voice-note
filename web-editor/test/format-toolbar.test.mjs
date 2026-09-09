// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 格式工具栏单测（happy-dom）：十项格式 apply/clear/激活态、setFontList 填充、常驻可见。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createFormatToolbar } from '../src/runtime/format-toolbar.js'
import { FORE_COLORS, BACK_COLORS, DARK_FORE_COLORS, DARK_BACK_COLORS } from '../src/runtime/format-palette.js'
import toolbarCss from '../src/runtime/format-toolbar.css?raw'
import tiptapEditorHtml from '../src/runtime/tiptap-editor.html?raw'
import { syncEmptyPlaceholderState } from '../src/runtime/empty-placeholder-state.js'
import { createEmptyDoc, createEnvelope, validateEnvelope } from '../src/schema/document-envelope.js'

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

function createEditorWithToolbar() {
  const window = setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
  const host = document.createElement('div')
  host.id = 'toolbar-host'
  document.body.appendChild(host)
  const toolbar = createFormatToolbar(editor, host)
  return { editor, host, toolbar, window, appElement: element }
}

function insertText(editor, text) {
  editor.commands.insertContent({ type: 'text', text })
}

// 选中首个段落内的全部文本（跳过文档边界位置 0），对 mark 与块级 toggle 均可用
function markEditorFocused(editor) {
  editor.isFocused = true
}

function selectText(editor) {
  markEditorFocused(editor)
  const size = editor.state.doc.content.size
  editor.chain().focus().setTextSelection({ from: 1, to: Math.max(1, size - 1) }).run()
}

// ---------------------------------------------------------------------------
// 常驻可见性
// ---------------------------------------------------------------------------

test('toolbar is always visible once mounted', () => {
  const { host, editor } = createEditorWithToolbar()
  const toolbar = host.querySelector('[data-testid="format-toolbar"]')
  assert.ok(toolbar, 'toolbar should be mounted in host')
  assert.notEqual(toolbar.style.display, 'none', 'toolbar must not be hidden')
  editor.destroy()
})

test('toolbar group separators are rendered on initial mount', () => {
  const { host, editor } = createEditorWithToolbar()
  const toolbar = host.querySelector('[data-testid="format-toolbar"]')
  assert.equal(
    toolbar.querySelectorAll('.tiptap-toolbar-separator').length,
    4,
    'all main toolbar group separators should be present before a resize',
  )
  editor.destroy()
})


test('toolbar icons use imported assets or sanitized SVG nodes and follow theme color', () => {
  const { host, editor } = createEditorWithToolbar()
  const toolbar = host.querySelector('[data-testid="format-toolbar"]')
  for (const format of ['bulletList', 'orderedList', 'taskList', 'insertVoice', 'insertImage']) {
    const button = toolbar.querySelector(`button[data-format="${format}"]`)
    assert.ok(button, `${format} button should exist`)
    const icon = button.querySelector('.tiptap-icon')
    assert.ok(icon, `${format} should render an icon node`)
    assert.ok(icon.querySelector('svg, img'), `${format} should render an icon node`)
    assert.equal(icon.querySelector('script'), null, `${format} must not render scripts`)
    assert.equal(icon.querySelector('foreignObject'), null, `${format} must not render foreign objects`)
  }

  assert.match(toolbarCss, /\.tiptap-icon--asset::after \{[\s\S]*background-color: currentColor;/)
  assert.match(toolbarCss, /\.tiptap-icon--asset::after \{[\s\S]*opacity: 1;/)
  assert.match(toolbarCss, /\.tiptap-icon--asset > img \{[\s\S]*opacity: 0;/)
  assert.match(toolbarCss, /\.tiptap-toolbar button\.is-active \.tiptap-icon--asset,[\s\S]*color: var\(--highlightColor/)

  editor.destroy()
})


test('toolbar keeps every tool visible at the default 616px editor pane width', () => {
  const { host, editor, window } = createEditorWithToolbar()
  const toolbar = host.querySelector('[data-testid="format-toolbar"]')
  let hostWidth = 616

  Object.defineProperty(host, 'clientWidth', {
    configurable: true,
    get: () => hostWidth,
  })
  Object.defineProperty(toolbar, 'scrollWidth', {
    configurable: true,
    get: () => 606,
  })

  window.dispatchEvent(new window.Event('resize'))
  assert.equal(toolbar.querySelector('[data-format="more"]'), null)
  for (const format of ['bold', 'italic', 'underline', 'strike', 'foreColor', 'backColor', 'bulletList', 'orderedList', 'taskList', 'insertVoice', 'insertImage']) {
    assert.equal(
      toolbar.querySelector(`[data-format="${format}"]`)?.closest('.tiptap-overflow-panel'),
      null,
      `${format} should remain visible at the default pane width`,
    )
  }

  hostWidth = 614
  window.dispatchEvent(new window.Event('resize'))
  assert.ok(toolbar.querySelector('[data-format="more"]'))
  editor.destroy()
})

test('more button anchors its tooltip to the button instead of native title', () => {
  const { host, editor, window } = createEditorWithToolbar()
  const toolbar = host.querySelector('[data-testid="format-toolbar"]')
  Object.defineProperty(host, 'clientWidth', {
    configurable: true,
    get: () => 600,
  })
  Object.defineProperty(toolbar, 'scrollWidth', {
    configurable: true,
    get: () => 700,
  })
  window.dispatchEvent(new window.Event('resize'))

  const moreButton = host.querySelector('button[data-format="more"]')
  assert.ok(moreButton)
  assert.equal(moreButton.getAttribute('title'), null)
  assert.equal(moreButton.getAttribute('data-tooltip'), '更多格式')
  assert.equal(moreButton.getAttribute('aria-label'), '更多格式')
  editor.destroy()
})

test('toolbar overflow moves buttons dynamically by available width', () => {
  const { host, editor, window } = createEditorWithToolbar()
  const toolbar = host.querySelector('[data-testid="format-toolbar"]')
  let hostWidth = 380

  Object.defineProperty(host, 'clientWidth', {
    configurable: true,
    get: () => hostWidth,
  })
  Object.defineProperty(toolbar, 'scrollWidth', {
    configurable: true,
    get: () => {
      const mainControls = Array.from(toolbar.querySelectorAll('[data-format]'))
        .filter((node) => !node.closest('.tiptap-overflow-panel'))
        .filter((node) => node.getAttribute('data-format') !== 'blockquote')
      return 100 + mainControls.length * 30
    },
  })

  window.dispatchEvent(new window.Event('resize'))
  let overflowCount = toolbar.querySelectorAll('.tiptap-overflow-panel [data-format]').length
  assert.ok(overflowCount > 0, 'narrow width should move trailing buttons into more panel')
  const visualOrder = ['bold', 'italic', 'underline', 'strike', 'foreColor', 'backColor', 'bulletList', 'orderedList', 'taskList', 'insertVoice', 'insertImage']
  let enteredOverflowSuffix = false
  for (const format of visualOrder) {
    const control = toolbar.querySelector(`[data-format=\"${format}\"]`)
    const inOverflow = Boolean(control?.closest('.tiptap-overflow-panel'))
    if (inOverflow) enteredOverflowSuffix = true
    if (enteredOverflowSuffix) {
      assert.equal(inOverflow, true, `${format} should be part of one reverse-order overflow suffix`)
    }
  }
  for (const format of ['insertImage', 'insertVoice']) {
    const control = toolbar.querySelector(`[data-format=\"${format}\"]`)
    assert.equal(Boolean(control?.closest('.tiptap-overflow-panel')), true, `${format} should overflow before earlier toolbar actions`)
  }
  const moreButton = toolbar.querySelector('button[data-format="more"]')
  assert.ok(moreButton, 'more button should exist when the toolbar overflows')
  assert.ok(moreButton.querySelector('img'), 'more should render an image asset')

  hostWidth = 410
  window.dispatchEvent(new window.Event('resize'))
  const widerOverflowCount = toolbar.querySelectorAll('.tiptap-overflow-panel [data-format]').length
  assert.ok(widerOverflowCount > 0, 'medium width should still keep some buttons overflowed')
  assert.ok(widerOverflowCount < overflowCount, 'wider width should overflow fewer buttons')

  hostWidth = 900
  window.dispatchEvent(new window.Event('resize'))
  assert.equal(toolbar.querySelectorAll('.tiptap-overflow-panel [data-format]').length, 0)
  assert.equal(toolbar.querySelector('[data-format="more"]'), null, 'wide width should restore all buttons to main toolbar')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 开关式格式：粗体 / 斜体 / 下划线 / 删除线 / 引用
// ---------------------------------------------------------------------------

for (const format of ['bold', 'italic', 'underline', 'strike', 'blockquote']) {
  test(`${format}: apply and clear toggles editor state and aria-pressed`, () => {
    const { editor, host } = createEditorWithToolbar()
    insertText(editor, 'hello')
    selectText(editor)

    const btn = host.querySelector(`button[data-format="${format}"]`)
    assert.ok(btn)

    btn.click()
    assert.equal(btn.getAttribute('aria-pressed'), 'true')
    assert.ok(editor.isActive(format), `${format} should be active after apply`)

    btn.click()
    assert.equal(btn.getAttribute('aria-pressed'), 'false')
    assert.ok(!editor.isActive(format), `${format} should be cleared after toggle off`)
    editor.destroy()
  })
}

// ---------------------------------------------------------------------------
// 标题下拉
// ---------------------------------------------------------------------------

test('heading dropdown applies and clears heading levels', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'title')
  selectText(editor)

  const select = host.querySelector('select[data-control="heading"]')
  assert.ok(select)

  select.value = '2'
  select.dispatchEvent(new Event('change'))
  assert.ok(editor.isActive('heading', { level: 2 }))

  select.value = 'p'
  select.dispatchEvent(new Event('change'))
  assert.ok(!editor.isActive('heading'))
  editor.destroy()
})

test('heading dropdown reflects collapsed cursor heading context', () => {
  const { editor, host } = createEditorWithToolbar()
  markEditorFocused(editor)
  const select = host.querySelector('select[data-control="heading"]')
  const label = host.querySelector('.tiptap-select-heading .tiptap-select-label')
  assert.ok(select && label)

  select.value = '1'
  select.dispatchEvent(new Event('change'))

  assert.ok(editor.isActive('heading', { level: 1 }))
  assert.equal(select.value, '1')
  assert.equal(label.textContent, '标题1')
  assert.equal(
    host.querySelector('.tiptap-select-heading .tiptap-select-option[data-value="1"]')?.getAttribute('aria-selected'),
    'true',
  )
  assert.equal(
    host.querySelector('.tiptap-select-heading .tiptap-select-option[data-value="p"]')?.getAttribute('aria-selected'),
    'false',
  )
  editor.destroy()
})

test('empty heading keeps the body placeholder on the active heading line', () => {
  const { editor, host, appElement } = createEditorWithToolbar()
  const select = host.querySelector('select[data-control="heading"]')
  assert.ok(select)

  syncEmptyPlaceholderState(editor, appElement)
  assert.equal(appElement.classList.contains('is-empty'), true)
  assert.equal(appElement.dataset.emptyBlock, 'paragraph')
  assert.equal(appElement.dataset.emptyHeadingLevel, '')

  select.value = '1'
  select.dispatchEvent(new Event('change'))
  syncEmptyPlaceholderState(editor, appElement)

  assert.equal(editor.view.dom.firstElementChild?.tagName, 'H1')
  assert.equal(appElement.classList.contains('is-empty'), true)
  assert.equal(appElement.dataset.emptyBlock, 'heading')
  assert.equal(appElement.dataset.emptyHeadingLevel, '1')
  assert.match(tiptapEditorHtml, /#app\.is-empty\[data-empty-heading-level="1"\] \.ProseMirror::before/)
  assert.match(tiptapEditorHtml, /\.ProseMirror h1, \.ProseMirror h2/)
  assert.match(tiptapEditorHtml, /\.ProseMirror::selection, \.ProseMirror ::selection \{ background: var\(--dvn-active-selection-bg/)
  assert.match(tiptapEditorHtml, /color: var\(--dvn-selection-fg, #ffffff\)/)
  assert.doesNotMatch(tiptapEditorHtml, /dvn-editor-empty-node/)

  insertText(editor, 'hello')
  syncEmptyPlaceholderState(editor, appElement)
  assert.equal(appElement.classList.contains('is-empty'), false)
  editor.destroy()
})

test('style dropdowns close each other and expose a scrollable custom menu', () => {
  const { editor, host } = createEditorWithToolbar()
  const heading = host.querySelector('.tiptap-select-heading')
  const font = host.querySelector('.tiptap-select-fontFamily')
  const size = host.querySelector('.tiptap-select-fontSize')
  assert.ok(heading && font && size)

  heading.querySelector('.tiptap-select-button').click()
  assert.equal(heading.classList.contains('is-open'), true)
  assert.equal(font.classList.contains('is-open'), false)

  font.querySelector('.tiptap-select-button').click()
  assert.equal(heading.classList.contains('is-open'), false)
  assert.equal(font.classList.contains('is-open'), true)
  assert.ok(font.querySelector('.tiptap-select-menu'))

  size.querySelector('.tiptap-select-button').click()
  assert.equal(font.classList.contains('is-open'), false)
  assert.equal(size.classList.contains('is-open'), true)
  editor.destroy()
})

test('heading dropdown follows the Sketch menu labels and type scale hooks', () => {
  const { editor, host } = createEditorWithToolbar()
  const heading = host.querySelector('.tiptap-select-heading')
  assert.ok(heading)

  const labels = Array.from(heading.querySelectorAll('.tiptap-select-option-label'))
    .map((node) => node.textContent)
  assert.deepEqual(labels, ['正文', '标题1', '标题2', '标题3', '标题4', '标题5'])

  const selected = heading.querySelector('.tiptap-select-option[data-value="p"]')
  assert.equal(selected?.getAttribute('aria-selected'), 'true')
  assert.equal(heading.querySelector('.tiptap-select-label')?.textContent, '正文')
  assert.match(toolbarCss, /\.tiptap-select-heading \.tiptap-select-menu \{[\s\S]*width: 184px;/)
  assert.match(toolbarCss, /\.tiptap-select-fontFamily \{[\s\S]*width: 74px;/)
  assert.match(toolbarCss, /select\[data-control="fontFamily"\] \{[\s\S]*width: 74px;/)
  assert.match(toolbarCss, /\.tiptap-toolbar \.tiptap-select-button \{[\s\S]*gap: 4px;/)
  assert.match(toolbarCss, /\.tiptap-toolbar \.tiptap-select-button \{[\s\S]*font-size: 12px;/)
  assert.match(toolbarCss, /\.tiptap-toolbar \.tiptap-select-button \.tiptap-select-arrow \{[\s\S]*width: 12px;/)
  assert.doesNotMatch(toolbarCss, /tiptap-select-button::after/, 'select arrow should use the shared SVG asset instead of a CSS triangle')
  assert.doesNotMatch(toolbarCss, /dvn-heading-menu-bg/, 'heading dropdown must share the themed menu background with other dropdowns')
  assert.doesNotMatch(toolbarCss, /dvn-heading-menu-border/, 'heading dropdown must share the themed menu border with other dropdowns')
  assert.match(toolbarCss, /\.tiptap-select-menu \{[\s\S]*background: var\(--dvn-menu-bg, var\(--dvn-panel-bg/)
  assert.match(toolbarCss, /\.tiptap-select-heading \.tiptap-select-option\[data-value="1"\] \{[\s\S]*--dvn-heading-option-font-size: 24px;/)
  assert.match(toolbarCss, /\.tiptap-select-heading \.tiptap-select-option\[data-value="2"\] \{[\s\S]*--dvn-heading-option-font-size: 21px;/)
  assert.equal(host.querySelector('.tiptap-select-option[data-value="6"]'), null, 'heading dropdown should not expose title 6')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 文字颜色面板
// ---------------------------------------------------------------------------

test('foreColor panel matches Summernote light palette and applies text color', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'colored')
  selectText(editor)

  const panel = host.querySelector('[data-panel="foreColor"]')
  assert.ok(panel)
  const cells = Array.from(panel.querySelectorAll('button[data-color]'))
  assert.deepEqual(cells.map((cell) => cell.getAttribute('data-color')), FORE_COLORS.flat())
  assert.equal(panel.querySelector('button[data-action]'), null, 'Summernote palette has no extra clear row')
  assert.match(toolbarCss, /\.tiptap-color-panel \{[\s\S]*grid-template-columns: repeat\(5, 22px\);/)
  assert.match(toolbarCss, /\.tiptap-color-panel button\[data-color\] \{[\s\S]*border-radius: 8px;/)

  const color = 'rgb(205, 35, 62)'
  const cell = cells.find((node) => node.getAttribute('data-color') === color)
  assert.ok(cell)

  cell.click()
  assert.ok(editor.isActive('color', { color }))
  assert.equal(
    host.querySelector('button[data-format="foreColor"]').style.getPropertyValue('--dvn-current-color'),
    color,
  )
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 背景色面板
// ---------------------------------------------------------------------------

test('backColor keeps the Summernote default transparent indicator', () => {
  const { editor, host } = createEditorWithToolbar()
  assert.equal(
    host.querySelector('button[data-format="backColor"]').style.getPropertyValue('--dvn-current-color'),
    'transparent',
  )
  editor.destroy()
})

test('backColor panel matches Summernote light palette and clears via transparent cell', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'highlighted')
  selectText(editor)

  const panel = host.querySelector('[data-panel="backColor"]')
  assert.ok(panel)
  const cells = Array.from(panel.querySelectorAll('button[data-color]'))
  assert.deepEqual(cells.map((cell) => cell.getAttribute('data-color')), BACK_COLORS.flat())

  const color = 'rgba(255, 215, 0, 0.2)'
  const target = cells.find((cell) => cell.getAttribute('data-color') === color)
  assert.ok(target)

  target.click()
  assert.ok(editor.isActive('highlight', { color }))
  assert.equal(
    host.querySelector('button[data-format="backColor"]').style.getPropertyValue('--dvn-current-color'),
    color,
  )

  const transparent = panel.querySelector('button[data-color="transparent"]')
  assert.ok(transparent)
  transparent.click()
  assert.ok(!editor.isActive('highlight'))
  editor.destroy()
})

test('color panels share dropdown theme and switch palettes in dark mode', () => {
  const { editor, host, window } = createEditorWithToolbar()
  const forePanel = host.querySelector('[data-panel="foreColor"]')
  const backPanel = host.querySelector('[data-panel="backColor"]')
  assert.ok(forePanel && backPanel)

  assert.match(toolbarCss, /\.tiptap-color-panel \{[\s\S]*background: var\(--dvn-menu-bg, var\(--dvn-panel-bg/)
  assert.match(toolbarCss, /\.tiptap-color-panel \{[\s\S]*border: 1px solid var\(--dvn-panel-border/)
  assert.match(toolbarCss, /\.tiptap-color-panel button\[data-color\] \{[\s\S]*var\(--dvn-color-chip-border/)
  assert.doesNotMatch(toolbarCss, /dvn-color-panel-bg/, 'color panels must not keep a light-only panel background token')

  document.documentElement.dataset.dvnTheme = 'dark'
  window.dispatchEvent(new window.CustomEvent('dvn-theme-applied', { detail: { theme: 'dark' } }))

  assert.deepEqual(
    Array.from(forePanel.querySelectorAll('button[data-color]')).map((cell) => cell.getAttribute('data-color')),
    DARK_FORE_COLORS.flat(),
  )
  assert.deepEqual(
    Array.from(backPanel.querySelectorAll('button[data-color]')).map((cell) => cell.getAttribute('data-color')),
    DARK_BACK_COLORS.flat(),
  )
  editor.destroy()
})

test('foreColor reflects collapsed cursor color context', () => {
  const { editor, host } = createEditorWithToolbar()
  markEditorFocused(editor)

  const color = 'rgb(1, 100, 255)'
  const panel = host.querySelector('[data-panel="foreColor"]')
  const cell = panel.querySelector(`button[data-color="${color}"]`)
  assert.ok(cell)
  cell.click()

  assert.equal(host.querySelector('button[data-format="foreColor"]').style.getPropertyValue('--dvn-current-color'), color)
  assert.equal(cell.getAttribute('aria-pressed'), 'true')
  editor.destroy()
})

test('backColor reflects collapsed cursor highlight context', () => {
  const { editor, host } = createEditorWithToolbar()
  markEditorFocused(editor)

  const color = 'rgba(130, 178, 255, 0.2)'
  const panel = host.querySelector('[data-panel="backColor"]')
  const cell = panel.querySelector(`button[data-color="${color}"]`)
  assert.ok(cell)
  cell.click()

  assert.equal(host.querySelector('button[data-format="backColor"]').style.getPropertyValue('--dvn-current-color'), color)
  assert.equal(cell.getAttribute('aria-pressed'), 'true')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 字体下拉
// ---------------------------------------------------------------------------

test('fontFamily dropdown applies and clears font family', () => {
  const { editor, host, toolbar } = createEditorWithToolbar()
  insertText(editor, 'font')
  selectText(editor)

  // 字体下拉需先由宿主下发填充
  toolbar.setFontList(['Arial', 'Noto Sans CJK SC'], 'Noto Sans CJK SC')
  const select = host.querySelector('select[data-control="fontFamily"]')
  assert.ok(select)

  select.value = 'Noto Sans CJK SC'
  select.dispatchEvent(new Event('change'))
  assert.ok(editor.isActive('fontFamily', { fontFamily: 'Noto Sans CJK SC' }))

  select.value = ''
  select.dispatchEvent(new Event('change'))
  assert.ok(!editor.isActive('fontFamily'))
  editor.destroy()
})

test('fontFamily dropdown reflects collapsed cursor font context', () => {
  const { editor, host, toolbar } = createEditorWithToolbar()
  markEditorFocused(editor)
  toolbar.setFontList(['Arial', 'Noto Sans CJK SC'], 'Noto Sans CJK SC')
  const select = host.querySelector('select[data-control="fontFamily"]')
  const label = host.querySelector('.tiptap-select-fontFamily .tiptap-select-label')
  assert.ok(select && label)

  select.value = 'Arial'
  select.dispatchEvent(new Event('change'))

  assert.ok(editor.isActive('fontFamily', { fontFamily: 'Arial' }))
  assert.equal(select.value, 'Arial')
  assert.equal(label.textContent, 'Arial')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 字号下拉
// ---------------------------------------------------------------------------

test('fontSize dropdown applies and clears font size', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'size')
  selectText(editor)

  const select = host.querySelector('select[data-control="fontSize"]')
  assert.ok(select)

  select.value = '18'
  select.dispatchEvent(new Event('change'))
  assert.ok(editor.isActive('fontSize', { fontSize: '18px' }))

  select.value = ''
  select.dispatchEvent(new Event('change'))
  assert.ok(!editor.isActive('fontSize'))
  editor.destroy()
})

test('fontSize dropdown reflects collapsed cursor size context without scaling menu items', () => {
  const { editor, host } = createEditorWithToolbar()
  markEditorFocused(editor)
  const select = host.querySelector('select[data-control="fontSize"]')
  const label = host.querySelector('.tiptap-select-fontSize .tiptap-select-label')
  assert.ok(select && label)

  select.value = '24'
  select.dispatchEvent(new Event('change'))

  assert.ok(editor.isActive('fontSize', { fontSize: '24px' }))
  assert.equal(select.value, '24')
  assert.equal(label.textContent, '24')
  for (const option of host.querySelectorAll('.tiptap-select-fontSize .tiptap-select-option')) {
    assert.equal(option.style.fontSize, '', 'font size menu items should keep a uniform menu type scale')
  }
  editor.destroy()
})

// ---------------------------------------------------------------------------
// setFontList 填充
// ---------------------------------------------------------------------------

test('setFontList populates the font family dropdown', () => {
  const { editor, host, toolbar } = createEditorWithToolbar()
  const select = host.querySelector('select[data-control="fontFamily"]')

  toolbar.setFontList(['Arial', 'Helvetica', 'Noto Sans CJK SC'], 'Helvetica')
  const options = select.querySelectorAll('option')
  assert.equal(options.length, 4, 'should keep a default display option + host fonts')
  assert.equal(options[0].value, '')
  assert.equal(options[0].textContent, 'Helvetica')
  assert.equal(options[0].selected, true, 'no explicit font mark should display default font')
  assert.equal(options[1].value, 'Arial')
  assert.equal(options[2].value, 'Helvetica')
  assert.equal(options[3].value, 'Noto Sans CJK SC')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 激活态随显式选区同步
// ---------------------------------------------------------------------------

test('active state syncs when explicit selection moves into formatted text', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'paragraph',
      content: [
        { type: 'text', text: 'bold', marks: [{ type: 'bold' }] },
        { type: 'text', text: 'plain' },
      ],
    }],
  })

  const btn = host.querySelector('button[data-format="bold"]')

  // 显式选中加粗文本：按钮高亮
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextRange(editor, 'bold')).run()
  assert.equal(btn.getAttribute('aria-pressed'), 'true')

  // 显式选中普通文本：按钮取消高亮
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextRange(editor, 'plain')).run()
  assert.equal(btn.getAttribute('aria-pressed'), 'false')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 资源插入区：图片按钮
// ---------------------------------------------------------------------------

test('toolbar mounts image insert button', () => {
  const { editor, host } = createEditorWithToolbar()
  const btn = host.querySelector('button[data-format="insertImage"]')
  assert.ok(btn, 'image insert button should be mounted')
  editor.destroy()
})

test('toolbar image button triggers injected pick callback', () => {
  const { editor, host, toolbar } = createEditorWithToolbar()
  let picked = 0
  toolbar.setOnPickImage(() => { picked++ })
  const btn = host.querySelector('button[data-format="insertImage"]')
  btn.click()
  assert.equal(picked, 1, 'pick callback should fire on image button click')
  editor.destroy()
})

test('resource buttons follow host enabled state', () => {
  const { editor, host, toolbar } = createEditorWithToolbar()
  const voiceBtn = host.querySelector('button[data-format="insertVoice"]')
  const imageBtn = host.querySelector('button[data-format="insertImage"]')

  toolbar.setResourceButtonsEnabled(false, false)
  assert.equal(voiceBtn.disabled, true)
  assert.equal(imageBtn.disabled, true)

  toolbar.setResourceButtonsEnabled(true, false)
  assert.equal(voiceBtn.disabled, false)
  assert.equal(imageBtn.disabled, true)

  toolbar.setResourceButtonsEnabled(true, true)
  assert.equal(voiceBtn.disabled, false)
  assert.equal(imageBtn.disabled, false)
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 列表/待办区：挂载 / toggle / 互切换丢勾选 / 缩进反缩进 / 激活态同步 / 保存恢复
// ---------------------------------------------------------------------------

function collectTypes(node, acc = new Set()) {
  if (!node || typeof node !== 'object') return acc
  if (node.type) acc.add(node.type)
  if (Array.isArray(node.content)) for (const child of node.content) collectTypes(child, acc)
  return acc
}

function findNode(node, type) {
  if (!node || typeof node !== 'object') return null
  if (node.type === type) return node
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      const found = findNode(child, type)
      if (found) return found
    }
  }
  return null
}

function collectTaskCheckedStates(node, acc = []) {
  if (!node || typeof node !== 'object') return acc
  if (node.type === 'taskItem') acc.push(Boolean(node.attrs?.checked))
  if (Array.isArray(node.content)) {
    for (const child of node.content) collectTaskCheckedStates(child, acc)
  }
  return acc
}

function hasNestedList(node) {
  function walk(n) {
    if (!n || typeof n !== 'object') return false
    if (n.type === 'listItem' || n.type === 'taskItem') {
      if (Array.isArray(n.content)) {
        for (const child of n.content) {
          if (child && ['bulletList', 'orderedList', 'taskList'].includes(child.type)) return true
        }
      }
    }
    if (Array.isArray(n.content)) {
      for (const child of n.content) {
        if (walk(child)) return true
      }
    }
    return false
  }
  return walk(node)
}

function maxListItemDepth(node, depth = 0) {
  if (!node || typeof node !== 'object') return depth
  const currentDepth = node.type === 'listItem' || node.type === 'taskItem' ? depth + 1 : depth
  let maxDepth = currentDepth
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      maxDepth = Math.max(maxDepth, maxListItemDepth(child, currentDepth))
    }
  }
  return maxDepth
}

function maxListItemVisualDepth(node, structuralDepth = 0) {
  if (!node || typeof node !== 'object') return structuralDepth
  const isListItem = node.type === 'listItem' || node.type === 'taskItem'
  const currentStructuralDepth = isListItem ? structuralDepth + 1 : structuralDepth
  const currentVisualDepth = isListItem
    ? currentStructuralDepth + (Number(node.attrs?.dvnIndentLevel) || 0)
    : currentStructuralDepth
  let maxDepth = currentVisualDepth
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      maxDepth = Math.max(maxDepth, maxListItemVisualDepth(child, currentStructuralDepth))
    }
  }
  return maxDepth
}

function listItemDepthForText(node, text, structuralDepth = 0, visualDepth = 0) {
  if (!node || typeof node !== 'object') return null
  const isListItem = node.type === 'listItem' || node.type === 'taskItem'
  const currentStructuralDepth = isListItem ? structuralDepth + 1 : structuralDepth
  const currentVisualDepth = isListItem
    ? Math.min(3, currentStructuralDepth + (Number(node.attrs?.dvnIndentLevel) || 0))
    : visualDepth
  if (node.type === 'text' && node.text === text) return currentVisualDepth
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      const found = listItemDepthForText(child, text, currentStructuralDepth, currentVisualDepth)
      if (found != null) return found
    }
  }
  return null
}

function listTypeForText(node, text, currentListType = null) {
  if (!node || typeof node !== 'object') return null
  const nextListType = ['bulletList', 'orderedList', 'taskList'].includes(node.type)
    ? node.type
    : currentListType
  if (node.type === 'text' && node.text === text) return nextListType
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      const found = listTypeForText(child, text, nextListType)
      if (found != null) return found
    }
  }
  return null
}

function listItemTypeForText(node, text, currentItemType = null) {
  if (!node || typeof node !== 'object') return null
  const nextItemType = node.type === 'listItem' || node.type === 'taskItem'
    ? node.type
    : currentItemType
  if (node.type === 'text' && node.text === text) return nextItemType
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      const found = listItemTypeForText(child, text, nextItemType)
      if (found != null) return found
    }
  }
  return null
}

function textOrder(node, acc = []) {
  if (!node || typeof node !== 'object') return acc
  if (node.type === 'text') acc.push(node.text)
  if (Array.isArray(node.content)) {
    for (const child of node.content) textOrder(child, acc)
  }
  return acc
}

function listItemForListType(listType, text, extra = []) {
  const itemType = listType === 'taskList' ? 'taskItem' : 'listItem'
  const attrs = itemType === 'taskItem' ? { checked: false } : undefined
  return {
    type: itemType,
    ...(attrs ? { attrs } : {}),
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...extra,
    ],
  }
}

function findTextEndPosition(editor, text) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (node.isText && node.text === text) {
      found = pos + node.nodeSize
      return false
    }
    return true
  })
  if (found == null) throw new Error(`text not found: ${text}`)
  return found
}

function findTextOffsetPosition(editor, text, offset) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (node.isText && node.text === text) {
      found = pos + Math.max(0, Math.min(node.text.length, offset))
      return false
    }
    return true
  })
  if (found == null) throw new Error(`text not found: ${text}`)
  return found
}

function findTextRange(editor, text) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (node.isText && node.text === text) {
      found = { from: pos, to: pos + node.nodeSize }
      return false
    }
    return true
  })
  if (found == null) throw new Error(`text not found: ${text}`)
  return found
}

function pressKey(editor, window, key, options = {}) {
  const event = new window.KeyboardEvent('keydown', {
    key,
    bubbles: true,
    cancelable: true,
    ...options,
  })
  editor.view.dom.dispatchEvent(event)
  return event
}

function pressTab(editor, window, shiftKey = false) {
  return pressKey(editor, window, 'Tab', { shiftKey })
}

function pressEnter(editor, window) {
  return pressKey(editor, window, 'Enter')
}

function threeLevelListDoc(itemType = 'listItem', listType = 'bulletList') {
  const attrs = itemType === 'taskItem' ? { checked: false } : undefined
  const item = (text, extra = []) => ({
    type: itemType,
    ...(attrs ? { attrs } : {}),
    content: [
      { type: 'paragraph', content: [{ type: 'text', text }] },
      ...extra,
    ],
  })
  return {
    type: 'doc',
    content: [{
      type: listType,
      content: [
        item('root', [{
          type: listType,
          content: [item('child', [{
            type: listType,
            content: [item('three'), item('four')],
          }])],
        }]),
      ],
    }],
  }
}

test('toolbar mounts list and indent buttons', () => {
  const { editor, host } = createEditorWithToolbar()
  for (const format of ['bulletList', 'orderedList', 'taskList', 'indentList', 'outdentList']) {
    assert.ok(host.querySelector(`button[data-format="${format}"]`), `${format} button should be mounted`)
  }
  editor.destroy()
})

test('task list style keeps checkbox and text on the same row', () => {
  const { editor } = createEditorWithToolbar()
  const style = document.getElementById('dvn-tiptap-tasklist-style')
  assert.ok(style, 'task list style should be injected')
  assert.match(style.textContent, /ul\[data-type="taskList"\] \{ list-style: none; padding-left: 0;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] ul\[data-type="taskList"\] \{ padding-left: 20px;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li > div > ul\[data-type="taskList"\] \{ margin-left: -20px;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li > div > ul\[data-type="taskList"\] > li > div > ul\[data-type="taskList"\] > li > div > ul\[data-type="taskList"\] \{ padding-left: 0;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li\[data-dvn-indent-level="1"\] > div > ul\[data-type="taskList"\] \{ margin-left: -40px;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li\[data-dvn-indent-level="2"\] > div > ul\[data-type="taskList"\] \{ margin-left: -60px;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li \{ display: flex; align-items: flex-start; gap: 0;/)
  assert.equal(/ul\[data-type="taskList"\] li \{ display: flex/.test(style.textContent), false, 'task list row style must not hide markers of nested bullet/ordered lists')
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li > label \{ display: inline-flex; align-items: center; justify-content: center;[^}]*flex: 0 0 20px; width: 20px;[^}]*height: 1\.72em;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li > label > input\[type="checkbox"\] \{ margin: 0;/)
  assert.match(tiptapEditorHtml, /\.ProseMirror ul:not\(\[data-type="taskList"\]\) \{ list-style-type: disc; \}/)
  assert.match(tiptapEditorHtml, /\.ProseMirror li\[data-dvn-indent-level="1"\] \{ margin-left: 20px; \}/)
  assert.match(tiptapEditorHtml, /\.ProseMirror li\[data-dvn-indent-level="2"\] \{ margin-left: 40px; \}/)
  assert.match(tiptapEditorHtml, /li\[data-dvn-indent-level="1"\] > ul,[\s\S]*margin-left: -20px;/)
  assert.match(tiptapEditorHtml, /li\[data-dvn-indent-level="2"\] > ul,[\s\S]*margin-left: -40px;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li > div > p \{ margin: 0;/)
  assert.match(style.textContent, /ul\[data-type="taskList"\] > li\[data-checked="true"\] > div > p:first-child \{ opacity: 0\.55;/)
  assert.equal(style.textContent.includes('text-decoration: line-through'), false, 'checked task text should not render a strikethrough')
  assert.equal(style.textContent.includes('color: var(--color'), false, 'checked task text should not switch to theme color')
  editor.destroy()
})

test('toolbar buttons keep editor selection on pointerdown and mousedown', () => {
  const { editor, host, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [
      { type: 'paragraph', content: [{ type: 'text', text: 'one' }] },
      { type: 'paragraph', content: [{ type: 'text', text: 'two' }] },
      { type: 'paragraph', content: [{ type: 'text', text: 'three' }] },
    ],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection({ from: 1, to: 15 }).run()

  const button = host.querySelector('button[data-format="bulletList"]')
  const pointerEvent = new window.PointerEvent('pointerdown', { bubbles: true, cancelable: true })
  button.dispatchEvent(pointerEvent)
  assert.equal(pointerEvent.defaultPrevented, true)

  const mouseEvent = new window.MouseEvent('mousedown', { bubbles: true, cancelable: true })
  button.dispatchEvent(mouseEvent)
  assert.equal(mouseEvent.defaultPrevented, true)

  assert.deepEqual(
    { from: editor.state.selection.from, to: editor.state.selection.to },
    { from: 1, to: 15 },
    'toolbar pointer/mouse down must not collapse the editor selection',
  )
  button.click()

  const list = findNode(editor.getJSON(), 'bulletList')
  assert.equal(list.content.length, 3)
  assert.deepEqual(list.content.map(item => item.content[0].content[0].text), ['one', 'two', 'three'])
  editor.destroy()
})

for (const format of ['bulletList', 'orderedList', 'taskList']) {
  test(`${format}: toggle creates and clears list`, () => {
    const { editor, host } = createEditorWithToolbar()
    insertText(editor, 'item')
    selectText(editor)

    const btn = host.querySelector(`button[data-format="${format}"]`)
    btn.click()
    assert.ok(editor.isActive(format), `${format} should be active after toggle on`)

    btn.click()
    assert.ok(!editor.isActive(format), `${format} should be cleared after toggle off`)
    editor.destroy()
  })
}

test('switching task list to bullet list drops checked, back to task list defaults unchecked', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [{
        type: 'taskItem',
        attrs: { checked: true },
        content: [{ type: 'paragraph', content: [{ type: 'text', text: 'done' }] }],
      }],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(editor.state.doc.content.size).run()

  // 切换为无序列表：checked 属 taskItem 专有属性，转 listItem 时丢弃
  host.querySelector('button[data-format="bulletList"]').click()
  const afterBullet = collectTypes(editor.getJSON())
  assert.ok(afterBullet.has('bulletList'), 'should contain bulletList after switch')
  assert.ok(!afterBullet.has('taskItem'), 'taskItem must be gone after switching to bullet list')

  // 切回待办：默认未勾选
  host.querySelector('button[data-format="taskList"]').click()
  const afterTask = collectTypes(editor.getJSON())
  assert.ok(afterTask.has('taskItem'), 'taskItem should reappear after switching back')
  const taskItem = findNode(editor.getJSON(), 'taskItem')
  assert.equal(taskItem.attrs?.checked, false, 'checked should default to false after switching back')
  editor.destroy()
})


for (const parentListType of ['bulletList', 'orderedList', 'taskList']) {
  for (const targetListType of ['bulletList', 'orderedList', 'taskList']) {
    if (parentListType === targetListType) continue

    test(`nested ${parentListType} can be switched to nested ${targetListType} without leaving parent list`, () => {
      const { editor, host } = createEditorWithToolbar()
      editor.commands.setContent({
        type: 'doc',
        content: [{
          type: parentListType,
          content: [listItemForListType(parentListType, 'root', [{
            type: parentListType,
            content: [listItemForListType(parentListType, 'child')],
          }])],
        }],
      })
      markEditorFocused(editor)
      editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'child')).run()

      host.querySelector(`button[data-format="${targetListType}"]`).click()
      const json = editor.getJSON()
      assert.equal(listTypeForText(json, 'root'), parentListType, 'parent list type must be preserved')
      assert.equal(listTypeForText(json, 'child'), targetListType, 'nested list type should switch in place')
      assert.equal(listItemDepthForText(json, 'root'), 1)
      assert.equal(listItemDepthForText(json, 'child'), 2)
      assert.equal(
        listItemTypeForText(json, 'child'),
        targetListType === 'taskList' ? 'taskItem' : 'listItem',
        'nested list item node should match the target list type',
      )
      assert.ok(maxListItemVisualDepth(json) <= 3, 'mixed nesting must still stay within three visual levels')
      editor.destroy()
    })
  }
}


test('switching list type changes only the active sibling item', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [
        listItemForListType('taskList', 'first'),
        listItemForListType('taskList', 'second'),
      ],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'second')).run()

  host.querySelector('button[data-format="bulletList"]').click()
  const json = editor.getJSON()
  assert.equal(listTypeForText(json, 'first'), 'taskList', 'previous sibling must stay task list')
  assert.equal(listTypeForText(json, 'second'), 'bulletList', 'active sibling should switch to bullet list')
  assert.equal(listItemTypeForText(json, 'first'), 'taskItem')
  assert.equal(listItemTypeForText(json, 'second'), 'listItem')
  assert.equal(listItemDepthForText(json, 'first'), 1)
  assert.equal(listItemDepthForText(json, 'second'), 1)
  editor.destroy()
})


test('switching current list item type preserves caret position', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [
        listItemForListType('taskList', 'first'),
        listItemForListType('taskList', 'second'),
      ],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextOffsetPosition(editor, 'second', 3)).run()

  host.querySelector('button[data-format="bulletList"]').click()
  editor.commands.insertContent('X')
  const json = editor.getJSON()
  assert.equal(listTypeForText(json, 'secXond'), 'bulletList')
  assert.equal(listTypeForText(json, 'first'), 'taskList')
  assert.match(editor.getText(), /first\s+secXond/, 'text should be inserted at the original caret offset')
  editor.destroy()
})

test('Shift+Tab after switching nested task item to bullet keeps it as a first-level bullet item', () => {
  const { editor, host, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [listItemForListType('taskList', 'DCSad', [{
        type: 'taskList',
        content: [listItemForListType('taskList', '打')],
      }])],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, '打')).run()
  host.querySelector('button[data-format="bulletList"]').click()
  assert.equal(listTypeForText(editor.getJSON(), '打'), 'bulletList', 'child should become a nested bullet list item')
  assert.equal(listItemDepthForText(editor.getJSON(), '打'), 2)

  editor.chain().focus().setTextSelection(findTextEndPosition(editor, '打')).run()
  pressTab(editor, window, true)
  const json = editor.getJSON()
  assert.equal(listTypeForText(json, 'DCSad'), 'taskList', 'parent should remain a task item')
  assert.equal(listTypeForText(json, '打'), 'bulletList', 'outdented child should remain a bullet list item')
  assert.equal(listItemDepthForText(json, 'DCSad'), 1)
  assert.equal(listItemDepthForText(json, '打'), 1, 'outdented child should become first-level list item instead of plain text')
  editor.destroy()
})

for (const parentListType of ['bulletList', 'orderedList', 'taskList']) {
  for (const targetListType of ['bulletList', 'orderedList', 'taskList']) {
    if (parentListType === targetListType) continue

    test(`Shift+Tab after switching nested ${parentListType} item to ${targetListType} keeps the target list type`, () => {
      const { editor, host, window } = createEditorWithToolbar()
      editor.commands.setContent({
        type: 'doc',
        content: [{
          type: parentListType,
          content: [listItemForListType(parentListType, 'parent', [{
            type: parentListType,
            content: [listItemForListType(parentListType, 'child')],
          }])],
        }],
      })
      markEditorFocused(editor)
      editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'child')).run()
      host.querySelector(`button[data-format="${targetListType}"]`).click()
      assert.equal(listTypeForText(editor.getJSON(), 'child'), targetListType, 'current child should switch in place before outdent')
      assert.equal(listItemDepthForText(editor.getJSON(), 'child'), 2)

      editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'child')).run()
      pressTab(editor, window, true)
      const json = editor.getJSON()
      assert.deepEqual(textOrder(json), ['parent', 'child'], 'outdent must preserve document order')
      assert.equal(listTypeForText(json, 'parent'), parentListType, 'parent should keep its original list type')
      assert.equal(listTypeForText(json, 'child'), targetListType, 'outdented child should keep the switched list type')
      assert.equal(listItemDepthForText(json, 'parent'), 1)
      assert.equal(listItemDepthForText(json, 'child'), 1, 'outdented child should be a first-level list item, not plain text')
      assert.equal(collectTypes(json).has(targetListType === 'taskList' ? 'taskItem' : 'listItem'), true)
      editor.destroy()
    })
  }
}

test('Shift+Tab in a mixed nested list preserves order and keeps following siblings under the lifted item', () => {
  const { editor, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [
        listItemForListType('taskList', 'root', [{
          type: 'bulletList',
          content: [
            listItemForListType('bulletList', 'before'),
            listItemForListType('bulletList', 'active'),
            listItemForListType('bulletList', 'after'),
          ],
        }]),
        listItemForListType('taskList', 'next root'),
      ],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'active')).run()
  pressTab(editor, window, true)
  const json = editor.getJSON()

  assert.deepEqual(textOrder(json), ['root', 'before', 'active', 'after', 'next root'], 'outdent must not reorder the active item after its following sibling')
  assert.equal(listTypeForText(json, 'root'), 'taskList')
  assert.equal(listTypeForText(json, 'before'), 'bulletList')
  assert.equal(listTypeForText(json, 'active'), 'bulletList')
  assert.equal(listTypeForText(json, 'after'), 'bulletList')
  assert.equal(listTypeForText(json, 'next root'), 'taskList')
  assert.equal(listItemDepthForText(json, 'root'), 1)
  assert.equal(listItemDepthForText(json, 'before'), 2)
  assert.equal(listItemDepthForText(json, 'active'), 1)
  assert.equal(listItemDepthForText(json, 'after'), 2, 'following sibling should remain one level below the lifted active item')
  assert.equal(listItemDepthForText(json, 'next root'), 1)
  editor.destroy()
})

test('switching current list item type preserves Chinese caret position', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [
        listItemForListType('taskList', '第一行'),
        listItemForListType('taskList', '你好世界'),
      ],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextOffsetPosition(editor, '你好世界', 2)).run()

  host.querySelector('button[data-format="orderedList"]').click()
  editor.commands.insertContent('X')
  const json = editor.getJSON()
  assert.equal(listTypeForText(json, '你好X世界'), 'orderedList')
  assert.equal(listTypeForText(json, '第一行'), 'taskList')
  assert.match(editor.getText(), /第一行\s+你好X世界/, 'Chinese text should be inserted at the original caret offset')
  editor.destroy()
})

test('indent/outdent buttons nest and unnest bullet list items', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'bulletList',
      content: [
        { type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'one' }] }] },
        { type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'two' }] }] },
      ],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(editor.state.doc.content.size).run()

  host.querySelector('button[data-format="indentList"]').click()
  assert.ok(hasNestedList(editor.getJSON()), 'second item should nest under first after indent')

  host.querySelector('button[data-format="outdentList"]').click()
  assert.ok(!hasNestedList(editor.getJSON()), 'no nested list should remain after outdent')
  editor.destroy()
})

for (const { name, listType, itemType, attrs } of [
  { name: 'bullet list', listType: 'bulletList', itemType: 'listItem' },
  { name: 'ordered list', listType: 'orderedList', itemType: 'listItem' },
  { name: 'task list', listType: 'taskList', itemType: 'taskItem', attrs: { checked: false } },
]) {
  test(`indent button follows real sink capability for ${name}`, () => {
    const { editor, host } = createEditorWithToolbar()
    const item = (text) => ({
      type: itemType,
      ...(attrs ? { attrs } : {}),
      content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
    })
    editor.commands.setContent({
      type: 'doc',
      content: [{
        type: listType,
        content: [item('one'), item('two')],
      }],
    })
    markEditorFocused(editor)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'one')).run()
    const indent = host.querySelector('button[data-format="indentList"]')
    assert.equal(indent.disabled, true, `${name} first item cannot be indented`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'two')).run()
    assert.equal(indent.disabled, false, `${name} item with a previous sibling can be indented`)
    editor.destroy()
  })
}

for (const { name, listType, itemType, attrs } of [
  { name: 'bullet list', listType: 'bulletList', itemType: 'listItem' },
  { name: 'ordered list', listType: 'orderedList', itemType: 'listItem' },
  { name: 'task list', listType: 'taskList', itemType: 'taskItem', attrs: { checked: false } },
]) {
  test(`the second ${name} item can be indented twice with Tab`, () => {
    const { editor, window } = createEditorWithToolbar()
    const item = (text) => ({
      type: itemType,
      ...(attrs ? { attrs } : {}),
      content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
    })
    editor.commands.setContent({
      type: 'doc',
      content: [{
        type: listType,
        content: [item('one'), item('two')],
      }],
    })
    markEditorFocused(editor)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'two')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'two'), 2, `${name} second item should become second level`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'two')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'two'), 3, `${name} second item should become third level after another Tab`)

    editor.commands.insertContent(' typed')
    assert.equal(editor.getText().includes('two typed'), true, `${name} item should still accept text after second Tab`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'two typed')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'two typed'), 3, `${name} second item should remain capped at third level`)
    editor.destroy()
  })
}

for (const { name, listType, itemType, attrs } of [
  { name: 'bullet list', listType: 'bulletList', itemType: 'listItem' },
  { name: 'ordered list', listType: 'orderedList', itemType: 'listItem' },
  { name: 'task list', listType: 'taskList', itemType: 'taskItem', attrs: { checked: false } },
]) {
  test(`Enter keeps native sibling insertion and Tab still caps ${name} at three levels`, () => {
    const { editor, window } = createEditorWithToolbar()
    const item = (text) => ({
      type: itemType,
      ...(attrs ? { attrs } : {}),
      content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
    })
    editor.commands.setContent({
      type: 'doc',
      content: [{
        type: listType,
        content: [item('one'), item('two')],
      }],
    })
    markEditorFocused(editor)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'two')).run()
    pressTab(editor, window)
    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'two')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'two'), 3)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'one')).run()
    pressEnter(editor, window)
    editor.commands.insertContent('new')
    assert.match(editor.getText(), /one\s+new\s+two/, `${name} new item should stay directly after the first item`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'new')).run()
    pressTab(editor, window)
    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'new')).run()
    pressTab(editor, window)

    assert.equal(listItemDepthForText(editor.getJSON(), 'new'), 3, `${name} new item should reach third level`)
    assert.ok(maxListItemVisualDepth(editor.getJSON()) <= 3, `${name} must never render deeper than third level`)
    editor.destroy()
  })
}

for (const { name, listType, itemType, attrs } of [
  { name: 'bullet list', listType: 'bulletList', itemType: 'listItem' },
  { name: 'ordered list', listType: 'orderedList', itemType: 'listItem' },
  { name: 'task list', listType: 'taskList', itemType: 'taskItem', attrs: { checked: false } },
]) {
  test(`Shift+Tab outdents only the active item in a four-item third-level ${name}`, () => {
    const { editor, window } = createEditorWithToolbar()
    const item = (text, extra = []) => ({
      type: itemType,
      ...(attrs ? { attrs } : {}),
      content: [
        { type: 'paragraph', content: [{ type: 'text', text }] },
        ...extra,
      ],
    })
    editor.commands.setContent({
      type: 'doc',
      content: [{
        type: listType,
        content: [item('one', [{
          type: listType,
          content: [item('two', [{
            type: listType,
            content: [item('a'), item('b'), item('c'), item('d')],
          }])],
        }])],
      }],
    })
    markEditorFocused(editor)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'c')).run()
    pressTab(editor, window, true)
    assert.equal(listItemDepthForText(editor.getJSON(), 'a'), 3)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 3)
    assert.equal(listItemDepthForText(editor.getJSON(), 'c'), 2)
    assert.equal(listItemDepthForText(editor.getJSON(), 'd'), 3, `${name} following sibling should stay third level after one outdent`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'c')).run()
    pressTab(editor, window, true)
    assert.equal(listItemDepthForText(editor.getJSON(), 'c'), 1)
    assert.equal(listItemDepthForText(editor.getJSON(), 'd'), 3, `${name} following sibling should not be pulled to second level`)
    assert.ok(maxListItemVisualDepth(editor.getJSON()) <= 3, `${name} outdent should keep visual depth capped`)
    editor.destroy()
  })
}

for (const { name, listType, itemType, attrs } of [
  { name: 'bullet list', listType: 'bulletList', itemType: 'listItem' },
  { name: 'ordered list', listType: 'orderedList', itemType: 'listItem' },
  { name: 'task list', listType: 'taskList', itemType: 'taskItem', attrs: { checked: false } },
]) {
  test(`level-one ${name} item never exits the list via Shift+Tab`, () => {
    const { editor, window } = createEditorWithToolbar()
    const item = (text, extra = []) => ({
      type: itemType,
      ...(attrs ? { attrs } : {}),
      content: [
        { type: 'paragraph', content: [{ type: 'text', text }] },
        ...extra,
      ],
    })
    editor.commands.setContent({
      type: 'doc',
      content: [{
        type: listType,
        content: [item('one', [{
          type: listType,
          content: [item('two', [{
            type: listType,
            content: [item('a'), item('b')],
          }])],
        }])],
      }],
    })
    markEditorFocused(editor)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'a')).run()
    pressTab(editor, window, true)
    assert.equal(listItemDepthForText(editor.getJSON(), 'a'), 2, `${name} active item should move from third to second level`)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 3, `${name} following item should stay third level after first outdent`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'a')).run()
    pressTab(editor, window, true)
    assert.equal(listItemDepthForText(editor.getJSON(), 'a'), 1, `${name} active item should move from second to first level`)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 3, `${name} following item should stay third level after second outdent`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'a')).run()
    pressTab(editor, window, true)
    assert.equal(listItemDepthForText(editor.getJSON(), 'a'), 1, `${name} active item should stay first level on top-level outdent`)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 3, `${name} following item should stay third level after ignored top-level outdent`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'b')).run()
    pressTab(editor, window, true)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 2, `${name} preserved item should still support Shift+Tab`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'b')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 3, `${name} preserved item should support Tab back to third level`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'b')).run()
    pressTab(editor, window, true)
    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'b')).run()
    pressTab(editor, window, true)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 1, `${name} preserved item should support Shift+Tab back to first level`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'b')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 2, `${name} preserved first-level item should support Tab again`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'b')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'b'), 3, `${name} preserved item should support two Tabs back to third level`)
    assert.ok(maxListItemVisualDepth(editor.getJSON()) <= 3, `${name} preserved item must stay capped at third level`)
    editor.destroy()
  })
}

for (const { name, listType, itemType, attrs } of [
  { name: 'bullet list', listType: 'bulletList', itemType: 'listItem' },
  { name: 'ordered list', listType: 'orderedList', itemType: 'listItem' },
  { name: 'task list', listType: 'taskList', itemType: 'taskItem', attrs: { checked: false } },
]) {
  test(`Tab creates up to three nesting levels for ${name}`, () => {
    const { editor, window } = createEditorWithToolbar()
    const item = (text) => ({
      type: itemType,
      ...(attrs ? { attrs } : {}),
      content: [{ type: 'paragraph', content: [{ type: 'text', text }] }],
    })
    editor.commands.setContent({
      type: 'doc',
      content: [{
        type: listType,
        content: [item('one'), item('two'), item('three')],
      }],
    })
    markEditorFocused(editor)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'two')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'two'), 2, `${name} Tab should create second level`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'three')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'three'), 2, `${name} first Tab on third item should keep second level`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'three')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'three'), 3, `${name} second Tab on third item should create third level`)

    editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'three')).run()
    pressTab(editor, window)
    assert.equal(listItemDepthForText(editor.getJSON(), 'three'), 3, `${name} Tab must not create fourth level`)
    editor.destroy()
  })
}

test('indent/outdent buttons nest and unnest task list items', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [
        { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'a' }] }] },
        { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'b' }] }] },
      ],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(editor.state.doc.content.size).run()

  host.querySelector('button[data-format="indentList"]').click()
  assert.ok(hasNestedList(editor.getJSON()), 'task item should nest after indent')

  host.querySelector('button[data-format="outdentList"]').click()
  assert.ok(!hasNestedList(editor.getJSON()), 'no nested task list should remain after outdent')
  editor.destroy()
})

test('list indentation is capped at three levels for toolbar and Tab', () => {
  const { editor, host, window } = createEditorWithToolbar()
  editor.commands.setContent(threeLevelListDoc('listItem', 'bulletList'))
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'four')).run()

  const indent = host.querySelector('button[data-format="indentList"]')
  assert.equal(maxListItemDepth(editor.getJSON()), 3)
  assert.equal(indent.disabled, true, 'indent disabled at the third list level')

  indent.click()
  assert.equal(maxListItemDepth(editor.getJSON()), 3, 'toolbar indent must not create fourth level')

  pressTab(editor, window)
  assert.equal(maxListItemDepth(editor.getJSON()), 3, 'Tab must not create fourth level')
  editor.destroy()
})

test('ordered list indentation is capped at three levels', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent(threeLevelListDoc('listItem', 'orderedList'))
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'four')).run()

  const indent = host.querySelector('button[data-format="indentList"]')
  assert.equal(maxListItemDepth(editor.getJSON()), 3)
  assert.equal(indent.disabled, true, 'indent disabled at the third ordered list level')

  indent.click()
  assert.equal(maxListItemDepth(editor.getJSON()), 3, 'ordered list indent must not create fourth level')
  editor.destroy()
})

test('task list indentation is capped at three levels', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent(threeLevelListDoc('taskItem', 'taskList'))
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'four')).run()

  const indent = host.querySelector('button[data-format="indentList"]')
  assert.equal(maxListItemDepth(editor.getJSON()), 3)
  assert.equal(indent.disabled, true, 'indent disabled at the third task level')

  indent.click()
  assert.equal(maxListItemDepth(editor.getJSON()), 3, 'task indent must not create fourth level')
  editor.destroy()
})

test('task parent checkbox cascades checked state to nested children', () => {
  const { editor, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [{
        type: 'taskItem',
        attrs: { checked: false },
        content: [
          { type: 'paragraph', content: [{ type: 'text', text: 'parent' }] },
          {
            type: 'taskList',
            content: [
              { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'child one' }] }] },
              { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'child two' }] }] },
            ],
          },
        ],
      }],
    }],
  })

  const parentCheckbox = editor.view.dom.querySelector('input[type="checkbox"]')
  parentCheckbox.checked = true
  parentCheckbox.dispatchEvent(new window.Event('change', { bubbles: true }))

  const afterChecked = collectTaskCheckedStates(editor.getJSON())
  assert.deepEqual(afterChecked, [true, true, true], 'checking parent should check all nested children')

  parentCheckbox.checked = false
  parentCheckbox.dispatchEvent(new window.Event('change', { bubbles: true }))

  const afterUnchecked = collectTaskCheckedStates(editor.getJSON())
  assert.deepEqual(afterUnchecked, [false, false, false], 'unchecking parent should uncheck all nested children')
  editor.destroy()
})

test('task children update parent only when all children are checked', () => {
  const { editor, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [{
        type: 'taskItem',
        attrs: { checked: false },
        content: [
          { type: 'paragraph', content: [{ type: 'text', text: 'parent' }] },
          {
            type: 'taskList',
            content: [
              { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'child one' }] }] },
              { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'child two' }] }] },
            ],
          },
        ],
      }],
    }],
  })

  const checkboxes = editor.view.dom.querySelectorAll('input[type="checkbox"]')
  checkboxes[1].checked = true
  checkboxes[1].dispatchEvent(new window.Event('change', { bubbles: true }))

  assert.deepEqual(collectTaskCheckedStates(editor.getJSON()), [false, true, false], 'partial children should keep parent unchecked')
  assert.equal(checkboxes[0].indeterminate, false, 'parent should not render an indeterminate state')
  assert.equal(checkboxes[0].closest('li').dataset.indeterminate, undefined, 'parent should not expose mixed DOM state')

  checkboxes[2].checked = true
  checkboxes[2].dispatchEvent(new window.Event('change', { bubbles: true }))

  assert.deepEqual(collectTaskCheckedStates(editor.getJSON()), [true, true, true], 'all checked children should check parent automatically')
  assert.equal(checkboxes[0].indeterminate, false, 'full checked parent should not use indeterminate state')
  assert.equal(checkboxes[0].closest('li').dataset.indeterminate, undefined, 'full checked parent should not expose mixed DOM state')
  editor.destroy()
})

test('visual-indented task children update their Shift+Tab-created parent when all are checked', () => {
  const { editor, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [{
        type: 'taskItem',
        attrs: { checked: false, dvnIndentLevel: 0 },
        content: [
          { type: 'paragraph', content: [{ type: 'text', text: 'top' }] },
          {
            type: 'taskList',
            content: [
              { type: 'taskItem', attrs: { checked: false, dvnIndentLevel: 0 }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'parent' }] }] },
              { type: 'taskItem', attrs: { checked: false, dvnIndentLevel: 1 }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'child one' }] }] },
              { type: 'taskItem', attrs: { checked: false, dvnIndentLevel: 1 }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'child two' }] }] },
              { type: 'taskItem', attrs: { checked: false, dvnIndentLevel: 0 }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'sibling' }] }] },
            ],
          },
        ],
      }],
    }],
  })

  let checkboxes = editor.view.dom.querySelectorAll('input[type="checkbox"]')
  checkboxes[2].checked = true
  checkboxes[2].dispatchEvent(new window.Event('change', { bubbles: true }))
  assert.deepEqual(
    collectTaskCheckedStates(editor.getJSON()),
    [false, false, true, false, false],
    'checking only one visual-indented child should keep the visual parent unchecked',
  )

  checkboxes = editor.view.dom.querySelectorAll('input[type="checkbox"]')
  checkboxes[3].checked = true
  checkboxes[3].dispatchEvent(new window.Event('change', { bubbles: true }))
  assert.deepEqual(
    collectTaskCheckedStates(editor.getJSON()),
    [false, true, true, true, false],
    'checking every visual-indented child should check the Shift+Tab-created visual parent only',
  )
  editor.destroy()
})

test('user-created third-level task siblings cascade to the second-level parent after Shift+Tab', () => {
  const { editor, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [
        { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'top' }] }] },
        { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'parent' }] }] },
      ],
    }],
  })
  markEditorFocused(editor)

  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'parent')).run()
  pressTab(editor, window)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'parent')).run()
  pressTab(editor, window)
  assert.equal(listItemDepthForText(editor.getJSON(), 'parent'), 3)

  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'parent')).run()
  pressEnter(editor, window)
  editor.commands.insertContent('child one')
  assert.equal(listItemDepthForText(editor.getJSON(), 'child one'), 3)

  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'parent')).run()
  pressTab(editor, window, true)
  assert.equal(listItemDepthForText(editor.getJSON(), 'parent'), 2, 'Shift+Tab should make the previous third-level task a second-level visual parent')
  assert.equal(listItemDepthForText(editor.getJSON(), 'child one'), 3, 'existing following task should remain third-level under the new visual parent')

  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'child one')).run()
  pressEnter(editor, window)
  editor.commands.insertContent('child two')
  assert.equal(listItemDepthForText(editor.getJSON(), 'child two'), 3)

  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'child two')).run()
  pressEnter(editor, window)
  editor.commands.insertContent('sibling')
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'sibling')).run()
  pressTab(editor, window, true)
  assert.equal(listItemDepthForText(editor.getJSON(), 'sibling'), 2, 'following second-level sibling keeps the top task unchecked')

  let checkboxes = editor.view.dom.querySelectorAll('input[type="checkbox"]')
  checkboxes[2].checked = true
  checkboxes[2].dispatchEvent(new window.Event('change', { bubbles: true }))
  checkboxes = editor.view.dom.querySelectorAll('input[type="checkbox"]')
  checkboxes[3].checked = true
  checkboxes[3].dispatchEvent(new window.Event('change', { bubbles: true }))

  assert.deepEqual(
    collectTaskCheckedStates(editor.getJSON()),
    [false, true, true, true, false],
    'the second-level parent should be checked after all of its generated third-level children are checked',
  )
  editor.destroy()
})

test('task completion bubbles upward only after every child level is complete', () => {
  const { editor, window } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'taskList',
      content: [{
        type: 'taskItem',
        attrs: { checked: false },
        content: [
          { type: 'paragraph', content: [{ type: 'text', text: 'parent' }] },
          {
            type: 'taskList',
            content: [{
              type: 'taskItem',
              attrs: { checked: false },
              content: [
                { type: 'paragraph', content: [{ type: 'text', text: 'child' }] },
                {
                  type: 'taskList',
                  content: [
                    { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'grandchild one' }] }] },
                    { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'grandchild two' }] }] },
                  ],
                },
              ],
            }],
          },
        ],
      }],
    }],
  })

  const checkboxes = editor.view.dom.querySelectorAll('input[type="checkbox"]')
  checkboxes[2].checked = true
  checkboxes[2].dispatchEvent(new window.Event('change', { bubbles: true }))
  assert.deepEqual(collectTaskCheckedStates(editor.getJSON()), [false, false, true, false], 'one completed grandchild should not complete child or parent')

  checkboxes[3].checked = true
  checkboxes[3].dispatchEvent(new window.Event('change', { bubbles: true }))
  assert.deepEqual(collectTaskCheckedStates(editor.getJSON()), [true, true, true, true], 'parent should complete only after the whole child subtree is complete')
  editor.destroy()
})

test('indent button follows real sink capability inside lists', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'plain')
  selectText(editor)

  const indent = host.querySelector('button[data-format="indentList"]')
  const outdent = host.querySelector('button[data-format="outdentList"]')
  assert.equal(indent.disabled, true, 'indent disabled in paragraph')
  assert.equal(outdent.disabled, true, 'outdent disabled in paragraph')

  host.querySelector('button[data-format="bulletList"]').click()
  assert.equal(indent.disabled, true, 'single list item has no previous sibling to indent under')
  assert.equal(outdent.disabled, true, 'outdent disabled at first list level because Shift+Tab no longer exits lists')
  editor.destroy()
})

test('list toggle buttons are not active before editor receives focus', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      content: [{ type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'x' }] }] }],
    }],
  })

  assert.equal(host.querySelector('button[data-format="bulletList"]').getAttribute('aria-pressed'), 'false')
  assert.equal(host.querySelector('button[data-format="orderedList"]').getAttribute('aria-pressed'), 'false')
  assert.equal(host.querySelector('button[data-format="taskList"]').getAttribute('aria-pressed'), 'false')
  editor.destroy()
})

test('list toggle buttons are not active for a collapsed cursor inside a list', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'orderedList',
      content: [{ type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'x' }] }] }],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextEndPosition(editor, 'x')).run()

  assert.equal(host.querySelector('button[data-format="bulletList"]').getAttribute('aria-pressed'), 'false')
  assert.equal(host.querySelector('button[data-format="orderedList"]').getAttribute('aria-pressed'), 'false')
  assert.equal(host.querySelector('button[data-format="taskList"]').getAttribute('aria-pressed'), 'false')
  editor.destroy()
})

test('list toggle buttons reflect active state as selection moves', () => {
  const { editor, host } = createEditorWithToolbar()
  editor.commands.setContent({
    type: 'doc',
    content: [{
      type: 'bulletList',
      content: [{ type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'x' }] }] }],
    }],
  })
  markEditorFocused(editor)
  editor.chain().focus().setTextSelection(findTextRange(editor, 'x')).run()

  assert.equal(host.querySelector('button[data-format="bulletList"]').getAttribute('aria-pressed'), 'true')
  assert.equal(host.querySelector('button[data-format="orderedList"]').getAttribute('aria-pressed'), 'false')
  assert.equal(host.querySelector('button[data-format="taskList"]').getAttribute('aria-pressed'), 'false')
  editor.destroy()
})

test('save and reload preserves list type, nesting, start and checked', () => {
  const { editor } = createEditorWithToolbar()
  const doc = {
    type: 'doc',
    content: [
      {
        type: 'bulletList',
        content: [
          {
            type: 'listItem',
            content: [
              { type: 'paragraph', content: [{ type: 'text', text: 'top' }] },
              {
                type: 'orderedList',
                attrs: { start: 3 },
                content: [{ type: 'listItem', content: [{ type: 'paragraph', content: [{ type: 'text', text: 'nested' }] }] }],
              },
            ],
          },
        ],
      },
      {
        type: 'taskList',
        content: [
          { type: 'taskItem', attrs: { checked: true }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'done' }] }] },
          { type: 'taskItem', attrs: { checked: false }, content: [{ type: 'paragraph', content: [{ type: 'text', text: 'todo' }] }] },
        ],
      },
    ],
  }
  editor.commands.setContent(doc)
  const json1 = editor.getJSON()

  const result = validateEnvelope(createEnvelope(json1))
  assert.equal(result.ok, true, JSON.stringify(result.errors, null, 2))

  const { editor: editor2 } = createEditorWithToolbar()
  editor2.commands.setContent(json1)
  const json2 = editor2.getJSON()
  assert.deepEqual(json2, json1, 'reload should preserve type/level/start/checked')
  editor.destroy()
  editor2.destroy()
})
