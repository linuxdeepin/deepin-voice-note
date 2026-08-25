// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 图片交互模块：粘贴策略与图片 DOM 事件委托。

import { TextSelection } from '@tiptap/pm/state'
import imageBlockCss from '../extensions/image-block.css?inline'

// 不为 image 节点实现 NodeView，通过 ProseMirror editorProps 与 editor.view.dom
// 上的委托式事件监听完成交互，改动面最小。

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
 * @returns {() => void} 销毁函数，移除 handlePaste 处理
 */
export function setupImagePaste(editor, bridge) {
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
  const pos = editor.view.posAtDOM(img, 0)
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

function setTextCursorAfterImageElement(editor, img) {
  const pos = imagePosFromElement(editor, img)
  return pos == null ? false : setTextCursorAfterImagePos(editor, pos)
}

function hasInlineContentAfterImage(editor, imagePos) {
  const imageNode = editor.state.doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return false
  const afterImage = imagePos + imageNode.nodeSize
  const $after = editor.state.doc.resolve(afterImage)
  return $after.parent.inlineContent && $after.parentOffset < $after.parent.content.size
}

function shouldHandleTrailingCaretClick(editor, img) {
  const pos = imagePosFromElement(editor, img)
  if (pos == null) return false
  // If text or another inline node already exists after the image, normal
  // ProseMirror hit testing must place the caret inside that content.  Only
  // synthesize a caret for the empty right side of a bare image paragraph.
  return !hasInlineContentAfterImage(editor, pos)
}

function soleImageCursorPosInParagraph(doc, paragraphPos) {
  const paragraph = doc.nodeAt(paragraphPos)
  if (!paragraph || paragraph.type.name !== 'paragraph') return null
  if (paragraph.childCount !== 1) return null
  const child = paragraph.child(0)
  if (child.type.name !== 'image') return null
  // paragraphPos points before the paragraph node. +1 enters paragraph content,
  // +child.nodeSize places the caret after the inline image.
  return paragraphPos + 1 + child.nodeSize
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
  const cursorPos = soleImageCursorPosInParagraph(doc, paragraphPos)
  if (cursorPos == null) return false
  return dispatchTextCursor(editor, cursorPos)
}

function findImageForTrailingCaretClick(dom, event) {
  if (!event || event.clientX == null || event.clientY == null) return null
  const imgs = dom.querySelectorAll('img[data-rel-path]')
  let candidate = null
  let candidateDistance = Number.POSITIVE_INFINITY
  for (const img of imgs) {
    const rect = img.getBoundingClientRect()
    if (rect.width <= 0 || rect.height <= 0) continue
    const sameLine = event.clientY >= rect.top && event.clientY <= rect.bottom
    const afterImage = event.clientX > rect.right
    if (!sameLine || !afterImage) continue
    const distance = event.clientX - rect.right
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

function createImageSelectionOverlay() {
  ensureImageSelectionStyles()
  const overlay = document.createElement('div')
  overlay.className = 'dvn-image-selection'
  overlay.setAttribute('data-testid', 'tiptap-image-selection')
  document.body.appendChild(overlay)
  return overlay
}

function selectedImageElement(editor) {
  const { selection } = editor.state
  if (selection.node?.type?.name !== 'image') return null
  const dom = editor.view.nodeDOM(selection.from)
  if (dom?.tagName === 'IMG') return dom
  return dom?.querySelector?.('img') || null
}

function updateImageSelectionOverlay(editor, overlay) {
  const img = selectedImageElement(editor)
  if (!img || !document.contains(img)) {
    overlay.style.display = 'none'
    return
  }

  const rect = img.getBoundingClientRect()
  if (rect.width <= 0 || rect.height <= 0) {
    overlay.style.display = 'none'
    return
  }

  overlay.style.display = 'block'
  overlay.style.left = `${Math.round(rect.left + window.scrollX)}px`
  overlay.style.top = `${Math.round(rect.top + window.scrollY)}px`
  overlay.style.width = `${Math.round(rect.width)}px`
  overlay.style.height = `${Math.round(rect.height)}px`
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
  const overlay = createImageSelectionOverlay()

  function viewOriginal(img) {
    const src = img.getAttribute('src') || ''
    if (src && bridge.jsRequestViewPicture) bridge.jsRequestViewPicture(src)
  }

  function syncOverlay() {
    updateImageSelectionOverlay(editor, overlay)
  }

  function onMouseDown(event) {
    if (findImageTarget(dom, event.target)) return
    const img = findImageForTrailingCaretClick(dom, event)
    if (!img || !shouldHandleTrailingCaretClick(editor, img)) return
    event.preventDefault()
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

    if (!['ArrowRight', 'ArrowDown', 'End'].includes(event.key)) return
    const { selection } = editor.state
    if (selection.node?.type?.name !== 'image') return
    event.preventDefault()
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

  dom.addEventListener('mousedown', onMouseDown)
  dom.addEventListener('click', onClick)
  dom.addEventListener('keydown', onKeyDown)
  dom.addEventListener('dblclick', onDblClick)
  dom.addEventListener('contextmenu', onContextMenu)
  editor.on('transaction', syncOverlay)
  window.addEventListener('scroll', syncOverlay, true)
  window.addEventListener('resize', syncOverlay)
  syncOverlay()

  return function destroy() {
    dom.removeEventListener('mousedown', onMouseDown)
    dom.removeEventListener('click', onClick)
    dom.removeEventListener('keydown', onKeyDown)
    dom.removeEventListener('dblclick', onDblClick)
    dom.removeEventListener('contextmenu', onContextMenu)
    editor.off('transaction', syncOverlay)
    window.removeEventListener('scroll', syncOverlay, true)
    window.removeEventListener('resize', syncOverlay)
    if (overlay.parentNode) overlay.parentNode.removeChild(overlay)
  }
}
