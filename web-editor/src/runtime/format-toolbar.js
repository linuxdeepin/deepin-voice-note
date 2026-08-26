// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import toolbarCss from './format-toolbar.css?inline'
import boldIconUrl from '../../../src/web/css/createfont/svg/bold.svg?url'
import italicIconUrl from '../../../src/web/css/createfont/svg/Italic.svg?url'
import underlineIconUrl from '../../../src/web/css/createfont/svg/underline.svg?url'
import strikeIconUrl from '../../../src/web/css/createfont/svg/Strikethrough.svg?url'
import bulletDotIconUrl from '../../../src/web/css/createfont/svg/bullet_dot.svg?url'
import bulletNumberIconUrl from '../../../src/web/css/createfont/svg/bullet_number.svg?url'
import markerIconUrl from '../../../src/web/css/createfont/svg/marker.svg?url'
import textColorIconUrl from '../../../src/web/css/createfont/svg/text_color.svg?url'
import arrowIconUrl from '../../../src/web/css/createfont/svg/richtext_arrow.svg?url'
import moreIconUrl from './icons/more.svg?url'
import micIconUrl from './icons/mic.svg?url'
import imageIconUrl from './icons/image.svg?url'
import taskListIconUrl from './icons/task.svg?url'
import { FORE_COLORS, BACK_COLORS, FONT_SIZES, toPxSize } from './format-palette.js'
import { canIndentActiveListItem, canOutdentActiveListItem, liftActiveListItem, sinkActiveListItem } from './list-behavior.js'

const TOGGLE_BUTTONS = [
  { format: 'bold', icon: 'bold', title: '粗体', className: 'tiptap-format-bold' },
  { format: 'italic', icon: 'italic', title: '斜体', className: 'tiptap-format-italic' },
  { format: 'underline', icon: 'underline', title: '下划线', className: 'tiptap-format-underline' },
  { format: 'strike', icon: 'strike', title: '删除线', className: 'tiptap-format-strike' },
]

const HEADING_OPTIONS = [
  { value: 'p', label: '标题' },
  { value: '1', label: '标题 1' },
  { value: '2', label: '标题 2' },
  { value: '3', label: '标题 3' },
  { value: '4', label: '标题 4' },
  { value: '5', label: '标题 5' },
  { value: '6', label: '标题 6' },
]

const LIST_TOGGLE_BUTTONS = [
  { format: 'bulletList', icon: 'bulletList', title: '无序列表' },
  { format: 'orderedList', icon: 'orderedList', title: '有序列表' },
  { format: 'taskList', icon: 'taskList', title: '待办列表' },
]


const TOOLBAR_STYLE_ID = 'dvn-tiptap-format-toolbar-style'
const TOOLBAR_HORIZONTAL_MARGIN = 10

function injectToolbarStyles() {
  if (document.getElementById(TOOLBAR_STYLE_ID)) return
  const style = document.createElement('style')
  style.id = TOOLBAR_STYLE_ID
  style.textContent = toolbarCss
  document.head.appendChild(style)
}

const ICON_ASSET_URLS = Object.freeze({
  bold: boldIconUrl,
  italic: italicIconUrl,
  underline: underlineIconUrl,
  strike: strikeIconUrl,
  bulletList: bulletDotIconUrl,
  orderedList: bulletNumberIconUrl,
  marker: markerIconUrl,
  textColor: textColorIconUrl,
  richtextArrow: arrowIconUrl,
  more: moreIconUrl,
  mic: micIconUrl,
  image: imageIconUrl,
  taskList: taskListIconUrl,
})

function createSvgIcon(name) {
  const span = createEl('span', { class: 'tiptap-icon', 'aria-hidden': 'true' })
  const assetUrl = ICON_ASSET_URLS[name]
  if (!assetUrl) return span

  // 外部 SVG 作为 img 时不会继承按钮的 currentColor。保留 img 作为正常态，
  // 同时记录资源地址，让 CSS 在激活态用 mask 以主题色重绘图标。
  span.classList.add('tiptap-icon--asset')
  span.style.setProperty('--tiptap-icon-mask', `url("${assetUrl}")`)

  const image = createEl('img', {
    src: assetUrl,
    alt: '',
    draggable: 'false',
    'aria-hidden': 'true',
  })
  span.appendChild(image)
  return span
}

function createSeparator() {
  return createEl('span', { class: 'tiptap-toolbar-separator', 'aria-hidden': 'true' })
}

function createToolbarButton({ format, title, children, className = '' }) {
  const btn = createEl('button', { type: 'button', 'data-format': format, title, class: className }, children)
  return btn
}

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
  const wrapper = createEl('span', { class: 'tiptap-color-picker' })

  const icon = createEl('span', { class: 'tiptap-color-icon' }, [
    createSvgIcon(kind === 'foreColor' ? 'textColor' : 'marker'),
  ])
  const arrow = createEl('span', { class: 'tiptap-color-arrow', 'aria-hidden': 'true' }, [
    createSvgIcon('richtextArrow'),
  ])
  const toggle = createEl('button', {
    type: 'button',
    class: 'tiptap-color-button',
    'data-format': kind,
    title: kind === 'foreColor' ? '文字颜色' : '背景颜色',
  }, [icon, arrow])
  toggle.setAttribute('aria-haspopup', 'true')
  toggle.setAttribute('aria-expanded', 'false')

  const panel = createEl('div', {
    class: 'tiptap-color-panel',
    'data-panel': kind,
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

  function setCurrentColor(color) {
    // 颜色指示条跟随当前显式选区；无颜色时恢复 Summernote 的默认值。
    toggle.style.setProperty('--dvn-current-color', color || (kind === 'foreColor' ? '#000000' : '#0081ff'))
  }

  toggle.addEventListener('click', (event) => {
    event.stopPropagation()
    togglePanel()
  })

  // 清除按钮
  const clearBtn = createEl('button', { type: 'button', 'data-action': `clear-${kind}`, title: '清除颜色' }, '清除颜色')
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
        style: `background: ${color};`,
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

  setCurrentColor('')
  return { wrapper, toggle, panel, openPanel, closePanel, setCurrentColor, readActive }
}

export function createFormatToolbar(editor, host) {
  if (!editor) throw new Error('editor instance is required')
  if (!host) throw new Error('toolbar host element is required')

  injectToolbarStyles()
  injectTaskListStyles()

  const toolbar = createEl('div', {
    class: 'tiptap-toolbar',
    'data-testid': 'format-toolbar',
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

  // 样式区：标题 / 字体 / 字号
  const styleGroup = createEl('span', { class: 'tiptap-toolbar-group tiptap-style-group' })

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
  styleGroup.appendChild(headingSelect)

  const fontSelect = createEl('select', { 'data-control': 'fontFamily', title: '字体', 'aria-label': '字体' })
  fontSelect.appendChild(createEl('option', { value: '' }, '字体'))
  fontSelect.addEventListener('change', () => {
    const value = fontSelect.value
    if (!value) {
      clearMarkColor(editor, 'fontFamily')
    } else {
      applyMarkColor(editor, 'fontFamily', 'fontFamily', value)
    }
  })
  styleGroup.appendChild(fontSelect)

  const sizeSelect = createEl('select', { 'data-control': 'fontSize', title: '字号', 'aria-label': '字号' })
  sizeSelect.appendChild(createEl('option', { value: '' }, '14'))
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
  styleGroup.appendChild(sizeSelect)
  toolbar.appendChild(styleGroup)
  const styleToggleSeparator = createSeparator()
  toolbar.appendChild(styleToggleSeparator)

  // 开关式按钮区：粗体 / 斜体 / 下划线 / 删除线
  const toggleGroup = createEl('span', { class: 'tiptap-toolbar-group tiptap-toggle-group' })
  for (const { format, icon, title, className } of TOGGLE_BUTTONS) {
    const btn = createToolbarButton({ format, title, className, children: createSvgIcon(icon) })
    btn.setAttribute('aria-pressed', 'false')
    btn.addEventListener('click', () => applyToggle(editor, format))
    toggleGroup.appendChild(btn)
    buttons[format] = btn
  }
  // 保留引用命令供测试/快捷入口复用，但截图样式不显示引用按钮。
  const quoteBtn = createToolbarButton({ format: 'blockquote', title: '引用', children: '"', className: 'tiptap-hidden-command' })
  quoteBtn.setAttribute('aria-pressed', 'false')
  quoteBtn.addEventListener('click', () => applyToggle(editor, 'blockquote'))
  toggleGroup.appendChild(quoteBtn)
  buttons.blockquote = quoteBtn
  toolbar.appendChild(toggleGroup)
  const toggleColorSeparator = createSeparator()
  toolbar.appendChild(toggleColorSeparator)

  // 文字颜色 / 背景颜色
  const colorGroup = createEl('span', { class: 'tiptap-toolbar-group tiptap-color-group' })
  const forePicker = buildColorPicker(
    editor,
    'foreColor',
    FORE_COLORS,
    (editor, color) => applyMarkColor(editor, 'color', 'color', color),
    (editor) => clearMarkColor(editor, 'color'),
    () => editor.getAttributes('color').color,
  )
  colorGroup.appendChild(forePicker.wrapper)

  const backPicker = buildColorPicker(
    editor,
    'backColor',
    BACK_COLORS,
    (editor, color) => applyHighlight(editor, color),
    (editor) => clearHighlight(editor),
    () => editor.getAttributes('highlight').color,
  )
  colorGroup.appendChild(backPicker.wrapper)
  toolbar.appendChild(colorGroup)
  const colorListSeparator = createSeparator()
  toolbar.appendChild(colorListSeparator)

  // 列表/待办区
  const listGroup = createEl('span', { class: 'tiptap-toolbar-group tiptap-list-group' })
  const listButtons = {}
  for (const { format, icon, title } of LIST_TOGGLE_BUTTONS) {
    const btn = createToolbarButton({ format, title, children: createSvgIcon(icon) })
    btn.setAttribute('aria-pressed', 'false')
    btn.addEventListener('click', () => applyListToggle(editor, format))
    listGroup.appendChild(btn)
    listButtons[format] = btn
  }
  const indentBtn = createToolbarButton({ format: 'indentList', title: '增加缩进', children: '缩进', className: 'tiptap-hidden-command' })
  indentBtn.disabled = true
  indentBtn.addEventListener('click', () => sinkActiveListItem(editor))
  listGroup.appendChild(indentBtn)
  listButtons.indentList = indentBtn

  const outdentBtn = createToolbarButton({ format: 'outdentList', title: '减少缩进', children: '反缩进', className: 'tiptap-hidden-command' })
  outdentBtn.disabled = true
  outdentBtn.addEventListener('click', () => liftActiveListItem(editor))
  listGroup.appendChild(outdentBtn)
  listButtons.outdentList = outdentBtn
  toolbar.appendChild(listGroup)
  const listResourceSeparator = createSeparator()
  toolbar.appendChild(listResourceSeparator)

  // 资源插入区：录音图标 / 图片按钮
  const resourceGroup = createEl('span', { class: 'tiptap-toolbar-group tiptap-resource-group' })
  let onRecordVoice = null
  const voiceBtn = createToolbarButton({ format: 'insertVoice', title: '录音', children: createSvgIcon('mic') })
  voiceBtn.addEventListener('click', () => {
    if (onRecordVoice) onRecordVoice()
  })
  resourceGroup.appendChild(voiceBtn)

  const imageBtn = createToolbarButton({ format: 'insertImage', title: '插入图片', children: createSvgIcon('image') })
  let onPickImage = null
  imageBtn.addEventListener('click', () => {
    if (onPickImage) onPickImage()
  })
  resourceGroup.appendChild(imageBtn)
  toolbar.appendChild(resourceGroup)

  function setResourceButtonsEnabled(voiceEnabled = true, imageEnabled = true) {
    voiceBtn.disabled = !voiceEnabled
    imageBtn.disabled = !imageEnabled
  }

  const moreSeparator = createSeparator()
  moreSeparator.classList.add('tiptap-more-separator')
  const moreBtn = createToolbarButton({
    format: 'more',
    title: '更多格式',
    className: 'tiptap-more-button',
    children: createSvgIcon('more'),
  })
  moreBtn.setAttribute('aria-haspopup', 'true')
  moreBtn.setAttribute('aria-expanded', 'false')
  const overflowPanel = createEl('div', { class: 'tiptap-overflow-panel', role: 'group', 'aria-label': '更多格式' })
  const overflowGroups = {
    toggle: createEl('span', { class: 'tiptap-toolbar-group tiptap-toggle-group' }),
    color: createEl('span', { class: 'tiptap-toolbar-group tiptap-color-group' }),
    list: createEl('span', { class: 'tiptap-toolbar-group tiptap-list-group' }),
    resource: createEl('span', { class: 'tiptap-toolbar-group tiptap-resource-group' }),
  }
  const mainGroups = {
    toggle: toggleGroup,
    color: colorGroup,
    list: listGroup,
    resource: resourceGroup,
  }
  const groupOrder = ['toggle', 'color', 'list', 'resource']
  const overflowUnits = [
    { group: 'toggle', node: buttons.bold },
    { group: 'toggle', node: buttons.italic },
    { group: 'toggle', node: buttons.underline },
    { group: 'toggle', node: buttons.strike },
    { group: 'color', node: forePicker.wrapper },
    { group: 'color', node: backPicker.wrapper },
    { group: 'list', node: listButtons.bulletList },
    { group: 'list', node: listButtons.orderedList },
    { group: 'list', node: listButtons.taskList },
    { group: 'resource', node: voiceBtn },
    { group: 'resource', node: imageBtn },
  ]
  const overflowSet = new Set()

  function removeAllChildren(node) {
    while (node.firstChild) node.removeChild(node.firstChild)
  }

  function closeOverflowPanel() {
    overflowPanel.classList.remove('is-open')
    moreBtn.setAttribute('aria-expanded', 'false')
  }

  function toggleOverflowPanel() {
    const open = !overflowPanel.classList.contains('is-open')
    overflowPanel.classList.toggle('is-open', open)
    moreBtn.setAttribute('aria-expanded', open ? 'true' : 'false')
    forePicker.closePanel()
    backPicker.closePanel()
  }

  function unitsInGroup(group, inOverflow) {
    return overflowUnits.filter((unit) => unit.group === group && overflowSet.has(unit) === inOverflow)
  }

  function appendGroupWithSeparator(parent, separator, group, units) {
    if (!units.length) return false
    parent.appendChild(separator)
    parent.appendChild(group)
    return true
  }

  function renderOverflowLayout() {
    closeOverflowPanel()
    toolbar.classList.toggle('is-overflowing', overflowSet.size > 0)

    removeAllChildren(toolbar)
    for (const group of Object.values(mainGroups)) removeAllChildren(group)
    for (const group of Object.values(overflowGroups)) removeAllChildren(group)
    removeAllChildren(overflowPanel)

    toolbar.appendChild(styleGroup)

    for (const unit of overflowUnits) {
      const target = overflowSet.has(unit) ? overflowGroups[unit.group] : mainGroups[unit.group]
      target.appendChild(unit.node)
    }
    // 隐藏命令仍保留在主 DOM 中，供既有测试/命令入口复用。
    toggleGroup.appendChild(quoteBtn)
    listGroup.appendChild(indentBtn)
    listGroup.appendChild(outdentBtn)

    const mainSeparators = {
      toggle: styleToggleSeparator,
      color: toggleColorSeparator,
      list: colorListSeparator,
      resource: listResourceSeparator,
    }
    for (const group of groupOrder) {
      appendGroupWithSeparator(toolbar, mainSeparators[group], mainGroups[group], unitsInGroup(group, false))
    }

    if (overflowSet.size > 0) {
      toolbar.appendChild(moreSeparator)
      toolbar.appendChild(moreBtn)
      toolbar.appendChild(overflowPanel)

      for (const group of groupOrder) {
        const units = unitsInGroup(group, true)
        if (!units.length) continue
        if (overflowPanel.childNodes.length > 0) {
          overflowPanel.appendChild(createSeparator())
        }
        overflowPanel.appendChild(overflowGroups[group])
      }
    }
  }

  function measuredToolbarWidth() {
    return toolbar.scrollWidth || toolbar.getBoundingClientRect().width || 0
  }

  function updateOverflowMode() {
    const hostWidth = host.clientWidth || document.documentElement.clientWidth || window.innerWidth || 0
    if (!hostWidth) return
    // 顶部工具栏跟随宿主宽度；左右各保留 10px。
    // Summernote 的 385px 只用于 airPopover 定位参考，不能限制当前常驻顶部工具栏。
    const maxToolbarWidth = Math.max(0, hostWidth - TOOLBAR_HORIZONTAL_MARGIN * 2)

    overflowSet.clear()
    renderOverflowLayout()
    if (measuredToolbarWidth() <= maxToolbarWidth + 1) return

    // 只从末尾向前折叠，保持主工具栏始终是原始按钮顺序的连续前缀。
    // 不能为了保留某个按钮而折叠中间按钮，否则会造成工具栏顺序断裂。
    for (let index = overflowUnits.length - 1; index >= 0; index--) {
      overflowSet.add(overflowUnits[index])
      renderOverflowLayout()
      if (measuredToolbarWidth() <= maxToolbarWidth + 1) break
    }
  }

  moreBtn.addEventListener('click', (event) => {
    event.stopPropagation()
    toggleOverflowPanel()
  })

  // 点击外部关闭所有面板
  function onOutsideClick(event) {
    if (!toolbar.contains(event.target)) {
      forePicker.closePanel()
      backPicker.closePanel()
      closeOverflowPanel()
    }
  }
  document.addEventListener('click', onOutsideClick)

  // 激活态同步
  function setPressed(btn, active) {
    btn.setAttribute('aria-pressed', active ? 'true' : 'false')
    btn.classList.toggle('is-active', active)
  }

  function syncActiveStates() {
    // 工具栏按钮高亮只表达当前显式选区的格式。
    // 页面加载/普通光标停留时 Tiptap 也会保留内部 selection，不能仅因光标所在位置或文档中存在
    // 列表/代办就把对应按钮显示为选中态；这与 Summernote 顶部工具栏反馈保持一致。
    const hasEditorContext = Boolean(editor.isFocused)
    const hasSelectionContext = hasEditorContext && !editor.state.selection.empty

    for (const format of ['bold', 'italic', 'underline', 'strike', 'blockquote']) {
      const btn = buttons[format]
      setPressed(btn, hasSelectionContext && editor.isActive(format))
    }

    // 标题
    let activeLevel = 'p'
    if (hasSelectionContext) {
      for (let level = 1; level <= 6; level++) {
        if (editor.isActive('heading', { level })) {
          activeLevel = String(level)
          break
        }
      }
    }
    headingSelect.value = activeLevel

    // 字体
    const fontFamily = hasSelectionContext ? editor.getAttributes('fontFamily').fontFamily : ''
    fontSelect.value = fontFamily || ''

    // 字号
    const fontSize = hasSelectionContext ? editor.getAttributes('fontSize').fontSize : ''
    sizeSelect.value = fontSize ? String(parseInt(fontSize, 10)) : ''

    // 颜色选中态
    const foreColor = hasSelectionContext ? editor.getAttributes('color').color : ''
    const backColor = hasSelectionContext ? editor.getAttributes('highlight').color : ''
    forePicker.setCurrentColor(foreColor)
    backPicker.setCurrentColor(backColor)
    syncColorCells(forePicker.panel, foreColor)
    syncColorCells(backPicker.panel, backColor)

    // 列表/待办区
    for (const format of ['bulletList', 'orderedList', 'taskList']) {
      const btn = listButtons[format]
      setPressed(btn, hasSelectionContext && editor.isActive(format))
    }
    listButtons.indentList.disabled = !hasEditorContext || !canIndentActiveListItem(editor)
    listButtons.outdentList.disabled = !hasEditorContext || !canOutdentActiveListItem(editor)
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
  editor.on('focus', syncActiveStates)
  editor.on('blur', syncActiveStates)
  syncActiveStates()

  host.appendChild(toolbar)
  updateOverflowMode()
  const resizeObserver = typeof ResizeObserver !== 'undefined'
    ? new ResizeObserver(() => updateOverflowMode())
    : null
  resizeObserver?.observe(host)
  window.addEventListener('resize', updateOverflowMode)

  return {
    setOnPickImage(fn) {
      onPickImage = typeof fn === 'function' ? fn : null
    },
    setOnRecordVoice(fn) {
      onRecordVoice = typeof fn === 'function' ? fn : null
    },
    setResourceButtonsEnabled,
    setFontList(fonts, defaultFont) {
      fontSelect.replaceChildren()
      const normalized = Array.isArray(fonts)
        ? fonts.filter((name, index, array) => typeof name === 'string' && name.trim() && array.indexOf(name) === index)
        : []
      const list = normalized.length > 0 ? [...normalized] : []
      if (defaultFont && !list.includes(defaultFont)) {
        list.unshift(defaultFont)
      }
      const effectiveDefaultFont = defaultFont || list[0] || ''
      if (effectiveDefaultFont) {
        document.body.style.fontFamily = effectiveDefaultFont
        document.documentElement.style.setProperty('--dvn-editor-default-font', effectiveDefaultFont)
      }

      // 与 Summernote 保持一致：没有显式 fontFamily mark 时，按钮仍显示系统默认字体。
      // 因此保留 value="" 的默认项；syncActiveStates() 在无 mark 时会回到该项。
      fontSelect.appendChild(createEl('option', { value: '' }, effectiveDefaultFont || '字体'))

      for (const name of list) {
        const option = createEl('option', { value: name }, name)
        fontSelect.appendChild(option)
      }
      const currentFont = editor.getAttributes('fontFamily').fontFamily
      fontSelect.value = currentFont && list.includes(currentFont) ? currentFont : ''
      updateOverflowMode()
    },
    destroy() {
      document.removeEventListener('click', onOutsideClick)
      window.removeEventListener('resize', updateOverflowMode)
      editor.off?.('transaction', syncActiveStates)
      editor.off?.('focus', syncActiveStates)
      editor.off?.('blur', syncActiveStates)
      resizeObserver?.disconnect()
    },
  }
}

function applyHighlight(editor, color) {
  editor.chain().focus().setHighlight({ color }).run()
}

function clearHighlight(editor) {
  editor.chain().focus().unsetHighlight().run()
}
