// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import toolbarCss from './format-toolbar.css?inline'
import { AllSelection, TextSelection } from '@tiptap/pm/state'
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
import checkIconUrl from './icons/check.svg?url'
import { FORE_COLORS, BACK_COLORS, FONT_SIZES, HEADING_FONT_SIZES, colorPaletteForTheme, toPxSize } from './format-palette.js'
import { activeListType, canIndentActiveListItem, canOutdentActiveListItem, liftActiveListItem, sinkActiveListItem, switchActiveListType, switchSelectedListType } from './list-behavior.js'

const TOGGLE_BUTTONS = [
  { format: 'bold', icon: 'bold', title: '粗体', className: 'tiptap-format-bold' },
  { format: 'italic', icon: 'italic', title: '斜体', className: 'tiptap-format-italic' },
  { format: 'underline', icon: 'underline', title: '下划线', className: 'tiptap-format-underline' },
  { format: 'strike', icon: 'strike', title: '删除线', className: 'tiptap-format-strike' },
]

const INLINE_TOGGLE_FORMATS = new Set(['bold', 'italic', 'underline', 'strike'])
const TOOLBAR_PRESS_CLEAR_EVENTS = ['pointerup', 'pointercancel', 'pointerleave', 'blur']

const HEADING_OPTIONS = [
  { value: 'p', label: '正文' },
  { value: '1', label: '标题1' },
  { value: '2', label: '标题2' },
  { value: '3', label: '标题3' },
  { value: '4', label: '标题4' },
  { value: '5', label: '标题5' },
]

const LIST_TOGGLE_BUTTONS = [
  { format: 'bulletList', icon: 'bulletList', title: '无序列表' },
  { format: 'orderedList', icon: 'orderedList', title: '有序列表' },
  { format: 'taskList', icon: 'taskList', title: '待办列表' },
]


const TOOLBAR_STYLE_ID = 'dvn-tiptap-format-toolbar-style'
const TOOLBAR_LEFT_MARGIN = 10
const DEFAULT_FORE_COLOR = 'var(--dvn-editor-fg, rgb(65, 77, 104))'
const DEFAULT_BACK_COLOR = 'transparent'

function normalizeCssColor(value) {
  if (!value) return ''
  const color = String(value).trim().toLowerCase().replace(/\s*,\s*/g, ', ')
  if (!color) return ''
  if (color === 'transparent') return 'transparent'

  const rgbaMatch = color.match(/^rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)(?:\s*,\s*([0-9.]+))?\s*\)$/)
  if (rgbaMatch) {
    const [, r, g, b, alpha] = rgbaMatch
    const a = alpha == null ? 1 : Number(alpha)
    if (a === 0) return 'transparent'
    if (a !== 1) return `rgba(${Number(r)}, ${Number(g)}, ${Number(b)}, ${String(a)})`
    return rgbToHex(Number(r), Number(g), Number(b))
  }

  const hexMatch = color.match(/^#([0-9a-f]{3}|[0-9a-f]{6})$/i)
  if (hexMatch) {
    const hex = hexMatch[1].length === 3
      ? hexMatch[1].split('').map((part) => part + part).join('')
      : hexMatch[1]
    return `#${hex.toLowerCase()}`
  }

  return color
}

function rgbToHex(r, g, b) {
  return `#${[r, g, b].map((part) => Math.max(0, Math.min(255, part)).toString(16).padStart(2, '0')).join('')}`
}

function sameCssColor(left, right) {
  return normalizeCssColor(left) === normalizeCssColor(right)
}

function collapsedMarks(state) {
  // storedMarks 是 ProseMirror 对“下一次输入”的权威格式；只有它为空时才
  // 回退到光标位置继承的 marks。不能用二者做并集，否则刚取消的格式会从
  // 光标前后的文字里被重新读回来。
  return state.storedMarks ?? state.selection.$from.marks()
}

function selectedTextRuns(state) {
  const { doc, selection } = state
  if (selection.empty) return []
  const runs = []
  doc.nodesBetween(selection.from, selection.to, (node, pos) => {
    if (!node.isText) return true
    const from = Math.max(selection.from, pos)
    const to = Math.min(selection.to, pos + node.nodeSize)
    if (to > from) runs.push({ node, pos, from, to })
    return false
  })
  return runs
}

function nearestTextblockAt(doc, pos) {
  const $pos = doc.resolve(Math.max(0, Math.min(doc.content.size, pos + 1)))
  for (let depth = $pos.depth; depth > 0; depth -= 1) {
    const node = $pos.node(depth)
    if (node.isTextblock) return { node, pos: $pos.before(depth) }
  }
  return null
}

function nearestTextblockAtSelection(state) {
  const { $from } = state.selection
  for (let depth = $from.depth; depth > 0; depth -= 1) {
    const node = $from.node(depth)
    if (node.isTextblock) return { node, pos: $from.before(depth) }
  }
  return null
}

function selectionTouchesTextblock(selection, pos, node) {
  const from = pos + 1
  const to = from + node.content.size
  return selection.from <= to && selection.to >= from
}

// 返回当前光标/选区中同一种 mark 属性是否一致。显式选区的判定口径是：
// 选区覆盖到的每段文字都必须有该 mark；任何一段缺失或取值不同都视为混合。
function uniformMarkContext(state, markName, attrName = '', normalize = value => value) {
  const { selection } = state
  if (selection.empty) {
    const mark = collapsedMarks(state).find((item) => item.type.name === markName)
    const value = mark ? (attrName ? mark.attrs?.[attrName] : true) : (attrName ? '' : false)
    return {
      value: value ?? '',
      normalizedValue: normalize(value ?? ''),
      mixed: false,
      hasMark: Boolean(mark),
      hasText: false,
    }
  }

  const runs = selectedTextRuns(state)
  if (runs.length === 0) {
    return { value: attrName ? '' : false, normalizedValue: normalize(''), mixed: false, hasMark: false, hasText: false }
  }

  let firstRaw
  let firstNormalized
  let mixed = false
  for (const { node } of runs) {
    const mark = node.marks.find((item) => item.type.name === markName)
    const raw = mark ? (attrName ? mark.attrs?.[attrName] : true) : (attrName ? '' : false)
    const value = raw ?? ''
    const normalized = normalize(value)
    if (firstRaw === undefined) {
      firstRaw = value
      firstNormalized = normalized
    } else if (!mixed && normalized !== firstNormalized) {
      mixed = true
    }
  }

  return {
    value: mixed ? (attrName ? '' : false) : firstRaw,
    normalizedValue: mixed ? undefined : firstNormalized,
    mixed,
    hasMark: !mixed && Boolean(firstRaw),
    hasText: true,
  }
}

function isUniformInlineMarkActive(state, markName) {
  const context = uniformMarkContext(state, markName)
  return !context.mixed && context.hasMark
}

function isNormalFontWeight(value) {
  const normalized = String(value ?? '').trim().toLowerCase()
  return normalized === 'normal' || normalized === '400'
}

function textRunIsBold(doc, node, pos) {
  const weightMark = node.marks.find((mark) => mark.type.name === 'fontWeight')
  const weight = weightMark?.attrs?.fontWeight
  if (isNormalFontWeight(weight)) return false
  if (node.marks.some((mark) => mark.type.name === 'bold')) return true
  if (weight) return true
  return nearestTextblockAt(doc, pos)?.node?.type?.name === 'heading'
}

function isBoldContextActive(state) {
  const { selection, doc } = state
  if (selection.empty) {
    const marks = collapsedMarks(state)
    const weight = marks.find((mark) => mark.type.name === 'fontWeight')?.attrs?.fontWeight
    if (isNormalFontWeight(weight)) return false
    if (marks.some((mark) => mark.type.name === 'bold') || weight) return true
    return nearestTextblockAtSelection(state)?.node?.type?.name === 'heading'
  }

  const runs = selectedTextRuns(state)
  return runs.length > 0 && runs.every(({ node, pos }) => textRunIsBold(doc, node, pos))
}

function textblockStyleContext(state) {
  const { doc, selection } = state
  if (selection.empty) {
    const textblock = nearestTextblockAtSelection(state)
    const level = textblock?.node?.type?.name === 'heading' ? textblock.node.attrs?.level : null
    return { value: level ? String(level) : 'p', mixed: false, hasText: false }
  }

  const styles = []
  doc.nodesBetween(selection.from, selection.to, (node, pos) => {
    if (!node.isTextblock || !selectionTouchesTextblock(selection, pos, node)) return true
    const level = node.type.name === 'heading' ? node.attrs?.level : null
    styles.push(level ? String(level) : 'p')
    return false
  })
  if (styles.length === 0) return { value: 'p', mixed: false, hasText: false }

  const first = styles[0]
  const mixed = styles.some((value) => value !== first)
  return { value: mixed ? '' : first, mixed, hasText: true }
}

function explicitFontSizeFromMarks(marks) {
  const raw = marks.find((mark) => mark.type.name === 'fontSize')?.attrs?.fontSize
  const value = Number.parseInt(String(raw ?? ''), 10)
  return Number.isFinite(value) ? String(value) : ''
}

function effectiveFontSizeAt(doc, pos, marks) {
  const explicit = explicitFontSizeFromMarks(marks)
  if (explicit) return explicit
  const textblock = nearestTextblockAt(doc, pos)
  const level = textblock?.node?.type?.name === 'heading' ? textblock.node.attrs?.level : null
  return HEADING_FONT_SIZES[level] || '14'
}

function effectiveFontSizeContext(state) {
  const { selection, doc } = state
  if (selection.empty) {
    return {
      value: effectiveFontSizeAt(doc, selection.from, collapsedMarks(state)),
      mixed: false,
      hasText: false,
    }
  }

  const runs = selectedTextRuns(state)
  if (runs.length === 0) return { value: '14', mixed: false, hasText: false }
  const values = runs.map(({ node, pos }) => effectiveFontSizeAt(doc, pos, node.marks))
  const first = values[0]
  const mixed = values.some((value) => value !== first)
  return { value: mixed ? '' : first, mixed, hasText: true }
}

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

  // 外部 SVG 作为 img 时不会继承按钮的 currentColor。保留 img 作为资源/fallback，
  // 同时记录资源地址，让 CSS 用 mask 统一按当前主题色重绘图标。
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
  // QtWebEngine 在 sticky 工具栏和 fixed 溢出面板中无法稳定地定位原生
  // title tooltip，偶尔会跑到编辑器左上角。统一使用按钮自身锚定的
  // data-tooltip，原生 title 不再参与定位。
  const btn = createEl('button', {
    type: 'button',
    'data-format': format,
    'data-tooltip': title,
    'aria-label': title,
    class: className,
  }, children)
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
  // 折叠光标按“当前行/当前列表项”互切；显式选中完整列表块时
  // 递归转换整个选区列表块，避免只转换选区起点或打散嵌套结构。
  if (switchSelectedListType(editor, kind)) return
  if (editor.state.selection.empty && switchActiveListType(editor, kind)) return

  const command = `toggle${kind.charAt(0).toUpperCase()}${kind.slice(1)}`
  editor.chain().focus()[command]().run()
}
const TASK_LIST_STYLE_ID = 'dvn-tiptap-tasklist-style'

// 注入待办列表主题化自包含样式：去列表符 + 已完成内容弱化。
// 已完成态不改正文颜色、不加删除线，避免文字变成主题色。
function injectTaskListStyles() {
  if (document.getElementById(TASK_LIST_STYLE_ID)) return
  const style = document.createElement('style')
  style.id = TASK_LIST_STYLE_ID
  style.textContent = [
    'ul[data-type="taskList"] { list-style: none; padding-left: 0; }',
    'ul[data-type="taskList"] ul[data-type="taskList"] { padding-left: 20px; }',
    'ul[data-type="taskList"] > li > div > ul[data-type="taskList"] { margin-left: -20px; }',
    'ul[data-type="taskList"] > li > div > ul[data-type="taskList"] > li > div > ul[data-type="taskList"] > li > div > ul[data-type="taskList"] { padding-left: 0; }',
    'ul[data-type="taskList"] > li[data-dvn-indent-level="1"] > div > ul[data-type="taskList"] { margin-left: -40px; }',
    'ul[data-type="taskList"] > li[data-dvn-indent-level="2"] > div > ul[data-type="taskList"] { margin-left: -60px; }',
    // 勾选框与 • / 1. 用同一套规则：勾选框固定 13px，不跟随正文字号；
    // label 盒高用 1lh * max(1, ratio) 跟踪首行有效行盒，align-items:center
    // 把勾选框放在首行中心。li 用 flex-start 让 label 从首行顶部开始，
    // 折行/多段时也不会掉到整个列表项中间。
    'ul[data-type="taskList"] > li { display: flex; align-items: flex-start; gap: 0; }',
    'ul[data-type="taskList"] > li > label { display: inline-flex; align-items: center; justify-content: center; flex: 0 0 var(--dvn-marker-gutter, 20px); width: var(--dvn-marker-gutter, 20px); height: calc(1lh * max(1, var(--dvn-marker-ratio, 1))); font-size: 14px; line-height: inherit; margin: 0; background: transparent; }',
    // 原生 checkbox 是固定 13px 的替换元素，不跟随 font-size；宽高保持默认，
    // 只由 label 的居中布局决定位置。
    'ul[data-type="taskList"] > li > label > input[type="checkbox"] { margin: 0; width: 13px; height: 13px; accent-color: var(--highlightColor, #007AFF); }',
    'ul[data-type="taskList"] > li > div { flex: 1 1 auto; min-width: 0; }',
    'ul[data-type="taskList"] > li > div > p { margin: 0; }',
    'ul[data-type="taskList"] > li[data-checked="true"] > div > p:first-child { opacity: 0.55; }',
  ].join('\n')
  document.head.appendChild(style)
}

// ---------------------------------------------------------------------------
// 块级样式（标题/正文）与显式字号的关系
// ---------------------------------------------------------------------------

// 选区覆盖到的文本块。nodesBetween 会遍历到语音块、图片等非文本块，
// 这里统一过滤，后续清理只作用于真正的文本块。
function selectedTextblocks(state) {
  const blocks = []
  const { from, to } = state.selection
  state.doc.nodesBetween(from, to, (node, pos) => {
    if (!node.isTextblock) return true
    blocks.push({ node, pos })
    return false
  })
  if (blocks.length > 0) return blocks

  // 折叠光标停在文档末尾等边界位置时 nodesBetween 取不到所在文本块，用父节点兜底。
  const $from = state.selection.$from
  for (let depth = $from.depth; depth > 0; depth -= 1) {
    const node = $from.node(depth)
    if (!node.isTextblock) continue
    blocks.push({ node, pos: $from.before(depth) })
    break
  }
  return blocks
}

// 整块文字使用同一个显式字号时返回该字号；存在没有显式字号的文字，
// 或同一块内出现多种字号时返回 null（视为用户刻意设置的局部字号）。
function uniformBlockFontSize(node, fontSizeType) {
  let hasText = false
  let value = null
  let mixed = false
  node.descendants((child) => {
    if (!child.isText) return
    const mark = fontSizeType.isInSet(child.marks)
    const current = mark ? String(mark.attrs?.fontSize || '') : ''
    if (!hasText) {
      hasText = true
      value = current
      return
    }
    if (!mixed && current !== value) mixed = true
  })
  return hasText && !mixed && value ? value : null
}

// 标题是带固定字号比例的块级样式。若整段文字仍带着同一个显式字号，
// 行内 font-size 会盖住标题自身的字号，视觉上等同于“点了标题没生效”，
// 因此这类整块显式字号要随标题样式一起清掉；局部字号继续保留。
function clearUniformBlockFontSize(tr, state, level) {
  const headingType = state.schema.nodes.heading
  const fontSizeType = state.schema.marks.fontSize
  if (!headingType || !fontSizeType) return
  for (const { node, pos } of selectedTextblocks(state)) {
    if (node.type !== headingType || Number(node.attrs?.level) !== level) continue
    if (!uniformBlockFontSize(node, fontSizeType)) continue
    tr.removeMark(pos + 1, pos + node.nodeSize - 1, fontSizeType)
  }
}

// 标题下拉是块级样式入口：无论转换前文字带了什么字体，应用标题级别
// （或切回正文）时都清掉显式 font-family，让文字重新继承宿主 initFont
// 下发的默认字体。这里清理被选中的完整文本块，避免只清选区后同一标题
// 的后半段仍保留旧字体。
function clearAppliedBlockFontFamily(tr, state, blockTypeName, level = null) {
  const blockType = state.schema.nodes[blockTypeName]
  const fontFamilyType = state.schema.marks.fontFamily
  if (!blockType || !fontFamilyType) return
  for (const { node, pos } of selectedTextblocks(state)) {
    if (node.type !== blockType) continue
    if (blockTypeName === 'heading' && Number(node.attrs?.level) !== level) continue
    tr.removeMark(pos + 1, pos + node.nodeSize - 1, fontFamilyType)
  }
  const storedMarks = state.storedMarks ?? tr.storedMarks
  if (storedMarks?.some((mark) => mark.type === fontFamilyType)) {
    tr.setStoredMarks(storedMarks.filter((mark) => mark.type !== fontFamilyType))
  }
}

function selectedTextRange(selection) {
  if (selection.empty || (!(selection instanceof TextSelection) && !(selection instanceof AllSelection))) return null
  return { from: selection.from, to: selection.to }
}

function textSelectionFromRange(doc, range) {
  const max = doc.content.size
  const from = Math.max(0, Math.min(range.from, max))
  const to = Math.max(0, Math.min(range.to, max))
  return TextSelection.between(doc.resolve(from), doc.resolve(to))
}

function applyMarkColor(editor, markName, attrName, value) {
  const { selection } = editor.state
  const textSelection = selectedTextRange(selection)
  const chain = editor.chain().focus().setMark(markName, { [attrName]: value })

  // 包含语音块/图片的跨块文本选区经过 setMark 后可能被折叠到资源节点上。
  // 恢复有效文本范围，避免用户接着点标题/列表时命令因 NodeSelection 失效。
  if (textSelection) {
    chain.command(({ tr }) => {
      tr.setSelection(textSelectionFromRange(tr.doc, textSelection))
      return true
    })
  }
  chain.run()
}

function clearMarkColor(editor, markName) {
  editor.chain().focus().unsetMark(markName).run()
}

function buildColorPicker(editor, kind, colors, apply, clear, readActive, onOpen) {
  // kind: 'foreColor' | 'backColor'
  const wrapper = createEl('span', { class: 'tiptap-color-picker' })
  const title = kind === 'foreColor' ? '文字颜色' : '背景颜色'
  let currentColor = ''

  const icon = createEl('span', { class: 'tiptap-color-icon' }, [
    createSvgIcon(kind === 'foreColor' ? 'textColor' : 'marker'),
  ])
  const arrow = createEl('span', { class: 'tiptap-color-arrow', 'aria-hidden': 'true' }, [
    createSvgIcon('richtextArrow'),
  ])
  const applyButton = createEl('button', {
    type: 'button',
    class: 'tiptap-color-button tiptap-color-apply-button',
    'data-format': kind,
    'data-tooltip': title,
    'aria-label': title,
  }, [icon])
  const menuButton = createEl('button', {
    type: 'button',
    class: 'tiptap-color-menu-button',
    'data-color-menu': kind,
    'data-tooltip': '更多颜色',
    'aria-label': '更多颜色',
  }, [arrow])
  menuButton.setAttribute('aria-haspopup', 'true')
  menuButton.setAttribute('aria-expanded', 'false')

  const panel = createEl('div', {
    class: 'tiptap-color-panel',
    'data-panel': kind,
    role: 'menu',
    'aria-label': kind === 'foreColor' ? '文字颜色' : '背景颜色',
  })

  function openPanel() {
    onOpen?.(wrapper)
    panel.style.display = 'grid'
    wrapper.classList.add('is-open')
    menuButton.setAttribute('aria-expanded', 'true')
    positionPanel()
  }

  function resetPanelPosition() {
    panel.style.position = ''
    panel.style.left = ''
    panel.style.top = ''
    panel.style.right = ''
  }

  function positionPanel() {
    const overflow = wrapper.closest('.tiptap-overflow-panel')
    // 普通工具栏由 picker 自身定位；折叠工具栏则由 overflow 面板定位。
    // overflow 面板的 backdrop-filter 会为 fixed 子元素创建 containing
    // block，因此两个场景都统一使用相对定位根节点的 absolute 坐标。
    const positioningRoot = overflow?.classList.contains('is-open') ? overflow : wrapper
    const margin = 6
    const viewportWidth = window.innerWidth || document.documentElement.clientWidth || 0
    const viewportHeight = window.innerHeight || document.documentElement.clientHeight || 0
    const triggerRect = wrapper.getBoundingClientRect()
    const rootRect = positioningRoot.getBoundingClientRect()
    const panelWidth = panel.offsetWidth || 153
    const panelHeight = panel.offsetHeight || 0
    // 保持原有的 -12px 左对齐视觉；靠近视口右侧时，将整个面板回收至可见范围。
    let viewportLeft = triggerRect.left - 12
    let viewportTop = triggerRect.bottom + 4

    if (viewportWidth) {
      viewportLeft = Math.max(margin, Math.min(viewportLeft, viewportWidth - panelWidth - margin))
    }
    if (viewportHeight && viewportTop + panelHeight > viewportHeight - margin && triggerRect.top - panelHeight - 4 >= margin) {
      viewportTop = triggerRect.top - panelHeight - 4
    }

    panel.style.position = 'absolute'
    panel.style.left = `${Math.round(viewportLeft - rootRect.left)}px`
    panel.style.top = `${Math.round(viewportTop - rootRect.top)}px`
    panel.style.right = 'auto'
  }

  function closePanel() {
    panel.style.display = 'none'
    wrapper.classList.remove('is-open')
    menuButton.setAttribute('aria-expanded', 'false')
    resetPanelPosition()
  }

  function togglePanel() {
    if (panel.style.display === 'none' || !panel.style.display) {
      openPanel()
    } else {
      closePanel()
    }
  }

  function setCurrentColor(color, mixed = false) {
    // 颜色指示条跟随当前输入上下文；无颜色时恢复 Summernote 的默认值。
    currentColor = color || ''
    wrapper.classList.toggle('is-mixed', Boolean(mixed))
    applyButton.style.setProperty('--dvn-current-color', currentColor || (kind === 'foreColor' ? DEFAULT_FORE_COLOR : DEFAULT_BACK_COLOR))
  }

  applyButton.addEventListener('click', (event) => {
    event.stopPropagation()
    if (!currentColor || currentColor === 'transparent') {
      clear(editor)
    } else {
      apply(editor, currentColor)
    }
    closePanel()
  })

  menuButton.addEventListener('click', (event) => {
    event.stopPropagation()
    togglePanel()
  })

  function appendColorCells(nextColors) {
    // 颜色网格。背景色第一个 transparent 色块与 Summernote 一致，用于清除背景色，
    // 不写入运行态 highlight mark，避免导出/保存时留下无意义的透明背景字段。
    panel.replaceChildren()
    for (const row of nextColors) {
      for (const color of row) {
        const colorLabel = color === 'transparent' ? '透明' : color
        const cell = createEl('button', {
          type: 'button',
          'data-color': color,
          'data-kind': kind,
          'data-tooltip': colorLabel,
          role: 'menuitemradio',
          'aria-label': colorLabel,
          'aria-pressed': 'false',
          style: `background: ${color};`,
        })
        cell.addEventListener('click', () => {
          if (color === 'transparent') {
            clear(editor)
          } else {
            apply(editor, color)
          }
          closePanel()
        })
        panel.appendChild(cell)
      }
    }
  }

  function setColors(nextColors) {
    appendColorCells(Array.isArray(nextColors) ? nextColors : [])
  }

  wrapper.appendChild(applyButton)
  wrapper.appendChild(menuButton)
  wrapper.appendChild(panel)

  setColors(colors)
  setCurrentColor('')
  return { wrapper, toggle: menuButton, applyButton, menuButton, panel, openPanel, closePanel, setCurrentColor, setColors, readActive }
}

function createStyledSelect({ control, title, options, onChange, onOpen }) {
  const wrapper = createEl('span', {
    class: `tiptap-select-wrap tiptap-select-${control}`,
    'data-control': control,
  })
  const select = createEl('select', {
    class: 'tiptap-native-select',
    'data-control': control,
    title,
    'aria-label': title,
  })
  const button = createEl('button', {
    type: 'button',
    class: 'tiptap-select-button',
    'aria-haspopup': 'listbox',
    'aria-expanded': 'false',
    'aria-label': title,
  })
  const menu = createEl('div', {
    class: 'tiptap-select-menu',
    role: 'listbox',
    'data-control': control,
  })
  const label = createEl('span', { class: 'tiptap-select-label' })
  const arrow = createSvgIcon('richtextArrow')
  arrow.classList.add('tiptap-select-arrow')
  button.appendChild(label)
  button.appendChild(arrow)
  let currentOptions = []
  let labelOverride = ''
  let hasLabelOverride = false
  let mixedLayoutLabel = ''
  let mixedState = false

  function close() {
    wrapper.classList.remove('is-open')
    button.setAttribute('aria-expanded', 'false')
  }

  function sync() {
    const blankLabelState = mixedState && Boolean(mixedLayoutLabel) && !labelOverride
    wrapper.classList.toggle('is-mixed', mixedState)
    wrapper.classList.toggle('is-layout-blank', blankLabelState)
    let displayLabel = hasLabelOverride ? labelOverride : (currentOptions.find((option) => option.value === select.value)?.label || currentOptions[0]?.label || '')
    if (blankLabelState) {
      // 混合态显示为空，但保留默认选项文字作为隐藏布局文本，使控件宽度和
      // 箭头位置与默认态一致。用真实文本而不是伪元素，避免 WebEngine 把
      // 隐藏占位文字渲染成可见的省略号“...”。
      displayLabel = mixedLayoutLabel
      label.setAttribute('aria-hidden', 'true')
    } else {
      label.removeAttribute('aria-hidden')
    }
    label.textContent = displayLabel
    for (const item of menu.querySelectorAll('.tiptap-select-option')) {
      const active = !mixedState && item.getAttribute('data-value') === select.value
      item.setAttribute('aria-selected', active ? 'true' : 'false')
    }
  }

  function setOptions(nextOptions) {
    currentOptions = Array.isArray(nextOptions) ? nextOptions : []
    labelOverride = ''
    hasLabelOverride = false
    mixedLayoutLabel = ''
    mixedState = false
    select.replaceChildren()
    menu.replaceChildren()
    for (const option of currentOptions) {
      const nativeOption = createEl('option', { value: option.value }, option.label)
      select.appendChild(nativeOption)

      const check = createEl('span', { class: 'tiptap-select-check', 'aria-hidden': 'true' })
      check.style.setProperty('--tiptap-select-check-mask', `url("${checkIconUrl}")`)
      const item = createEl('button', {
        type: 'button',
        class: 'tiptap-select-option',
        role: 'option',
        'data-value': option.value,
        'aria-selected': 'false',
      }, [
        check,
        createEl('span', { class: 'tiptap-select-option-label' }, option.label),
      ])
      if (option.style && typeof option.style === 'object') {
        Object.assign(item.style, option.style)
      }
      item.addEventListener('click', (event) => {
        event.stopPropagation()
        select.value = option.value
        select.dispatchEvent(new Event('change', { bubbles: true }))
        close()
      })
      menu.appendChild(item)
    }
    if (!currentOptions.some((option) => option.value === select.value)) {
      select.value = currentOptions[0]?.value || ''
    }
    sync()
  }

  button.addEventListener('click', (event) => {
    event.stopPropagation()
    const open = !wrapper.classList.contains('is-open')
    if (open) onOpen?.(wrapper)
    wrapper.classList.toggle('is-open', open)
    button.setAttribute('aria-expanded', open ? 'true' : 'false')
  })
  select.addEventListener('change', () => {
    labelOverride = ''
    hasLabelOverride = false
    mixedLayoutLabel = ''
    mixedState = false
    sync()
    onChange?.(select.value)
  })

  wrapper.appendChild(select)
  wrapper.appendChild(button)
  wrapper.appendChild(menu)
  setOptions(options)
  return {
    wrapper,
    select,
    button,
    menu,
    close,
    setOptions,
    setValue(value, options = {}) {
      mixedState = Boolean(options.mixed)
      hasLabelOverride = options.label !== undefined
      labelOverride = hasLabelOverride ? String(options.label) : ''
      mixedLayoutLabel = options.layoutLabel !== undefined ? String(options.layoutLabel) : ''
      select.value = value
      // 粘贴内容可能带有宿主字体列表里没有的字体。按钮仍应显示真实字体名，
      // 不能因为 select 中找不到对应 option 就回落显示成列表第一项。
      if (!mixedState && value && !currentOptions.some((option) => option.value === value)) {
        labelOverride = labelOverride || String(value)
        hasLabelOverride = true
        mixedState = true
      }
      sync()
    },
  }
}

function currentToolbarTheme() {
  return document.documentElement.dataset.dvnTheme === 'dark' ? 'dark' : 'light'
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

  let closeStyleSelects = () => {}
  let effectiveDefaultFontFamily = ''

  const headingControl = createStyledSelect({
    control: 'heading',
    title: '标题',
    onOpen: (current) => closeStyleSelects(current),
    options: HEADING_OPTIONS.map(({ value, label }) => ({ value, label })),
    onChange(value) {
      const level = value === 'p' ? null : Number(value)
      const { selection } = editor.state
      const textSelection = selectedTextRange(selection)
      const chain = editor.chain().focus()
      if (textSelection) {
        // Ctrl+A 产生 AllSelection。块级转换在这种选区下容易把文档边界
        // 也算进去；先规范成文本选区，转换后再恢复用户原来的选择范围。
        chain.command(({ tr }) => {
          tr.setSelection(textSelectionFromRange(tr.doc, textSelection))
          return true
        })
      }

      if (value === 'p') {
        chain.setParagraph()
      } else {
        // 选择标题后，标题自身的默认粗体就是当前视觉状态；必须先清掉
        // 手动粗体开关遗留状态，否则切换标题等级的事务同步会重新写入
        // fontWeight: normal，导致标题看起来不再自动加粗。
        chain
          .unsetMark('fontWeight', { extendEmptyMarkRange: true })
          // 下拉表达的是“设为该标题级别”，不是切换开关；重复选择当前级别
          // 也必须保持标题，不能像 toggleHeading 那样降回正文。
          .setHeading({ level })
      }

      chain
        // 跨资源节点时 toggleHeading 可能将选区落到语音块/图片；转换后再恢复
        // 有效文本范围，确保字体、字号清理和工具栏状态作用于文本块。
        .command(({ tr }) => {
          if (textSelection) tr.setSelection(textSelectionFromRange(tr.doc, textSelection))
          return true
        })
        // 同一事务里清理字族和整段显式字号，保证一次撤销即可回到切换前状态。
        .command(({ tr, state }) => {
          clearAppliedBlockFontFamily(tr, state, value === 'p' ? 'paragraph' : 'heading', level)
          if (value !== 'p') clearUniformBlockFontSize(tr, state, level)
          return true
        })
        .run()
      syncActiveStates()
    },
  })
  styleGroup.appendChild(headingControl.wrapper)

  const fontControl = createStyledSelect({
    control: 'fontFamily',
    title: '字体',
    onOpen: (current) => closeStyleSelects(current),
    options: [{ value: '', label: '字体' }],
    onChange(value) {
      if (!value) {
        clearMarkColor(editor, 'fontFamily')
      } else {
        applyMarkColor(editor, 'fontFamily', 'fontFamily', value)
      }
    },
  })
  styleGroup.appendChild(fontControl.wrapper)

  const sizeControl = createStyledSelect({
    control: 'fontSize',
    title: '字号',
    onOpen: (current) => closeStyleSelects(current),
    // Keep the size menu as a plain numeric list. The paragraph default is
    // 14px, so it is a normal option rather than a special first row.
    options: FONT_SIZES.map((size) => ({ value: size, label: size })),
    onChange(value) {
      if (value) applyMarkColor(editor, 'fontSize', 'fontSize', toPxSize(value))
    },
  })
  styleGroup.appendChild(sizeControl.wrapper)

  const styleControls = [headingControl, fontControl, sizeControl]
  closeStyleSelects = (except) => {
    for (const control of styleControls) {
      if (control.wrapper !== except) control.close()
    }
  }
  toolbar.appendChild(styleGroup)

  function isInlineFormatActive(format) {
    if (format === 'bold') return isBoldContextActive(editor.state)
    return isUniformInlineMarkActive(editor.state, format)
  }

  function applyStoredInlineFormat(format, active) {
    if (format === 'bold') {
      // 标题的粗体来自块级样式，正文的粗体来自 bold mark；两者混选时不能
      // 只走其中一种命令。关闭时只给标题文字写 fontWeight: normal，正文则
      // 清掉 bold/fontWeight；开启时先清 normal 覆盖，再给整个选区补 bold。
      const selection = editor.state.selection
      let chain = editor.chain().focus().unsetMark('bold').unsetMark('fontWeight')
      if (active) return chain.setMark('bold').run()

      if (selection.empty) {
        if (nearestTextblockAtSelection(editor.state)?.node?.type?.name === 'heading') {
          chain = chain.setMark('fontWeight', { fontWeight: 'normal' })
        }
        return chain.run()
      }

      return chain.command(({ tr, state }) => {
        const fontWeightType = state.schema.marks.fontWeight
        if (!fontWeightType) return true
        for (const run of selectedTextRuns(state)) {
          if (nearestTextblockAt(state.doc, run.pos)?.node?.type?.name !== 'heading') continue
          tr.addMark(run.from, run.to, fontWeightType.create({ fontWeight: 'normal' }))
        }
        return true
      }).run()
    }

    let chain = editor.chain().focus()
    if (active) {
      return chain.setMark(format).run()
    }
    return chain.unsetMark(format).run()
  }

  function applyToolbarToggle(format) {
    if (!INLINE_TOGGLE_FORMATS.has(format)) {
      applyToggle(editor, format)
      return
    }

    // 按钮状态和点击取反都以当前真实上下文为准：有选区时是选区格式，
    // 折叠光标时是光标位置 marks 或 ProseMirror 的 storedMarks。手动开启的
    // 待输入格式只作用于当前位置，光标移到别处后自动回到目标位置的真实格式。
    const nextActive = !isInlineFormatActive(format)
    applyStoredInlineFormat(format, nextActive)
    syncActiveStates()
  }
  const styleToggleSeparator = createSeparator()
  toolbar.appendChild(styleToggleSeparator)

  // 开关式按钮区：粗体 / 斜体 / 下划线 / 删除线
  const toggleGroup = createEl('span', { class: 'tiptap-toolbar-group tiptap-toggle-group' })
  for (const { format, icon, title, className } of TOGGLE_BUTTONS) {
    const btn = createToolbarButton({ format, title, className, children: createSvgIcon(icon) })
    btn.setAttribute('aria-pressed', 'false')
    btn.addEventListener('click', () => applyToolbarToggle(format))
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
    (pickerWrapper) => {
      if (!overflowPanel.contains(pickerWrapper)) closeOverflowPanel()
      backPicker.closePanel()
      headingControl.close()
      fontControl.close()
      sizeControl.close()
    },
  )
  colorGroup.appendChild(forePicker.wrapper)

  const backPicker = buildColorPicker(
    editor,
    'backColor',
    BACK_COLORS,
    (editor, color) => applyHighlight(editor, color),
    (editor) => clearHighlight(editor),
    () => editor.getAttributes('highlight').color,
    (pickerWrapper) => {
      if (!overflowPanel.contains(pickerWrapper)) closeOverflowPanel()
      forePicker.closePanel()
      headingControl.close()
      fontControl.close()
      sizeControl.close()
    },
  )
  colorGroup.appendChild(backPicker.wrapper)

  function syncColorPalettesForTheme(theme = currentToolbarTheme()) {
    forePicker.setColors(colorPaletteForTheme('foreColor', theme))
    backPicker.setColors(colorPaletteForTheme('backColor', theme))
  }

  syncColorPalettesForTheme()
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
  // createToolbarButton 已统一改为按钮自身锚定的 data-tooltip；这里只补充
  // “更多”按钮的面板语义。
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
  const orderedToolbarUnits = [
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
  const overflowCollapseUnits = [...orderedToolbarUnits].reverse()
  const overflowSet = new Set()
  let pressingButton = null

  function clearPressingButton() {
    pressingButton?.classList.remove('is-pressing')
    pressingButton = null
  }

  function onToolbarPointerDown(event) {
    const button = event.target?.closest?.('button')
    if (!button || !toolbar.contains(button) || button.disabled) return
    clearPressingButton()
    pressingButton = button
    button.classList.add('is-pressing')
  }

  toolbar.addEventListener('pointerdown', onToolbarPointerDown, true)
  for (const eventName of TOOLBAR_PRESS_CLEAR_EVENTS) {
    toolbar.addEventListener(eventName, clearPressingButton, true)
  }
  window.addEventListener('pointerup', clearPressingButton, true)
  window.addEventListener('pointercancel', clearPressingButton, true)

  function removeAllChildren(node) {
    while (node.firstChild) node.removeChild(node.firstChild)
  }

  function closeOverflowPanel() {
    overflowPanel.classList.remove('is-open')
    moreBtn.setAttribute('aria-expanded', 'false')
  }

  function clampOverflowPanelToViewport() {
    if (!overflowPanel.classList.contains('is-open')) return

    const margin = 6
    const viewportWidth = window.innerWidth || document.documentElement.clientWidth || host.clientWidth || 0
    if (!viewportWidth) return

    overflowPanel.style.maxWidth = `${Math.max(0, viewportWidth - margin * 2)}px`

    const buttonRect = moreBtn.getBoundingClientRect()
    const panelRect = overflowPanel.getBoundingClientRect()
    const panelWidth = Math.min(panelRect.width || 0, Math.max(0, viewportWidth - margin * 2))
    const left = Math.max(margin, Math.min(buttonRect.right - panelWidth, viewportWidth - panelWidth - margin))
    const top = Math.max(margin, buttonRect.bottom + 6)

    overflowPanel.style.left = `${Math.round(left)}px`
    overflowPanel.style.top = `${Math.round(top)}px`
  }

  function toggleOverflowPanel() {
    const open = !overflowPanel.classList.contains('is-open')
    overflowPanel.classList.toggle('is-open', open)
    moreBtn.setAttribute('aria-expanded', open ? 'true' : 'false')
    if (open) {
      window.requestAnimationFrame(clampOverflowPanelToViewport)
    }
    forePicker.closePanel()
    backPicker.closePanel()
    headingControl.close()
    fontControl.close()
    sizeControl.close()
  }

  function unitsInGroup(group, inOverflow) {
    return orderedToolbarUnits.filter((unit) => unit.group === group && overflowSet.has(unit) === inOverflow)
  }

  function appendGroupWithSeparator(parent, separator, group, units) {
    if (!units.length) return false
    parent.appendChild(separator)
    parent.appendChild(group)
    return true
  }

  function renderOverflowLayout() {
    closeOverflowPanel()
    forePicker.closePanel()
    backPicker.closePanel()
    toolbar.classList.toggle('is-overflowing', overflowSet.size > 0)

    removeAllChildren(toolbar)
    for (const group of Object.values(mainGroups)) removeAllChildren(group)
    for (const group of Object.values(overflowGroups)) removeAllChildren(group)
    removeAllChildren(overflowPanel)

    toolbar.appendChild(styleGroup)

    for (const unit of orderedToolbarUnits) {
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
    closeOverflowPanel()
    const hostWidth = host.clientWidth || document.documentElement.clientWidth || window.innerWidth || 0
    if (!hostWidth) return
    // 顶部工具栏只在左侧保留 10px，与宿主实际布局一致；右侧不再额外
    // 扣除 10px，避免正常窗口下因这段虚拟边距过早把末尾资源按钮折叠。
    // Summernote 的 385px 只用于 airPopover 定位参考，不能限制当前常驻顶部工具栏。
    const maxToolbarWidth = Math.max(0, hostWidth - TOOLBAR_LEFT_MARGIN)

    overflowSet.clear()
    renderOverflowLayout()
    if (measuredToolbarWidth() <= maxToolbarWidth + 1) return

    // Summernote 工具栏按视觉顺序从后往前折叠，不能跳过末尾资源区去隐藏中间按钮。
    // 渲染时仍使用 orderedToolbarUnits，保证主工具栏与更多面板内的视觉顺序稳定。
    for (const unit of overflowCollapseUnits) {
      overflowSet.add(unit)
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
      headingControl.close()
      fontControl.close()
      sizeControl.close()
      closeOverflowPanel()
    }
  }
  document.addEventListener('click', onOutsideClick)

  function onThemeApplied(event) {
    syncColorPalettesForTheme(event?.detail?.theme)
    syncActiveStates()
  }
  window.addEventListener('dvn-theme-applied', onThemeApplied)

  // 激活态同步
  function setPressed(btn, active) {
    btn.setAttribute('aria-pressed', active ? 'true' : 'false')
    btn.classList.toggle('is-active', active)
  }

  function syncActiveStates() {
    // 开关式格式按钮始终表达当前编辑器上下文：
    // - 显式选区：整个选区都带该格式才激活；
    // - 折叠光标：读取光标位置 marks / storedMarks；
    // - 手动开启待输入格式后 typing 使用该格式，光标移动则重新读取新位置。
    const hasEditorContext = Boolean(editor.isFocused)

    for (const format of ['bold', 'italic', 'underline', 'strike']) {
      const btn = buttons[format]
      setPressed(btn, hasEditorContext && isInlineFormatActive(format))
    }
    setPressed(buttons.blockquote, hasEditorContext && editor.isActive('blockquote'))

    // 标题下拉表达“当前输入位置的块级样式”。显式选区跨越标题和正文、
    // 或跨不同标题级别时直接留空，不能取选区第一段的标题冒充整体状态。
    const headingContext = hasEditorContext
      ? textblockStyleContext(editor.state)
      : { value: 'p', mixed: false }
    if (headingContext.mixed) {
      headingControl.setValue('', { mixed: true, label: '', layoutLabel: '正文' })
    } else {
      headingControl.setValue(headingContext.value || 'p')
    }

    // 字体下拉同样读取整个选区。没有显式字体 mark 的文字按宿主默认字体
    // 计算，因此“默认字体 + 显式设置成同一默认字体”不会被误报为混合。
    const fontContext = hasEditorContext
      ? uniformMarkContext(editor.state, 'fontFamily', 'fontFamily', value => value || effectiveDefaultFontFamily)
      : { value: '', normalizedValue: '', mixed: false }
    if (fontContext.mixed) {
      fontControl.setValue('', { mixed: true, label: '', layoutLabel: effectiveDefaultFontFamily || '字体' })
    } else {
      const currentFont = fontContext.normalizedValue || ''
      fontControl.setValue(currentFont === effectiveDefaultFontFamily ? '' : currentFont)
    }

    // 字号按“实际视觉字号”比较：显式 font-size 优先，否则使用标题级别
    // 自带字号，正文回落到 14px。这样标题与正文、不同级别标题混选时
    // 不会显示选区里第一个文字的字号。
    const sizeContext = hasEditorContext
      ? effectiveFontSizeContext(editor.state)
      : { value: '14', mixed: false }
    if (sizeContext.mixed) {
      sizeControl.setValue('', { mixed: true, label: '', layoutLabel: '14' })
    } else {
      sizeControl.setValue(sizeContext.value || '14')
    }

    // 颜色同样按整个选区判断。混合颜色时不选中调色板里的任何一格，
    // 避免把选区中第一个颜色误当成整体已激活的颜色。
    const foreContext = hasEditorContext
      ? uniformMarkContext(editor.state, 'color', 'color', normalizeCssColor)
      : { value: '', mixed: false }
    const backContext = hasEditorContext
      ? uniformMarkContext(editor.state, 'highlight', 'color', value => normalizeCssColor(value || 'transparent'))
      : { value: '', mixed: false }
    const foreColor = foreContext.mixed ? '' : foreContext.value
    const backColor = backContext.mixed ? '' : (backContext.value || 'transparent')
    forePicker.setCurrentColor(foreColor, foreContext.mixed)
    backPicker.setCurrentColor(backColor, backContext.mixed)
    syncColorCells(forePicker.panel, foreColor, foreContext.mixed)
    syncColorCells(backPicker.panel, backColor, backContext.mixed)

    // 列表/待办按钮表达当前输入位置的列表类型。用户点击序列号/序列点后
    // 会生成折叠光标所在的列表上下文，按钮需要立即进入 active 状态。
    // 仍保留焦点门禁，避免页面加载时仅因历史文档中存在列表而误高亮。
    const currentListType = hasEditorContext ? activeListType(editor) : null
    for (const format of ['bulletList', 'orderedList', 'taskList']) {
      const btn = listButtons[format]
      setPressed(btn, currentListType === format)
    }
    listButtons.indentList.disabled = !hasEditorContext || !canIndentActiveListItem(editor)
    listButtons.outdentList.disabled = !hasEditorContext || !canOutdentActiveListItem(editor)
  }

  function syncColorCells(panel, activeColor, mixed = false) {
    const cells = Array.from(panel.querySelectorAll('button[data-color]'))
    const hasActiveColor = Boolean(activeColor)
    for (const [index, cell] of cells.entries()) {
      const color = cell.getAttribute('data-color')
      // When the current context has no explicit color, keep the first palette
      // item selected as the default option.  This matches the palette's
      // visual default without applying a mark to the document.
      const isDefault = !mixed && !hasActiveColor && index === 0
      const isActive = (hasActiveColor && sameCssColor(color, activeColor)) || isDefault
      cell.setAttribute('aria-pressed', !mixed && isActive ? 'true' : 'false')
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
  window.addEventListener('scroll', clampOverflowPanelToViewport, true)

  return {
    setOnPickImage(fn) {
      onPickImage = typeof fn === 'function' ? fn : null
    },
    setOnRecordVoice(fn) {
      onRecordVoice = typeof fn === 'function' ? fn : null
    },
    setResourceButtonsEnabled,
    setFontList(fonts, defaultFont) {
      const normalized = Array.isArray(fonts)
        ? fonts.filter((name, index, array) => typeof name === 'string' && name.trim() && array.indexOf(name) === index)
        : []
      const list = normalized.length > 0 ? [...normalized] : []
      if (defaultFont && !list.includes(defaultFont)) {
        list.unshift(defaultFont)
      }
      const effectiveDefaultFont = defaultFont || list[0] || ''
      effectiveDefaultFontFamily = effectiveDefaultFont
      if (effectiveDefaultFont) {
        document.body.style.fontFamily = effectiveDefaultFont
        document.documentElement.style.setProperty('--dvn-editor-default-font', effectiveDefaultFont)
      }

      // 与 Summernote 保持一致：下拉项按各自字体渲染；默认项仍代表当前系统字体。
      fontControl.setOptions([
        { value: '', label: effectiveDefaultFont || '字体', style: effectiveDefaultFont ? { fontFamily: effectiveDefaultFont } : {} },
        ...list.map((name) => ({ value: name, label: name, style: { fontFamily: name } })),
      ])
      syncActiveStates()
      updateOverflowMode()
    },
    destroy() {
      document.removeEventListener('click', onOutsideClick)
      window.removeEventListener('resize', updateOverflowMode)
      window.removeEventListener('dvn-theme-applied', onThemeApplied)
      toolbar.removeEventListener('pointerdown', onToolbarPointerDown, true)
      for (const eventName of TOOLBAR_PRESS_CLEAR_EVENTS) {
        toolbar.removeEventListener(eventName, clearPressingButton, true)
      }
      window.removeEventListener('pointerup', clearPressingButton, true)
      window.removeEventListener('pointercancel', clearPressingButton, true)
      clearPressingButton()
      editor.off?.('transaction', syncActiveStates)
      editor.off?.('focus', syncActiveStates)
      editor.off?.('blur', syncActiveStates)
      headingControl.close()
      fontControl.close()
      sizeControl.close()
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
