// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 异步资源插入光标管理。
// 图片选择、截图粘贴、文件拖拽都需要先经过 QML/C++ 落盘，再由
// QWebChannel 回插资源节点；这段异步间隔会让 WebEngine 失焦，甚至把
// ProseMirror selection 留在图片 NodeSelection 等运行态位置。这里以
// ProseMirror SelectionBookmark 为唯一状态源，等资源回插前恢复一次，
// 从架构上补齐 Summernote saveRange/restoreRange 的语义。

import { TextSelection } from '@tiptap/pm/state'

function clampDocPos(doc, pos) {
  const numeric = Number(pos)
  if (!Number.isFinite(numeric)) return null
  return Math.max(0, Math.min(Math.round(numeric), doc.content.size))
}

function safeBookmark(selection) {
  try {
    return selection?.getBookmark?.() || null
  } catch (err) {
    console.warn('[tiptap] capture insertion selection failed:', err)
    return null
  }
}

function safeResolveBookmark(bookmark, doc) {
  try {
    return bookmark?.resolve?.(doc) || null
  } catch (err) {
    console.warn('[tiptap] restore insertion selection failed:', err)
    return null
  }
}

function isTrailingHardBreak(node) {
  return node?.type?.name === 'hardBreak'
}

function bareImageCursorFromDomImage(editor, img, side) {
  if (!editor?.view || !img) return null

  let imagePos = null
  const nodeDom = img.closest?.('[data-dvn-image-node]') || img
  try {
    imagePos = editor.view.posAtDOM(nodeDom, 0)
  } catch (err) {
    console.warn('[tiptap] resolve drop image DOM position failed:', err)
    return null
  }

  const doc = editor.state.doc
  const imageNode = doc.nodeAt(imagePos)
  if (!imageNode || imageNode.type.name !== 'image') return null

  const $image = doc.resolve(imagePos)
  if ($image.parent.type.name !== 'paragraph') return null
  if ($image.parent.childCount < 1 || $image.parent.childCount > 2) return null
  if ($image.parent.child(0).type.name !== 'image') return null
  if ($image.parent.childCount === 2 && !isTrailingHardBreak($image.parent.child(1))) return null

  const pos = side === 'before' ? imagePos : imagePos + imageNode.nodeSize
  try {
    return TextSelection.create(doc, pos)
  } catch (err) {
    console.warn('[tiptap] create drop image-line selection failed:', err)
    return null
  }
}

function selectionNearBareImageLine(editor, left, top) {
  const dom = editor?.view?.dom
  if (!dom || typeof document === 'undefined') return null

  const x = Number(left)
  const y = Number(top)
  if (!Number.isFinite(x) || !Number.isFinite(y)) return null

  let editorRect = null
  try {
    editorRect = dom.getBoundingClientRect()
  } catch (_) {
    editorRect = null
  }

  const images = dom.querySelectorAll('img[data-rel-path]')
  let candidate = null
  let candidateSide = 'after'
  let candidateDistance = Number.POSITIVE_INFINITY

  for (const img of images) {
    const rect = img.getBoundingClientRect()
    if (rect.width <= 0 || rect.height <= 0) continue
    const paragraph = img.closest?.('p') || (img.parentElement?.tagName === 'P' ? img.parentElement : null)
    const rowRect = paragraph?.getBoundingClientRect?.() || rect
    const rowTop = Math.min(rowRect.height > 0 ? rowRect.top : rect.top, rect.top)
    const rowBottom = Math.max(rowRect.height > 0 ? rowRect.bottom : rect.bottom, rect.bottom)
    const hasEditorWidth = editorRect && editorRect.width > 0
    const hasRowWidth = rowRect.width > 0
    let editorOuterLeft = hasEditorWidth ? editorRect.left : (hasRowWidth ? rowRect.left : Number.NEGATIVE_INFINITY)
    let editorOuterRight = hasEditorWidth ? editorRect.right : (hasRowWidth ? rowRect.right : Number.POSITIVE_INFINITY)
    let editorContentLeft = hasRowWidth ? rowRect.left : Number.NEGATIVE_INFINITY
    let editorContentRight = hasRowWidth ? rowRect.right : Number.POSITIVE_INFINITY
    if (hasEditorWidth && typeof window !== 'undefined') {
      try {
        const style = window.getComputedStyle(dom)
        editorContentLeft = editorRect.left + (Number.parseFloat(style.paddingLeft) || 0)
        editorContentRight = editorRect.right - (Number.parseFloat(style.paddingRight) || 0)
      } catch (_) {
        editorContentLeft = rowRect.left
        editorContentRight = rowRect.right
      }
    }
    const rowLeft = Math.min(rowRect.left, editorContentLeft)
    const rowRight = Math.max(rowRect.right, editorContentRight)
    // Treat the whole bare image visual row as an insertion target.  This fixes
    // Qt DropArea drags where WebEngine's posAtCoords often resolves an image
    // line to the browser's preferred side instead of the user's drop side.
    // When an image fills the content width, the visible tail is often only the
    // editor padding outside p; include the WebEngine/editor outer bounds too.
    if (y < rowTop || y > rowBottom) continue
    if (x < editorOuterLeft || x > editorOuterRight) continue

    const horizontalDistance = x < rect.left ? rect.left - x : (x > rect.right ? x - rect.right : 0)
    if (horizontalDistance >= candidateDistance) continue
    candidate = img
    if (y > rect.bottom) {
      // The paragraph tail below an inline image is ProseMirror's runtime
      // trailingBreak area.  A drop there visually means “after this image”,
      // even when the pointer is on the left side of the row.
      candidateSide = 'after'
    } else if (y < rect.top) {
      candidateSide = 'before'
    } else {
      candidateSide = x <= rect.left + rect.width / 2 && x <= rowRight ? 'before' : 'after'
    }
    candidateDistance = horizontalDistance
  }

  return candidate ? bareImageCursorFromDomImage(editor, candidate, candidateSide) : null
}

function selectionNearClientPoint(editor, left, top) {
  const view = editor?.view
  if (!view || typeof view.posAtCoords !== 'function') return null

  const x = Number(left)
  const y = Number(top)
  if (!Number.isFinite(x) || !Number.isFinite(y)) return null

  const imageLineSelection = selectionNearBareImageLine(editor, x, y)
  if (imageLineSelection) return imageLineSelection

  const hit = view.posAtCoords({ left: x, top: y })
  const pos = clampDocPos(editor.state.doc, hit?.pos)
  if (pos == null) return null

  try {
    return TextSelection.create(editor.state.doc, pos)
  } catch (err) {
    try {
      return TextSelection.near(editor.state.doc.resolve(pos))
    } catch (nearErr) {
      console.warn('[tiptap] resolve drop insertion position failed:', nearErr)
      return null
    }
  }
}

/**
 * 创建资源插入光标管理器。
 *
 * 设计约束：
 * - capture/captureAtClientPoint 只记录“下一次”资源插入位置；
 * - restoreAndConsume 在首个资源节点回插前恢复并消费该位置，多图连续
 *   插入时后续图片沿用上一张图片插入后的自然光标；
 * - pending 期间如文档有普通编辑事务，bookmark 跟随 mapping 迁移；
 * - 宿主加载新笔记时由调用方 clear，避免跨笔记误插。
 */
export function createResourceInsertionSelection(editor) {
  let bookmark = null
  let active = false

  function clear() {
    bookmark = null
    active = false
  }

  function capture() {
    if (!editor?.state?.selection) return false
    const nextBookmark = safeBookmark(editor.state.selection)
    if (!nextBookmark) {
      clear()
      return false
    }
    bookmark = nextBookmark
    active = true
    return true
  }

  function captureAtClientPoint(left, top) {
    if (!editor?.view) return false
    const selection = selectionNearClientPoint(editor, left, top)
    if (selection) {
      const tr = editor.state.tr.setSelection(selection).scrollIntoView()
      editor.view.dispatch(tr)
      editor.view.focus()
    }
    return capture()
  }

  function restoreAndConsume() {
    if (!active || !bookmark || !editor?.view) return false

    const selection = safeResolveBookmark(bookmark, editor.state.doc)
    clear()
    if (!selection) return false

    try {
      const tr = editor.state.tr.setSelection(selection).scrollIntoView()
      editor.view.dispatch(tr)
      editor.view.focus()
      return true
    } catch (err) {
      console.warn('[tiptap] dispatch insertion selection failed:', err)
      return false
    }
  }

  function updateOnTransaction(payload) {
    if (!active || !bookmark) return
    const transaction = payload?.transaction || payload
    if (!transaction?.docChanged) return
    try {
      if (typeof bookmark.map === 'function') {
        bookmark = bookmark.map(transaction.mapping)
      }
    } catch (err) {
      console.warn('[tiptap] map insertion selection failed:', err)
      clear()
    }
  }

  editor?.on?.('transaction', updateOnTransaction)

  return {
    capture,
    captureAtClientPoint,
    restoreAndConsume,
    clear,
    destroy() {
      editor?.off?.('transaction', updateOnTransaction)
      clear()
    },
  }
}
