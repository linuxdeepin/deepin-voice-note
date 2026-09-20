// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 普通有序列表使用 CSS counter 自绘 marker，以便和无序/待办列表一样，
// marker 可以在大字号行高中垂直居中。这里仅同步 start 属性到运行态
// counter-reset 样式，不进入文档保存；即使同步未触发，CSS counter 也会
// 默认从 1 显示，避免编号不可见。

const ORDERED_LIST_SELECTOR = '.ProseMirror ol, ol'

function orderedListStart(ol) {
  const start = Number.parseInt(ol?.getAttribute?.('start') || '1', 10)
  return Number.isFinite(start) ? start : 1
}

export function syncOrderedListMarkers(root = document) {
  const lists = root.querySelectorAll?.(ORDERED_LIST_SELECTOR) || []
  for (const list of lists) {
    const resetValue = `dvn-ordered-list ${orderedListStart(list) - 1}`
    if (list.style.counterReset !== resetValue) {
      list.style.counterReset = resetValue
    }
  }
}

export function installNormalListMarkerSync(editorOrRoot) {
  const root = editorOrRoot?.view?.dom || editorOrRoot || document.querySelector('.ProseMirror') || document.body
  if (!root) return () => {}

  let frame = 0
  const run = () => {
    frame = 0
    syncOrderedListMarkers(root)
  }
  const schedule = () => {
    if (frame) return
    frame = window.requestAnimationFrame?.(run) || window.setTimeout(run, 0)
  }

  editorOrRoot?.on?.('transaction', schedule)
  editorOrRoot?.on?.('update', schedule)

  const observer = typeof MutationObserver === 'function' ? new MutationObserver(schedule) : null
  observer?.observe(root, {
    childList: true,
    subtree: true,
    attributes: true,
    attributeFilter: ['start'],
  })

  schedule()

  return () => {
    if (frame) {
      window.cancelAnimationFrame?.(frame)
      window.clearTimeout?.(frame)
      frame = 0
    }
    editorOrRoot?.off?.('transaction', schedule)
    editorOrRoot?.off?.('update', schedule)
    observer?.disconnect()
  }
}
