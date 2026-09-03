// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 图片交互模块：粘贴策略与图片 DOM 事件委托。

import { TextSelection } from '@tiptap/pm/state'
import imageBlockCss from '../extensions/image-block.css?inline'

// 不为 image 节点实现 NodeView。图片粘贴策略、选中态与上下文菜单
// 统一收敛在运行时交互层，避免节点视图承载过多宿主耦合。

/**
 * 判定单个图片 src 是否为远程地址（应阻止）。
 * 仅允许本地相对路径（images/ 开头）与本地 file:// 地址；其余一律视为远程。
 * @param {string} src
 * @returns {boolean}
 */
export function isRemoteImageSrc(src) {
  if (!src || typeof src !== 'string') return false
  const trimmed = src.trim()
  if (trimmed.length === 0) return false
  // 本地相对路径（笔记资源约定）放行
  if (trimmed.startsWith('images/')) return false
  // 本地 file:// 地址放行
  if (/^file:\/\//i.test(trimmed)) return false
  // http/https、协议相对 // 及其余非本地路径视为远程，阻止
  return true
}

/**
 * 用 DOM 解析器从 HTML 中提取所有 img 节点的 src 属性。
 * 优先使用 DOMParser；不可用时回退到临时容器 innerHTML。
 * @param {string} html
 * @returns {string[]}
 */
export function extractImageSrcs(html) {
  if (!html) return []
  let root
  if (typeof DOMParser !== 'undefined') {
    root = new DOMParser().parseFromString(html, 'text/html')
  } else if (typeof document !== 'undefined') {
    root = document.createElement('div')
    root.innerHTML = html
  } else {
    return []
  }
  const imgs = root.querySelectorAll('img')
  const srcs = []
  for (const img of imgs) {
    const src = img.getAttribute('src')
    if (src != null) srcs.push(src)
  }
  return srcs
}

/**
 * 判定 HTML 中是否含远程图片 src。
 * @param {string} html
 * @returns {boolean}
 */
export function hasRemoteImageInHtml(html) {
  return extractImageSrcs(html).some(isRemoteImageSrc)
}

/**
 * 分析剪贴板内容，决定粘贴动作。
 * - 'saveImage'：剪贴板含本地图片数据（截图 / 复制图片文件），需落盘回插。
 * - 'blockRemote'：HTML 含远程图片 src（http/https/协议相对 // 及其余非本地路径），阻止粘贴。
 * - 'default'：其余情况（笔记内复制/粘贴、纯文本），交由编辑器默认处理，保留节点 attrs。
 * @param {DataTransfer|null} clipboardData
 * @returns {{ action: string, imageItem?: object, html?: string }}
 */
export function analyzePaste(clipboardData) {
  if (!clipboardData) return { action: 'default' }

  // 1. 剪贴板含本地图片数据 → 落盘回插
  const imageItem = findImageItem(clipboardData)
  if (imageItem) {
    return { action: 'saveImage', imageItem }
  }

  // 2. 用 DOM 解析提取 img src，远程图片阻止粘贴
  const html = clipboardData.getData ? clipboardData.getData('text/html') : ''
  if (html && hasRemoteImageInHtml(html)) {
    return { action: 'blockRemote', html }
  }

  // 3. 其余 → 默认（笔记内复制/粘贴保留节点 attrs，不重复落盘）
  return { action: 'default' }
}

function findImageItem(clipboardData) {
  const items = clipboardData.items
  if (!items) return null
  for (const item of items) {
    if (item && item.kind === 'file' && typeof item.type === 'string' && item.type.startsWith('image/')) {
      return item
    }
  }
  return null
}

/**
 * 将剪贴板图片 item 读取为 data URL（base64）。
 * @param {object} imageItem — DataTransferItem（kind=file, type=image/*）
 * @returns {Promise<{ dataUrl: string, name: string }>}
 */
export function readImageItemAsDataUrl(imageItem) {
  return new Promise((resolve, reject) => {
    const file = imageItem.getAsFile()
    if (!file) {
      reject(new Error('image item has no file'))
      return
    }
    const reader = new FileReader()
    reader.onload = () => resolve({ dataUrl: reader.result, name: file.name || '' })
    reader.onerror = () => reject(reader.error)
    reader.readAsDataURL(file)
  })
}

/**
 * 注册粘贴处理：剪贴板图片落盘往返、远程图片阻止、笔记内默认保留 attrs。
 * @param {Editor} editor
 * @param {object} bridge — QWebChannel bridge（需 jsPasteImage 方法）
 * @param {object} [options] — 可选，{ captureInsertionSelection }
 * @returns {() => void} 销毁函数，移除 handlePaste 处理
 */
export function setupImagePaste(editor, bridge, options = {}) {
  if (!editor || !bridge) return () => {}

  // 保存原有的 handlePaste，destroy 时恢复，避免永久关闭默认粘贴
  const previousProps = editor.view.props
  const previousHandlePaste = previousProps.handlePaste

  editor.setOptions({
    editorProps: {
      handlePaste(_view, event) {
        const decision = analyzePaste(event.clipboardData)
        if (decision.action === 'saveImage') {
          event.preventDefault()
          options.captureInsertionSelection?.()
          readImageItemAsDataUrl(decision.imageItem)
            .then(({ dataUrl }) => {
              if (bridge.jsPasteImage) bridge.jsPasteImage(dataUrl)
            })
            .catch((err) => console.error('[tiptap] paste image read failed:', err))
          return true
        }
        if (decision.action === 'blockRemote') {
          event.preventDefault()
          return true
        }
        return false
      },
    },
  })

  return function destroy() {
    editor.setOptions({ editorProps: { handlePaste: previousHandlePaste } })
  }
}

// ---------------------------------------------------------------------------
// 图片 DOM 事件委托：单击选中、双击查看原图、右键交给宿主菜单
// ---------------------------------------------------------------------------

function findImageTarget(dom, target) {
  let node = target
  while (node && node !== dom) {
    if (node.tagName === 'IMG' && node.hasAttribute && node.hasAttribute('data-rel-path')) {
      return node
    }
    node = node.parentNode
  }
  return null
}

function imagePosFromElement(editor, img) {
  const nodeDom = img?.closest?.('[data-dvn-image-node]') || img
  const pos = editor.view.posAtDOM(nodeDom, 0)
  return pos == null || pos < 0 ? null : pos
}

function selectImageNode(editor, img) {
  const pos = imagePosFromElement(editor, img)
  if (pos == null) return false
  return editor.chain().focus().setNodeSelection(pos).run()
}

function cursorAfterImage(editor, imagePos) {
  const imageNode = editor.state.doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return null
  const afterImage = imagePos + imageNode.nodeSize
  const $after = editor.state.doc.resolve(afterImage)
  if ($after.parent.inlineContent) return afterImage
  const nextNode = $after.nodeAfter
  return nextNode?.isTextblock ? afterImage + 1 : null
}

function cursorBeforeImage(editor, imagePos) {
  const imageNode = editor.state.doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return null
  const $before = editor.state.doc.resolve(imagePos)
  return $before.parent.inlineContent ? imagePos : null
}

function dispatchTextCursor(editor, pos) {
  const tr = editor.state.tr.setSelection(TextSelection.create(editor.state.doc, pos)).scrollIntoView()
  editor.view.dispatch(tr)
  editor.view.focus()
  return true
}

function setTextCursorAfterImagePos(editor, imagePos) {
  const imageNode = editor.state.doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return false

  const afterImage = imagePos + imageNode.nodeSize
  let tr = editor.state.tr
  let cursorPos = cursorAfterImage(editor, imagePos)

  // Compatibility for old top-level image nodes: create a following paragraph
  // if the image is not already inside inline paragraph content.
  if (cursorPos == null) {
    const paragraph = editor.state.schema.nodes.paragraph?.create()
    if (!paragraph) return false
    tr = tr.insert(afterImage, paragraph)
    cursorPos = afterImage + 1
  }

  tr = tr.setSelection(TextSelection.create(tr.doc, cursorPos)).scrollIntoView()
  editor.view.dispatch(tr)
  editor.view.focus()
  return true
}

function setTextCursorBeforeImagePos(editor, imagePos) {
  const cursorPos = cursorBeforeImage(editor, imagePos)
  if (cursorPos == null) return false
  return dispatchTextCursor(editor, cursorPos)
}

function setTextCursorAfterImageElement(editor, img) {
  const pos = imagePosFromElement(editor, img)
  return pos == null ? false : setTextCursorAfterImagePos(editor, pos)
}

function isTrailingHardBreak(node) {
  return node?.type?.name === 'hardBreak'
}

function hasInlineContentAfterImage(editor, imagePos) {
  const imageNode = editor.state.doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return false
  const afterImage = imagePos + imageNode.nodeSize
  const $after = editor.state.doc.resolve(afterImage)
  if (!$after.parent.inlineContent || $after.parentOffset >= $after.parent.content.size) return false
  const next = $after.parent.childAfter($after.parentOffset).node
  return !isTrailingHardBreak(next)
}

function shouldHandleTrailingCaretClick(editor, img) {
  const pos = imagePosFromElement(editor, img)
  if (pos == null) return false
  // If text or another inline node already exists after the image, normal
  // ProseMirror hit testing must place the caret inside that content.  Only
  // synthesize a caret for the empty right side of a bare image paragraph.
  return !hasInlineContentAfterImage(editor, pos)
}

function bareImageParagraphInfo(doc, paragraphPos) {
  const paragraph = doc.nodeAt(paragraphPos)
  if (!paragraph || paragraph.type.name !== 'paragraph') return null
  if (paragraph.childCount < 1 || paragraph.childCount > 2) return null
  const child = paragraph.child(0)
  if (child.type.name !== 'image') return null
  if (paragraph.childCount === 2 && !isTrailingHardBreak(paragraph.child(1))) return null
  const beforePos = paragraphPos + 1
  // paragraphPos points before the paragraph node. +1 enters paragraph content;
  // +child.nodeSize places the caret after the inline image and before an
  // optional legacy trailing hardBreak.
  return {
    image: child,
    imagePos: beforePos,
    beforePos,
    afterPos: beforePos + child.nodeSize,
  }
}

function soleImageCursorPosInParagraph(doc, paragraphPos, side = 'after') {
  const info = bareImageParagraphInfo(doc, paragraphPos)
  if (!info) return null
  return side === 'before' ? info.beforePos : info.afterPos
}

function currentBareImageCursor(selection) {
  if (!selection.empty) return null
  const $from = selection.$from
  if ($from.parent.type.name !== 'paragraph') return null
  const paragraphPos = $from.before($from.depth)
  const info = bareImageParagraphInfo($from.doc, paragraphPos)
  if (!info) return null
  if ($from.parentOffset <= 0) return { ...info, paragraphPos, side: 'before' }
  if ($from.parentOffset >= info.image.nodeSize) return { ...info, paragraphPos, side: 'after' }
  return null
}

function adjacentBlockPos($pos, direction) {
  const textblockDepth = $pos.depth
  if (textblockDepth <= 0) return null
  const blockPos = direction > 0 ? $pos.after(textblockDepth) : $pos.before(textblockDepth)
  const $blockBoundary = $pos.doc.resolve(blockPos)
  const node = direction > 0 ? $blockBoundary.nodeAfter : $blockBoundary.nodeBefore
  if (!node) return null
  return direction > 0 ? blockPos : blockPos - node.nodeSize
}

function moveVerticalToAdjacentBareImageParagraph(editor, direction) {
  const { selection, doc } = editor.state
  if (!selection.empty) return false
  const $from = selection.$from
  if (!$from.parent.isTextblock) return false

  const paragraphPos = adjacentBlockPos($from, direction)
  if (paragraphPos == null) return false
  // Browser vertical navigation loses the before/after side on pure image
  // lines in WebEngine.  Preserve the current side when moving image-line to
  // image-line; text lines keep the legacy default of landing after the image.
  const side = currentBareImageCursor(selection)?.side || 'after'
  const cursorPos = soleImageCursorPosInParagraph(doc, paragraphPos, side)
  if (cursorPos == null) return false
  return dispatchTextCursor(editor, cursorPos)
}

function moveHorizontalAroundBareImageParagraph(editor, direction) {
  const current = currentBareImageCursor(editor.state.selection)
  if (!current) return false

  if (direction > 0) {
    if (current.side === 'before') {
      return dispatchTextCursor(editor, current.afterPos)
    }
    const nextParagraphPos = adjacentBlockPos(editor.state.doc.resolve(current.afterPos), 1)
    const nextBefore = nextParagraphPos == null
      ? null
      : soleImageCursorPosInParagraph(editor.state.doc, nextParagraphPos, 'before')
    return nextBefore == null ? false : dispatchTextCursor(editor, nextBefore)
  }

  if (current.side === 'after') {
    return dispatchTextCursor(editor, current.beforePos)
  }
  const prevParagraphPos = adjacentBlockPos(editor.state.doc.resolve(current.beforePos), -1)
  const prevAfter = prevParagraphPos == null
    ? null
    : soleImageCursorPosInParagraph(editor.state.doc, prevParagraphPos, 'after')
  return prevAfter == null ? false : dispatchTextCursor(editor, prevAfter)
}

function editorContentBounds(dom) {
  let rect = null
  try {
    rect = dom.getBoundingClientRect()
  } catch (_) {
    rect = null
  }
  if (!rect || rect.width <= 0) return null

  let paddingLeft = 0
  let paddingRight = 0
  try {
    const style = window.getComputedStyle(dom)
    paddingLeft = Number.parseFloat(style.paddingLeft) || 0
    paddingRight = Number.parseFloat(style.paddingRight) || 0
  } catch (_) {
    paddingLeft = 0
    paddingRight = 0
  }

  return {
    left: rect.left + paddingLeft,
    right: rect.right - paddingRight,
    outerLeft: rect.left,
    outerRight: rect.right,
  }
}

function imageRowBounds(dom, img) {
  const rect = img.getBoundingClientRect()
  const paragraph = img.closest?.('p') || (img.parentElement?.tagName === 'P' ? img.parentElement : null)
  const rowRect = paragraph?.getBoundingClientRect?.() || rect
  const editorBounds = editorContentBounds(dom)

  const rowTop = rowRect.height > 0 ? rowRect.top : rect.top
  const rowBottom = rowRect.height > 0 ? rowRect.bottom : rect.bottom
  const hasRowWidth = rowRect.width > 0
  const contentLeft = editorBounds?.left ?? (hasRowWidth ? rowRect.left : Number.NEGATIVE_INFINITY)
  const contentRight = editorBounds?.right ?? (hasRowWidth ? rowRect.right : Number.POSITIVE_INFINITY)

  return {
    image: rect,
    top: Math.min(rowTop, rect.top),
    bottom: Math.max(rowBottom, rect.bottom),
    left: Math.min(rowRect.left, contentLeft),
    right: Math.max(rowRect.right, contentRight),
    editorRight: editorBounds?.outerRight ?? contentRight,
  }
}

function findImageForTrailingCaretClick(dom, event) {
  if (!event || event.clientX == null || event.clientY == null) return null
  const imgs = dom.querySelectorAll('img[data-rel-path]')
  let candidate = null
  let candidateDistance = Number.POSITIVE_INFINITY
  for (const img of imgs) {
    const bounds = imageRowBounds(dom, img)
    const rect = bounds.image
    if (rect.width <= 0 || rect.height <= 0) continue

    const sameLine = event.clientY >= bounds.top && event.clientY <= bounds.bottom
    // 图片接近内容区满宽时，真正的“行尾”落在编辑器右内边距里，
    // 不属于 p 的 DOM rect。把这段 padding 也纳入命中，否则拖拽/插入
    // 产生的纯图片段落看起来有尾部空白但 WebEngine 不给 after-image 光标。
    const afterImage = event.clientX > rect.right && event.clientX <= bounds.editorRight
    const paragraphTail = event.clientY > rect.bottom
      && event.clientY <= bounds.bottom
      && event.clientX >= bounds.left
      && event.clientX <= bounds.editorRight
    if ((!sameLine || !afterImage) && !paragraphTail) continue

    const distance = paragraphTail
      ? Math.max(0, event.clientY - rect.bottom)
      : Math.max(0, event.clientX - rect.right)
    if (distance < candidateDistance) {
      candidate = img
      candidateDistance = distance
    }
  }
  return candidate
}

function ensureImageSelectionStyles() {
  if (typeof document === 'undefined') return
  if (document.getElementById('dvn-tiptap-image-block-style')) return
  const style = document.createElement('style')
  style.id = 'dvn-tiptap-image-block-style'
  style.textContent = imageBlockCss
  document.head.appendChild(style)
}

function syncImageSelectionState() {
  // 图片选中高亮已经由 ImageBlock NodeView 绑定到图片节点自身。
  // 这里保留同步入口，避免事件处理分支为了选中态再散落特殊判断。
}

/**
 * 注册图片查看原图、选中态与右键菜单入口。
 *
 * 注意：右键菜单必须继续走宿主 PictureCtxMenu，与 Summernote 的
 * webobj.jsCallPopupMenu(0, img.src) 保持一致；这里不再自绘网页菜单。
 * @param {Editor} editor
 * @param {object} bridge — QWebChannel bridge（需 jsRequestViewPicture 方法）
 * @returns {() => void} 销毁函数，移除监听与遮罩
 */
export function setupImageViewAndMenu(editor, bridge) {
  if (!editor || !bridge) return () => {}
  const dom = editor.view.dom
  ensureImageSelectionStyles()

  function viewOriginal(img) {
    const src = img.getAttribute('src') || ''
    if (src && bridge.jsRequestViewPicture) bridge.jsRequestViewPicture(src)
  }

  function syncOverlay() {
    syncImageSelectionState()
  }

  function onMouseDown(event) {
    if (findImageTarget(dom, event.target)) return
    const img = findImageForTrailingCaretClick(dom, event)
    if (!img || !shouldHandleTrailingCaretClick(editor, img)) return
    event.preventDefault()
    event.stopPropagation()
    if (setTextCursorAfterImageElement(editor, img)) syncOverlay()
  }

  function onClick(event) {
    const img = findImageTarget(dom, event.target)
    if (img) {
      selectImageNode(editor, img)
      syncOverlay()
    }
  }

  function onKeyDown(event) {
    if (['ArrowDown', 'ArrowUp'].includes(event.key)) {
      const direction = event.key === 'ArrowDown' ? 1 : -1
      if (moveVerticalToAdjacentBareImageParagraph(editor, direction)) {
        event.preventDefault()
        syncOverlay()
        return
      }
    }

    if (['ArrowRight', 'ArrowLeft'].includes(event.key)) {
      const direction = event.key === 'ArrowRight' ? 1 : -1
      if (moveHorizontalAroundBareImageParagraph(editor, direction)) {
        event.preventDefault()
        syncOverlay()
        return
      }
    }

    if (!['ArrowRight', 'ArrowLeft', 'ArrowDown', 'ArrowUp', 'End', 'Home'].includes(event.key)) return
    const { selection } = editor.state
    if (selection.node?.type?.name !== 'image') return
    event.preventDefault()
    if (['ArrowLeft', 'ArrowUp', 'Home'].includes(event.key)) {
      if (setTextCursorBeforeImagePos(editor, selection.from)) syncOverlay()
      return
    }
    if (setTextCursorAfterImagePos(editor, selection.from)) syncOverlay()
  }

  function onDblClick(event) {
    const img = findImageTarget(dom, event.target)
    if (img) {
      event.preventDefault()
      selectImageNode(editor, img)
      syncOverlay()
      viewOriginal(img)
    }
  }

  function onContextMenu(event) {
    const img = findImageTarget(dom, event.target)
    if (!img) return
    selectImageNode(editor, img)
    syncOverlay()
    // 不 preventDefault：让 Qt WebEngine 继续产生 contextMenuRequested，
    // QML 侧会按图片类型弹出应用统一 PictureCtxMenu。
  }

  dom.addEventListener('mousedown', onMouseDown, true)
  dom.addEventListener('click', onClick)
  dom.addEventListener('keydown', onKeyDown)
  dom.addEventListener('dblclick', onDblClick)
  dom.addEventListener('contextmenu', onContextMenu)
  return function destroy() {
    dom.removeEventListener('mousedown', onMouseDown, true)
    dom.removeEventListener('click', onClick)
    dom.removeEventListener('keydown', onKeyDown)
    dom.removeEventListener('dblclick', onDblClick)
    dom.removeEventListener('contextmenu', onContextMenu)
  }
}
