// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// TTP-023 复制/剪贴板归一。
//
// 现象：全选复制 → 单击取消选中（光标停在行尾）→ 再按一次 Ctrl+C → 粘贴时内容
// 不在光标处，而是在下面另起一段（"新开了一行"）。
//
// 成因（两段叠加）：
// 1. ProseMirror 的复制是"自己序列化剪贴板"：copy 事件里写入
//    `text/html = <p data-pm-slice="0 0 []">abc</p>`。粘贴回来时
//    parseFromClipboard 读到 `data-pm-slice`（prosemirror-view/dist/index.cjs:2750）
//    会把这段当成"完整块"还原，于是把当前段落切开 → 另起一段；
//    反过来没有 `data-pm-slice` 时走 Slice.maxOpen（:2752），会内联合并到光标处。
// 2. 光标没有选区时 ProseMirror 的 copy 处理器直接 return，剪贴板保持不变
//    → 第二次 Ctrl+C 复制到的其实是上一次的残留内容。
//
// 修复：在页面级 copy/cut 事件（晚于 ProseMirror 自己的处理器）统一改写剪贴板：
// - 纯文本用文档文本（块之间用 \n），不再依赖 ProseMirror 的块级序列化；
// - HTML 保留 ProseMirror 的序列化、只去掉 `data-pm-slice`，这样粘贴回本应用是
//   内联合并（不另起段），粘贴到外部应用仍然保留加粗/颜色/列表等格式；
// - 光标无选区时（Ctrl+C）按 VS Code 习惯复制光标所在块的文本，避免复制到残留内容；
//   该块为空则显式写入空文本，等于清掉上一次的剪贴板内容；
// - 语音转写（.translateText）有自己的复制通道，图片/语音等非文本块保持原通道。

const PM_SLICE_ATTRIBUTE = /\sdata-pm-slice=(?:"[^"]*"|'[^']*')/g

function hasDom() {
  return typeof window !== 'undefined' && typeof document !== 'undefined'
}

export function stripPmSliceFromHtml(html) {
  if (typeof html !== 'string' || html === '') return ''
  return html.replace(PM_SLICE_ATTRIBUTE, '')
}

export function selectionPlainText(state) {
  if (!state?.selection || !state.doc) return ''
  const { from, to } = state.selection
  return state.doc.textBetween(from, to, '\n')
}

// 光标所在块（段落/标题/列表项里的段落）的纯文本；非文本块返回 null，
// 交给原通道处理（图片、语音块等原子内容不能只取文本）。
export function currentBlockPlainText(state) {
  const selection = state?.selection
  if (!selection || !selection.empty || !state.doc) return null
  const parent = selection.$from?.parent
  if (!parent || !parent.isTextblock) return null
  const start = selection.$from.start()
  const end = selection.$from.end()
  return state.doc.textBetween(start, end, '\n')
}

function belongsToEditor(event, editor) {
  const dom = editor?.view?.dom
  const target = event?.target
  if (!dom || !target || typeof dom.contains !== 'function') return false
  return target === dom || dom.contains(target)
}

function isTranscriptCopy(event) {
  return Boolean(event?.target?.closest?.('.translateText'))
}

function eventClipboard(event) {
  const clipboard = event?.clipboardData
  if (!clipboard || typeof clipboard.setData !== 'function') return null
  return clipboard
}

function readClipboardHtml(clipboard) {
  if (typeof clipboard.getData !== 'function') return ''
  try {
    return clipboard.getData('text/html') || ''
  } catch (_) {
    return ''
  }
}

function readClipboardText(clipboard) {
  if (typeof clipboard.getData !== 'function') return ''
  try {
    return clipboard.getData('text/plain') || ''
  } catch (_) {
    return ''
  }
}

function normalizeClipboardEvent(event, editor, options) {
  const clipboard = eventClipboard(event)
  if (!clipboard || !editor?.state) return false
  if (!belongsToEditor(event, editor) || isTranscriptCopy(event)) return false

  // Ctrl+X：ProseMirror 的处理器已经写好吃剪贴板并删除了选区，state 里已经取不到文本，
  // 此时只做"去掉 data-pm-slice"的归一，纯文本沿用 ProseMirror 写好的内容。
  if (options.type === 'cut') {
    const cutText = readClipboardText(clipboard)
    if (!cutText) return false
    return writeNormalizedClipboard(event, clipboard, cutText)
  }

  const selectedText = selectionPlainText(editor.state)
  if (selectedText) {
    return writeNormalizedClipboard(event, clipboard, selectedText)
  }

  // Ctrl+C 且光标没有选区：按 VS Code 习惯复制光标所在块的文本。
  const blockText = currentBlockPlainText(editor.state)
  if (blockText === null) return false
  if (blockText === '') {
    // 光标所在块为空：显式清掉上一次的剪贴板内容，避免"复制到残留内容"。
    options.clearClipboard?.()
    event.preventDefault()
    return true
  }
  return writeNormalizedClipboard(event, clipboard, blockText)
}

function writeNormalizedClipboard(event, clipboard, plain) {
  const html = stripPmSliceFromHtml(readClipboardHtml(clipboard))
  clipboard.setData('text/plain', plain)
  if (html) {
    clipboard.setData('text/html', html)
  }
  event.preventDefault()
  return true
}

export function normalizeCopyEvent(event, editor, options = {}) {
  return normalizeClipboardEvent(event, editor, { type: 'copy', clearClipboard: options.clearClipboard })
}

export function normalizeCutEvent(event, editor) {
  return normalizeClipboardEvent(event, editor, { type: 'cut' })
}

export function installClipboardCopyNormalizer(editor, options = {}) {
  if (!hasDom() || !editor) return () => {}
  const onCopy = (event) => {
    normalizeClipboardEvent(event, editor, { type: 'copy', clearClipboard: options.clearClipboard })
  }
  const onCut = (event) => {
    normalizeClipboardEvent(event, editor, { type: 'cut' })
  }
  document.addEventListener('copy', onCopy)
  document.addEventListener('cut', onCut)
  return () => {
    document.removeEventListener('copy', onCopy)
    document.removeEventListener('cut', onCut)
  }
}
