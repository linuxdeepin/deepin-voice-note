// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 图片交互模块：粘贴策略与图片 DOM 事件委托。
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
// 图片 DOM 事件委托：单击选中、双击查看原图、右键菜单（查看原图 / 删除）
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

function selectImageNode(editor, img) {
  const pos = editor.view.posAtDOM(img, 0)
  if (pos == null || pos < 0) return
  editor.chain().setNodeSelection(pos).run()
}

function createImageContextMenu({ onView, onDelete, x, y }) {
  const menu = document.createElement('div')
  menu.setAttribute('data-testid', 'tiptap-image-menu')
  Object.assign(menu.style, {
    position: 'fixed',
    left: `${x}px`,
    top: `${y}px`,
    background: '#fff',
    border: '1px solid #ccc',
    boxShadow: '0 2px 8px rgba(0,0,0,0.15)',
    zIndex: '1000',
    padding: '2px 0',
    fontSize: '14px',
  })

  const viewItem = document.createElement('div')
  viewItem.textContent = '查看原图'
  viewItem.setAttribute('data-action', 'view-original')
  Object.assign(viewItem.style, { padding: '6px 18px', cursor: 'pointer' })
  viewItem.addEventListener('mouseenter', () => { viewItem.style.background = '#f0f0f0' })
  viewItem.addEventListener('mouseleave', () => { viewItem.style.background = 'transparent' })
  viewItem.addEventListener('click', (e) => { e.stopPropagation(); onView() })

  const deleteItem = document.createElement('div')
  deleteItem.textContent = '删除'
  deleteItem.setAttribute('data-action', 'delete-image')
  Object.assign(deleteItem.style, { padding: '6px 18px', cursor: 'pointer' })
  deleteItem.addEventListener('mouseenter', () => { deleteItem.style.background = '#f0f0f0' })
  deleteItem.addEventListener('mouseleave', () => { deleteItem.style.background = 'transparent' })
  deleteItem.addEventListener('click', (e) => { e.stopPropagation(); onDelete() })

  menu.appendChild(viewItem)
  menu.appendChild(deleteItem)
  return menu
}

/**
 * 注册图片查看原图与右键菜单交互：绑定到 editor.view.dom 上的委托式监听。
 * @param {Editor} editor
 * @param {object} bridge — QWebChannel bridge（需 jsRequestViewPicture 方法）
 * @returns {() => void} 销毁函数，移除监听与菜单
 */
export function setupImageViewAndMenu(editor, bridge) {
  if (!editor || !bridge) return () => {}
  const dom = editor.view.dom
  let menuEl = null

  function closeMenu() {
    if (menuEl && menuEl.parentNode) menuEl.parentNode.removeChild(menuEl)
    menuEl = null
    document.removeEventListener('click', onOutsideClick)
  }

  function onOutsideClick(event) {
    if (menuEl && !menuEl.contains(event.target)) closeMenu()
  }

  function viewOriginal(img) {
    const src = img.getAttribute('src') || ''
    if (src && bridge.jsRequestViewPicture) bridge.jsRequestViewPicture(src)
  }

  function onClick(event) {
    const img = findImageTarget(dom, event.target)
    if (img) {
      selectImageNode(editor, img)
    }
  }

  function onDblClick(event) {
    const img = findImageTarget(dom, event.target)
    if (img) {
      event.preventDefault()
      viewOriginal(img)
    }
  }

  function onContextMenu(event) {
    const img = findImageTarget(dom, event.target)
    if (!img) return
    event.preventDefault()
    selectImageNode(editor, img)
    closeMenu()
    menuEl = createImageContextMenu({
      onView: () => { viewOriginal(img); closeMenu() },
      onDelete: () => { editor.chain().deleteSelection().run(); closeMenu() },
      x: event.clientX,
      y: event.clientY,
    })
    document.body.appendChild(menuEl)
    setTimeout(() => document.addEventListener('click', onOutsideClick), 0)
  }

  dom.addEventListener('click', onClick)
  dom.addEventListener('dblclick', onDblClick)
  dom.addEventListener('contextmenu', onContextMenu)

  return function destroy() {
    dom.removeEventListener('click', onClick)
    dom.removeEventListener('dblclick', onDblClick)
    dom.removeEventListener('contextmenu', onContextMenu)
    closeMenu()
  }
}
