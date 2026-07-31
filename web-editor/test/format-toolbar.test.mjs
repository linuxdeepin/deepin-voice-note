// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 格式工具栏单测（happy-dom）：十项格式 apply/clear/激活态、setFontList 填充、常驻可见。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { createFormatToolbar } from '../src/runtime/format-toolbar.js'
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
  return { editor, host, toolbar, window }
}

function insertText(editor, text) {
  editor.commands.insertContent({ type: 'text', text })
}

// 选中首个段落内的全部文本（跳过文档边界位置 0），对 mark 与块级 toggle 均可用
function selectText(editor) {
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

// ---------------------------------------------------------------------------
// 文字颜色面板
// ---------------------------------------------------------------------------

test('foreColor panel applies and clears text color', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'colored')
  selectText(editor)

  const panel = host.querySelector('[data-panel="foreColor"]')
  assert.ok(panel)
  const cell = panel.querySelector('button[data-color="#e50000"]')
  assert.ok(cell)

  cell.click()
  assert.ok(editor.isActive('color', { color: '#e50000' }))

  const clear = panel.querySelector('button[data-action="clear-foreColor"]')
  clear.click()
  assert.ok(!editor.isActive('color'))
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 背景色面板
// ---------------------------------------------------------------------------

test('backColor panel applies and clears highlight', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'highlighted')
  selectText(editor)

  const panel = host.querySelector('[data-panel="backColor"]')
  assert.ok(panel)
  const cell = panel.querySelector('button[data-color="#fff2cc"]')
  // fallback to first non-transparent cell if exact value absent
  const target = cell || [...panel.querySelectorAll('button[data-color]')].find(c => c.getAttribute('data-color') !== 'transparent')
  assert.ok(target)

  const color = target.getAttribute('data-color')
  target.click()
  assert.ok(editor.isActive('highlight', { color }))

  const clear = panel.querySelector('button[data-action="clear-backColor"]')
  clear.click()
  assert.ok(!editor.isActive('highlight'))
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

// ---------------------------------------------------------------------------
// setFontList 填充
// ---------------------------------------------------------------------------

test('setFontList populates the font family dropdown', () => {
  const { editor, host, toolbar } = createEditorWithToolbar()
  const select = host.querySelector('select[data-control="fontFamily"]')

  toolbar.setFontList(['Arial', 'Helvetica', 'Noto Sans CJK SC'], 'Helvetica')
  const options = select.querySelectorAll('option')
  assert.equal(options.length, 4, 'should have default option + 3 fonts')
  assert.equal(options[1].value, 'Arial')
  assert.equal(options[2].value, 'Helvetica')
  assert.equal(options[2].selected, true, 'default font should be selected')
  assert.equal(options[3].value, 'Noto Sans CJK SC')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 激活态随光标同步
// ---------------------------------------------------------------------------

test('active state syncs when selection moves into formatted text', () => {
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

  // 光标进入加粗文本：按钮高亮
  editor.chain().focus().setTextSelection(3).run()
  assert.equal(btn.getAttribute('aria-pressed'), 'true')

  // 光标进入普通文本：按钮取消高亮
  editor.chain().focus().setTextSelection(7).run()
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

test('toolbar mounts list and indent buttons', () => {
  const { editor, host } = createEditorWithToolbar()
  for (const format of ['bulletList', 'orderedList', 'taskList', 'indentList', 'outdentList']) {
    assert.ok(host.querySelector(`button[data-format="${format}"]`), `${format} button should be mounted`)
  }
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
  editor.chain().focus().setTextSelection(editor.state.doc.content.size).run()

  host.querySelector('button[data-format="indentList"]').click()
  assert.ok(hasNestedList(editor.getJSON()), 'second item should nest under first after indent')

  host.querySelector('button[data-format="outdentList"]').click()
  assert.ok(!hasNestedList(editor.getJSON()), 'no nested list should remain after outdent')
  editor.destroy()
})

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
  editor.chain().focus().setTextSelection(editor.state.doc.content.size).run()

  host.querySelector('button[data-format="indentList"]').click()
  assert.ok(hasNestedList(editor.getJSON()), 'task item should nest after indent')

  host.querySelector('button[data-format="outdentList"]').click()
  assert.ok(!hasNestedList(editor.getJSON()), 'no nested task list should remain after outdent')
  editor.destroy()
})

test('indent/outdent buttons disabled outside list, enabled inside', () => {
  const { editor, host } = createEditorWithToolbar()
  insertText(editor, 'plain')
  selectText(editor)

  const indent = host.querySelector('button[data-format="indentList"]')
  const outdent = host.querySelector('button[data-format="outdentList"]')
  assert.equal(indent.disabled, true, 'indent disabled in paragraph')
  assert.equal(outdent.disabled, true, 'outdent disabled in paragraph')

  host.querySelector('button[data-format="bulletList"]').click()
  assert.equal(indent.disabled, false, 'indent enabled inside list')
  assert.equal(outdent.disabled, false, 'outdent enabled inside list')
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
  editor.chain().focus().setTextSelection(editor.state.doc.content.size).run()

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
