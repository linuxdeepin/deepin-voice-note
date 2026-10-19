// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { AllSelection, Plugin, Selection, TextSelection } from '@tiptap/pm/state'

// TTP-022 选区交互兜底：全选（Ctrl+A / 框选）后单击正文，紫色高亮不消失。
//
// 成因：ProseMirror 的 LeftMouseDown.up() 把“单击折叠选区”交给浏览器原生行为，只有
// ① click 处理器消费了这次点击、② DOM 已被 flush、③ 编辑器失焦且落点在端点 2 字符内，
// 这三种情况才由 ProseMirror 自己折叠；其余情况只 setSelectionOrigin(view, "pointer")，
// 等浏览器折叠原生选区后再由 DOM observer 同步回 state。
// 而本应用 WebEngine 只要“按下→抬起”不是瞬时点击（实测按下 >10ms，真人点击普遍
// 60–200ms）或指针抖动 ≥4 物理像素（触摸板很常见），就不再派发 click，原生选区也不折叠，
// 于是 ProseMirror 和浏览器两边都不动，选区（高亮）就一直留在原地。
//
// 修复：按下时对非空文本选区做快照，抬起时若这次是“未拖动的普通左键点击”且选区原样未变，
// 就按浏览器本该有的语义把选区折叠到落点。不依赖 click 事件，也不影响拖动选区内文字、
// 拖动选中图片、Shift 扩选、双击选词、待办勾选等既有行为。
const DRAG_TOLERANCE_PX = 4
const PROBE_STEP_PX = 8

function isTextLikeSelection(selection) {
  return selection instanceof TextSelection || selection instanceof AllSelection
}

function posAt(view, x, y) {
  if (typeof view.posAtCoords !== 'function') return null
  const hit = view.posAtCoords({ left: x, top: y })
  return hit && typeof hit.pos === 'number' ? hit.pos : null
}

// 命中的正文位置；点在段落下方/左侧留白等 padding 上时，向四周探测最近的可落点位置。
function nearestTextPos(view, x, y) {
  const direct = posAt(view, x, y)
  if (direct !== null) return direct

  const rect = view.dom?.getBoundingClientRect?.()
  if (!rect || !rect.height) return null

  const xs = [x, rect.left + 2, rect.right - 2]
  for (let delta = PROBE_STEP_PX; delta <= rect.height; delta += PROBE_STEP_PX) {
    for (const probeY of [y - delta, y + delta]) {
      if (probeY < rect.top + 1 || probeY > rect.bottom - 1) continue
      for (const probeX of xs) {
        const pos = posAt(view, probeX, probeY)
        if (pos !== null) return pos
      }
    }
  }
  return null
}

function collapseTo(view, pos) {
  const { doc } = view.state
  const raw = typeof pos === 'number' ? pos : doc.content.size
  const safe = Math.max(0, Math.min(raw, doc.content.size))
  view.dispatch(
    view.state.tr.setSelection(Selection.near(doc.resolve(safe))).setMeta('addToHistory', false),
  )
}

export function createSelectionClickResetPlugin() {
  let press = null

  const reset = () => {
    press = null
  }

  return new Plugin({
    props: {
      handleDOMEvents: {
        mousedown(view, event) {
          reset()
          if (event.button !== 0) return false
          if (event.shiftKey || event.altKey || event.metaKey || event.ctrlKey) return false
          if (typeof view.dom.contains === 'function' && !view.dom.contains(event.target)) return false
          const { selection } = view.state
          if (selection.empty || !isTextLikeSelection(selection)) return false
          press = {
            from: selection.from,
            to: selection.to,
            doc: view.state.doc,
            x: event.clientX,
            y: event.clientY,
            dragged: false,
          }
          return false
        },

        mousemove(view, event) {
          if (!press || press.dragged) return false
          if (event.buttons === 0) {
            reset()
            return false
          }
          if (
            Math.abs(event.clientX - press.x) > DRAG_TOLERANCE_PX ||
            Math.abs(event.clientY - press.y) > DRAG_TOLERANCE_PX
          ) {
            press.dragged = true
          }
          return false
        },

        dragstart() {
          if (press) press.dragged = true
          return false
        },

        mouseup(view, event) {
          const snapshot = press
          reset()
          if (!snapshot || snapshot.dragged) return false
          if (event.button !== 0) return false
          if (view.isDestroyed) return false
          if (typeof view.dom.contains === 'function' && !view.dom.contains(event.target)) return false

          const { clientX: x, clientY: y } = event
          // 等浏览器原生折叠、ProseMirror 的 click 处理（图片选中、待办勾选等）跑完，
          // 只有选区仍原封不动时才兜底折叠，避免抢掉它们的行为。
          setTimeout(() => {
            if (view.isDestroyed) return
            const { selection, doc } = view.state
            if (doc !== snapshot.doc) return
            if (selection.empty) return
            if (selection.from !== snapshot.from || selection.to !== snapshot.to) return
            collapseTo(view, nearestTextPos(view, x, y))
          }, 0)
          return false
        },
      },
    },
  })
}

export const SelectionClickReset = Extension.create({
  name: 'selectionClickReset',

  addProseMirrorPlugins() {
    return [createSelectionClickResetPlugin()]
  },
})
