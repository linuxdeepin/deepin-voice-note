// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { FORE_COLORS, BACK_COLORS, FONT_SIZES, toPxSize } from './format-palette.js'
import { canIndentActiveListItem, canOutdentActiveListItem, liftActiveListItem, sinkActiveListItem } from './list-behavior.js'

const TOGGLE_BUTTONS = [
  { format: 'bold', label: 'B', title: '粗体' },
  { format: 'italic', label: 'I', title: '斜体' },
  { format: 'underline', label: 'U', title: '下划线' },
  { format: 'strike', label: 'S', title: '删除线' },
]

const HEADING_OPTIONS = [
  { value: 'p', label: '正文' },
  { value: '1', label: '标题 1' },
  { value: '2', label: '标题 2' },
  { value: '3', label: '标题 3' },
  { value: '4', label: '标题 4' },
  { value: '5', label: '标题 5' },
  { value: '6', label: '标题 6' },
]

const LIST_TOGGLE_BUTTONS = [
  { format: 'bulletList', label: '无序', title: '无序列表' },
  { format: 'orderedList', label: '有序', title: '有序列表' },
  { format: 'taskList', label: '待办', title: '待办列表' },
]

function createEl(tag, attrs = {}, children = []) {
  const node = document.createElement(tag)
  for (const [key, value] of Object.entries(attrs)) {
    if (key === 'class') {
      node.className = value
    } else if (key === 'style') {
      node.setAttribute('style', value)
    } else if (value !== undefined && value !== null) {
      node.setAttribute(key, String(value))
    }
  }
  const list = Array.isArray(children) ? children : [children]
  for (const child of list) {
    if (child == null) continue
    node.appendChild(typeof child === 'string' ? document.createTextNode(child) : child)
  }
  return node
}

function applyToggle(editor, format) {
  const command = `toggle${format.charAt(0).toUpperCase()}${format.slice(1)}`
  editor.chain().focus()[command]().run()
}

function applyListToggle(editor, kind) {
  const command = `toggle${kind.charAt(0).toUpperCase()}${kind.slice(1)}`
  editor.chain().focus()[command]().run()
}
const TASK_LIST_STYLE_ID = 'dvn-tiptap-tasklist-style'

// 注入待办「已完成」主题化自包含样式：去列表符 + 已完成删除线/灰化，
// 颜色取 --color / --highlightColor / --backgroundColor 适配深浅色。
function injectTaskListStyles() {
  if (document.getElementById(TASK_LIST_STYLE_ID)) return
  const style = document.createElement('style')
  style.id = TASK_LIST_STYLE_ID
  style.textContent = [
    'ul[data-type="taskList"] { list-style: none; padding-left: 0; }',
    'ul[data-type="taskList"] li { display: flex; align-items: flex-start; gap: 6px; }',
    'ul[data-type="taskList"] li > label { display: inline-flex; align-items: center; flex: 0 0 auto; height: 1.72em; margin: 0; }',
    'ul[data-type="taskList"] li > label > input[type="checkbox"] { margin: 0; }',
    'ul[data-type="taskList"] li > div { flex: 1 1 auto; min-width: 0; }',
    'ul[data-type="taskList"] li > div > p { margin: 0; }',
    'ul[data-type="taskList"] li[data-checked="true"] { opacity: 0.55; }',
    'ul[data-type="taskList"] li[data-checked="true"] p {',
    '  color: var(--color, inherit);',
    '  text-decoration: line-through;',
    '  text-decoration-color: var(--highlightColor, #007AFF);',
    '}',
    'ul[data-type="taskList"] li[data-checked="true"] > label {',
    '  background: var(--backgroundColor, transparent);',
    '  border-color: var(--highlightColor, #007AFF);',
    '}',
  ].join('\n')
  document.head.appendChild(style)
}

function applyMarkColor(editor, markName, attrName, value) {
  editor.chain().focus().setMark(markName, { [attrName]: value }).run()
}

function clearMarkColor(editor, markName) {
  editor.chain().focus().unsetMark(markName).run()
}

function buildColorPicker(editor, kind, colors, apply, clear, readActive) {
  // kind: 'foreColor' | 'backColor'
  const wrapper = createEl('span', { class: 'tiptap-color-picker', style: 'position: relative; display: inline-flex;' })

  const toggle = createEl('button', { type: 'button', 'data-format': kind, title: kind === 'foreColor' ? '文字颜色' : '背景颜色' }, kind === 'foreColor' ? 'A' : '背景')
  toggle.setAttribute('aria-haspopup', 'true')
  toggle.setAttribute('aria-expanded', 'false')

  const panel = createEl('div', {
    class: 'tiptap-color-panel',
    'data-panel': kind,
    style: 'display: none; position: absolute; top: 100%; left: 0; background: var(--dvn-panel-bg, #fff); border: 1px solid var(--dvn-panel-border, #ccc); padding: 4px; z-index: 20;',
  })

  function openPanel() {
    panel.style.display = 'grid'
    toggle.setAttribute('aria-expanded', 'true')
  }

  function closePanel() {
    panel.style.display = 'none'
    toggle.setAttribute('aria-expanded', 'false')
  }

  function togglePanel() {
    if (panel.style.display === 'none' || !panel.style.display) {
      openPanel()
    } else {
      closePanel()
    }
  }

  toggle.addEventListener('click', (event) => {
    event.stopPropagation()
    togglePanel()
  })

  // 清除按钮
  const clearBtn = createEl('button', { type: 'button', 'data-action': `clear-${kind}`, title: '清除颜色', style: 'grid-column: 1 / -1; margin-top: 2px; border: 1px solid var(--dvn-panel-border, #ccc); background: var(--dvn-clear-btn-bg, #fafafa);' }, '清除颜色')
  clearBtn.addEventListener('click', () => {
    clear(editor)
    closePanel()
  })
  panel.appendChild(clearBtn)

  // 颜色网格
  for (const row of colors) {
    for (const color of row) {
      const cell = createEl('button', {
        type: 'button',
        'data-color': color,
        'data-kind': kind,
        title: color,
        style: `width: 18px; height: 18px; border: 1px solid #ddd; background: ${color}; padding: 0;`,
      })
      cell.addEventListener('click', () => {
        apply(editor, color)
        closePanel()
      })
      panel.appendChild(cell)
    }
  }

  wrapper.appendChild(toggle)
  wrapper.appendChild(panel)

  return { wrapper, toggle, panel, openPanel, closePanel, readActive }
}

export function createFormatToolbar(editor, host) {
  if (!editor) throw new Error('editor instance is required')
  if (!host) throw new Error('toolbar host element is required')

  injectTaskListStyles()

  const toolbar = createEl('div', {
    class: 'tiptap-toolbar',
    'data-testid': 'format-toolbar',
    style: [
      'display: flex',
      'flex-wrap: wrap',
      'align-items: center',
      'gap: 4px',
      'padding: 6px 8px',
      'border-bottom: 1px solid var(--dvn-toolbar-border, #d0d0d0)',
      'background: var(--dvn-clear-btn-bg, #fafafa)',
      'position: sticky',
      'top: 0',
      'z-index: 10',
    ].join(';'),
  })

  function keepEditorSelection(event) {
    const control = event.target.closest?.('button')
    if (control && toolbar.contains(control) && !control.disabled) {
      event.preventDefault()
    }
  }
  toolbar.addEventListener('pointerdown', keepEditorSelection)
  toolbar.addEventListener('mousedown', keepEditorSelection)

  const buttons = {}

  // 开关式按钮组
  const toggleGroup = createEl('span', { class: 'tiptap-toolbar-group', style: 'display: inline-flex; gap: 2px;' })
  for (const { format, label, title } of TOGGLE_BUTTONS) {
    const btn = createEl('button', { type: 'button', 'data-format': format, title }, label)
    btn.setAttribute('aria-pressed', 'false')
    btn.addEventListener('click', () => applyToggle(editor, format))
    toggleGroup.appendChild(btn)
    buttons[format] = btn
  }
  const quoteBtn = createEl('button', { type: 'button', 'data-format': 'blockquote', title: '引用' }, '"')
  quoteBtn.setAttribute('aria-pressed', 'false')
  quoteBtn.addEventListener('click', () => applyToggle(editor, 'blockquote'))
  toggleGroup.appendChild(quoteBtn)
  buttons.blockquote = quoteBtn
  toolbar.appendChild(toggleGroup)

  // 标题下拉
  const headingSelect = createEl('select', { 'data-control': 'heading', title: '标题', 'aria-label': '标题' })
  for (const { value, label } of HEADING_OPTIONS) {
    headingSelect.appendChild(createEl('option', { value }, label))
  }
  headingSelect.addEventListener('change', () => {
    const value = headingSelect.value
    if (value === 'p') {
      editor.chain().focus().setParagraph().run()
    } else {
      editor.chain().focus().toggleHeading({ level: Number(value) }).run()
    }
  })
  toolbar.appendChild(headingSelect)

  // 字体下拉
  const fontSelect = createEl('select', { 'data-control': 'fontFamily', title: '字体', 'aria-label': '字体' })
  fontSelect.appendChild(createEl('option', { value: '' }, '默认字体'))
  fontSelect.addEventListener('change', () => {
    const value = fontSelect.value
    if (!value) {
      clearMarkColor(editor, 'fontFamily')
    } else {
      applyMarkColor(editor, 'fontFamily', 'fontFamily', value)
    }
  })
  toolbar.appendChild(fontSelect)

  // 字号下拉
  const sizeSelect = createEl('select', { 'data-control': 'fontSize', title: '字号', 'aria-label': '字号' })
  sizeSelect.appendChild(createEl('option', { value: '' }, '默认字号'))
  for (const size of FONT_SIZES) {
    sizeSelect.appendChild(createEl('option', { value: size }, size))
  }
  sizeSelect.addEventListener('change', () => {
    const value = sizeSelect.value
    if (!value) {
      clearMarkColor(editor, 'fontSize')
    } else {
      applyMarkColor(editor, 'fontSize', 'fontSize', toPxSize(value))
    }
  })
  toolbar.appendChild(sizeSelect)

  // 文字颜色
  const forePicker = buildColorPicker(
    editor,
    'foreColor',
    FORE_COLORS,
    (editor, color) => applyMarkColor(editor, 'color', 'color', color),
    (editor) => clearMarkColor(editor, 'color'),
    () => editor.getAttributes('color').color,
  )
  toolbar.appendChild(forePicker.wrapper)

  // 背景颜色
  const backPicker = buildColorPicker(
    editor,
    'backColor',
    BACK_COLORS,
    (editor, color) => applyHighlight(editor, color),
    (editor) => clearHighlight(editor),
    () => editor.getAttributes('highlight').color,
  )
  toolbar.appendChild(backPicker.wrapper)

  // 列表/待办区（区段顺序：富文本格式区 → 列表/待办区 → 资源插入区）
  const listGroup = createEl('span', { class: 'tiptap-toolbar-group', style: 'display: inline-flex; gap: 2px; margin-left: 8px; border-left: 1px solid var(--dvn-toolbar-border, #d0d0d0); padding-left: 8px;' })
  const listButtons = {}
  for (const { format, label, title } of LIST_TOGGLE_BUTTONS) {
    const btn = createEl('button', { type: 'button', 'data-format': format, title }, label)
    btn.setAttribute('aria-pressed', 'false')
    btn.addEventListener('click', () => applyListToggle(editor, format))
    listGroup.appendChild(btn)
    listButtons[format] = btn
  }
  const indentBtn = createEl('button', { type: 'button', 'data-format': 'indentList', title: '增加缩进' }, '缩进')
  indentBtn.disabled = true
  indentBtn.addEventListener('click', () => sinkActiveListItem(editor))
  listGroup.appendChild(indentBtn)
  listButtons.indentList = indentBtn

  const outdentBtn = createEl('button', { type: 'button', 'data-format': 'outdentList', title: '减少缩进' }, '反缩进')
  outdentBtn.disabled = true
  outdentBtn.addEventListener('click', () => liftActiveListItem(editor))
  listGroup.appendChild(outdentBtn)
  listButtons.outdentList = outdentBtn

  toolbar.appendChild(listGroup)

  // 资源插入区：图片按钮（区段顺序：富文本格式区 → 列表/待办区 → 资源插入区）
  const resourceGroup = createEl('span', { class: 'tiptap-toolbar-group', style: 'display: inline-flex; gap: 2px; margin-left: 8px; border-left: 1px solid var(--dvn-toolbar-border, #d0d0d0); padding-left: 8px;' })
  const imageBtn = createEl('button', { type: 'button', 'data-format': 'insertImage', title: '插入图片' }, '图片')
  let onPickImage = null
  imageBtn.addEventListener('click', () => {
    if (onPickImage) onPickImage()
  })
  resourceGroup.appendChild(imageBtn)
  toolbar.appendChild(resourceGroup)

  // 点击外部关闭所有面板
  function onOutsideClick(event) {
    if (!toolbar.contains(event.target)) {
      forePicker.closePanel()
      backPicker.closePanel()
    }
  }
  document.addEventListener('click', onOutsideClick)

  // 激活态同步
  function syncActiveStates() {
    for (const format of ['bold', 'italic', 'underline', 'strike', 'blockquote']) {
      const btn = buttons[format]
      const active = editor.isActive(format)
      btn.setAttribute('aria-pressed', active ? 'true' : 'false')
      btn.classList.toggle('is-active', active)
    }

    // 标题
    let activeLevel = 'p'
    for (let level = 1; level <= 6; level++) {
      if (editor.isActive('heading', { level })) {
        activeLevel = String(level)
        break
      }
    }
    headingSelect.value = activeLevel

    // 字体
    const fontFamily = editor.getAttributes('fontFamily').fontFamily
    fontSelect.value = fontFamily || ''

    // 字号
    const fontSize = editor.getAttributes('fontSize').fontSize
    sizeSelect.value = fontSize ? String(parseInt(fontSize, 10)) : ''

    // 颜色选中态
    const foreColor = editor.getAttributes('color').color
    const backColor = editor.getAttributes('highlight').color
    syncColorCells(forePicker.panel, foreColor)
    syncColorCells(backPicker.panel, backColor)

    // 列表/待办区
    for (const format of ['bulletList', 'orderedList', 'taskList']) {
      const btn = listButtons[format]
      const active = editor.isActive(format)
      btn.setAttribute('aria-pressed', active ? 'true' : 'false')
      btn.classList.toggle('is-active', active)
    }
    listButtons.indentList.disabled = !canIndentActiveListItem(editor)
    listButtons.outdentList.disabled = !canOutdentActiveListItem(editor)
  }

  function syncColorCells(panel, activeColor) {
    const cells = panel.querySelectorAll('button[data-color]')
    for (const cell of cells) {
      const color = cell.getAttribute('data-color')
      const isActive = activeColor && color === activeColor
      cell.setAttribute('aria-pressed', isActive ? 'true' : 'false')
      cell.style.outline = isActive ? `2px solid var(--dvn-active-outline, #0086cc)` : ''
    }
  }

  editor.on('transaction', syncActiveStates)
  syncActiveStates()

  host.appendChild(toolbar)

  return {
    setOnPickImage(fn) {
      onPickImage = typeof fn === 'function' ? fn : null
    },
    setFontList(fonts, defaultFont) {
      fontSelect.innerHTML = ''
      fontSelect.appendChild(createEl('option', { value: '' }, '默认字体'))
      const list = Array.isArray(fonts) ? fonts : []
      for (const name of list) {
        const option = createEl('option', { value: name }, name)
        if (defaultFont && name === defaultFont) {
          option.setAttribute('selected', 'selected')
        }
        fontSelect.appendChild(option)
      }
      const currentFont = editor.getAttributes('fontFamily').fontFamily
      fontSelect.value = currentFont || (defaultFont && list.includes(defaultFont) ? defaultFont : '')
    },
    destroy() {
      document.removeEventListener('click', onOutsideClick)
    },
  }
}

function applyHighlight(editor, color) {
  editor.chain().focus().setHighlight({ color }).run()
}

function clearHighlight(editor) {
  editor.chain().focus().unsetHighlight().run()
}
