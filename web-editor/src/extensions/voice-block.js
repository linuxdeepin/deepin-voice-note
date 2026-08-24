// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Node, mergeAttributes } from '@tiptap/core'
import { Plugin, PluginKey } from '@tiptap/pm/state'
import { Slice, Fragment } from '@tiptap/pm/model'
import voiceBlockCss from './voice-block.css?inline'
import { getVoiceBridge, subscribeVoiceEvents } from '../runtime/tiptap-channel.js'

// Inject self-contained styles at runtime (no external CSS link needed)
if (typeof document !== 'undefined') {
  const style = document.createElement('style')
  style.textContent = voiceBlockCss
  document.head.appendChild(style)
}

/**
 * 生成语音块唯一标识（UUID）。优先使用 crypto.randomUUID，
 * 测试环境（happy-dom）回退到 Math.random 方案。
 * @returns {string}
 */
function generateVoiceId() {
  if (typeof crypto !== 'undefined' && typeof crypto.randomUUID === 'function') {
    return crypto.randomUUID()
  }
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
    const r = Math.random() * 16 | 0
    const v = c === 'x' ? r : (r & 0x3 | 0x8)
    return v.toString(16)
  })
}

/**
 * 毫秒转 mm:ss 文本。
 * @param {number} ms
 * @returns {string}
 */
function formatTime(ms) {
  if (typeof ms !== 'number' || ms < 0) ms = 0
  const totalSeconds = Math.floor(ms / 1000)
  const hours = Math.floor(totalSeconds / 3600)
  const minutes = Math.floor((totalSeconds % 3600) / 60)
  const seconds = totalSeconds % 60
  return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`
}

/**
 * 旧语音块显示到分钟，日期分隔符使用 /。
 * @param {string|null|undefined} value
 * @returns {string}
 */
function formatCreateTime(value) {
  if (typeof value !== 'string' || value.length === 0) return ''
  return value.slice(0, 16).replace(/-/g, '/')
}

/**
 * 由节点 attrs 构造发给宿主的 voiceInfoJson（MetaDataParser 可解析格式）。
 * @param {object} attrs
 * @returns {string}
 */
function buildVoiceInfoJson(attrs) {
  return JSON.stringify({
    type: 2,
    voiceId: attrs.voiceId,
    voicePath: attrs.voicePath,
    voiceSize: attrs.voiceSize,
    title: attrs.title,
    createTime: attrs.createTime,
  })
}

/**
 * 由节点 attrs 构造宿主菜单可解析的 voice JSON（MetaDataParser 兼容格式）。
 * 与 buildVoiceInfoJson 的区别：使用键 type（而非 dataType），并补齐 text/state，
 * 供右键菜单探测脚本通过 data-voice-meta 属性读取后下发给 C++ processVoiceMenuRequest。
 * @param {object} attrs
 * @returns {string}
 */
function buildVoiceMenuJson(attrs) {
  return JSON.stringify({
    type: 2,
    voiceId: attrs.voiceId,
    voicePath: attrs.voicePath,
    voiceSize: attrs.voiceSize,
    title: attrs.title,
    createTime: attrs.createTime,
    text: attrs.text,
    state: !!attrs.text,
  })
}

export const VoiceBlock = Node.create({
  name: 'voiceBlock',

  group: 'block',
  atom: true,
  selectable: true,
  draggable: true,

  addAttributes() {
    return {
      voiceId: { default: null },
      voicePath: { default: null },
      voiceSize: { default: 0 },
      createTime: { default: null },
      title: { default: null },
      text: { default: null },
      translateUnfold: { default: true },
    }
  },

  parseHTML() {
    return [{ tag: 'div[data-type="voice-block"]' }]
  },

  renderHTML({ HTMLAttributes }) {
    return ['div', mergeAttributes(HTMLAttributes, { 'data-type': 'voice-block' })]
  },

  addNodeView() {
    return ({ node, view, getPos, editor }) => {
      // --- 运行态（视图态，不进 addAttributes，不进持久化 JSON） ---
      let playing = false
      let paused = false
      let translating = false
      let progress = 0
      let duration = node.attrs.voiceSize || 0
      let seeking = false
      let unplayable = false
      let currentNode = node
      let destroyed = false

      // --- 构建 DOM ---
      const wrapper = document.createElement('div')
      wrapper.className = 'voiceBox'
      wrapper.setAttribute('data-type', 'voice-block')
      wrapper.setAttribute('data-voice-meta', buildVoiceMenuJson(node.attrs))

      const box = document.createElement('div')
      box.className = 'voiceInfoBox'
      box.setAttribute('data-type', 'voice-block')
      box.setAttribute('data-voice-meta', buildVoiceMenuJson(node.attrs))
      wrapper.appendChild(box)

      const playback = document.createElement('div')
      playback.className = 'voicePlayback'
      box.appendChild(playback)

      const left = document.createElement('div')
      left.className = 'left'

      const voiceBtn = document.createElement('div')
      voiceBtn.className = 'voiceBtn'

      const titleEl = document.createElement('div')
      titleEl.className = 'title'
      titleEl.textContent = node.attrs.title || ''

      left.appendChild(voiceBtn)
      left.appendChild(titleEl)
      playback.appendChild(left)

      const createTimeEl = document.createElement('div')
      createTimeEl.className = 'createTime'
      createTimeEl.textContent = formatCreateTime(node.attrs.createTime)

      const progressBar = document.createElement('input')
      progressBar.type = 'range'
      progressBar.className = 'progressBar'
      progressBar.min = 0
      progressBar.max = Math.max(duration, 1)
      progressBar.value = 0
      playback.appendChild(progressBar)

      const right = document.createElement('div')
      right.className = 'right'
      const timeField = document.createElement('div')
      timeField.className = 'timeField'
      const timePassed = document.createElement('div')
      timePassed.className = 'timePassed'
      timePassed.textContent = '00:00/'
      const timeTotal = document.createElement('div')
      timeTotal.className = 'timeTotal'
      timeTotal.textContent = formatTime(duration)
      timeField.appendChild(timePassed)
      timeField.appendChild(timeTotal)
      right.appendChild(timeField)

      const toTextLabel = document.createElement('div')
      toTextLabel.className = 'voiceToTextLabel'
      const toTextIcon = document.createElement('div')
      toTextIcon.className = 'voiceToTextIcon'
      const translatingLabel = document.createElement('span')
      translatingLabel.className = 'translatingLabel'
      translatingLabel.textContent = '正在转为文字'
      toTextLabel.appendChild(toTextIcon)
      toTextLabel.appendChild(translatingLabel)
      right.appendChild(toTextLabel)

      const toTextTrigger = document.createElement('div')
      toTextTrigger.className = 'voiceToTextTrigger'
      toTextTrigger.textContent = '转文字'
      right.appendChild(toTextTrigger)

      const closePlaybackBarBtn = document.createElement('div')
      closePlaybackBarBtn.className = 'closePlaybackBarBtn'
      right.appendChild(closePlaybackBarBtn)

      playback.appendChild(right)

      // 转写结果区
      const translate = document.createElement('div')
      translate.className = 'voiceTranscript translate'
      const translateHeader = document.createElement('div')
      translateHeader.className = 'translateHeader'
      const translateIcon = document.createElement('div')
      translateIcon.className = 'translateIcon'
      const translateLabel = document.createElement('span')
      translateLabel.className = 'translateLabel'
      translateLabel.textContent = '语音转文字'
      const foldBtn = document.createElement('div')
      foldBtn.className = 'foldBtn'
      translateHeader.appendChild(translateIcon)
      translateHeader.appendChild(translateLabel)
      translateHeader.appendChild(foldBtn)
      translate.appendChild(translateHeader)

      const translateText = document.createElement('div')
      translateText.className = 'translateText'
      translateText.setAttribute('draggable', 'false')
      translateText.setAttribute('contenteditable', 'false')
      translateText.textContent = node.attrs.text || ''
      translate.appendChild(translateText)
      box.appendChild(translate)

      // --- 初始状态渲染 ---
      function refreshState() {
        playback.classList.toggle('play', playing)
        playback.classList.toggle('pause', paused)
        playback.classList.toggle('voiceToText', translating)
        playback.classList.toggle('unplayable', unplayable)

        const hasText = !!(currentNode.attrs.text && currentNode.attrs.text.length > 0)
        box.classList.toggle('containText', hasText)
        toTextTrigger.style.display = 'none'
        createTimeEl.style.display = (playing || paused || translating) ? 'none' : ''

        const unfold = currentNode.attrs.translateUnfold !== false
        translateHeader.classList.toggle('unfold', unfold)
        translateText.style.display = (hasText && unfold) ? '' : 'none'

        // 进度
        const pct = duration > 0 ? Math.min((progress / duration) * 100, 100) : 0
        progressBar.style.setProperty('--progressValue', pct + '%')
        timePassed.textContent = formatTime(progress) + '/'
        timeTotal.textContent = formatTime(duration)
        progressBar.max = Math.max(duration, 1)
        progressBar.value = Math.min(progress, progressBar.max)
      }

      // --- 事件处理 ---

      function onVoiceBtnClick() {
        const bridge = getVoiceBridge()
        if (!bridge || unplayable) return
        if (bridge.jsRequestVoicePlayback) {
          bridge.jsRequestVoicePlayback(buildVoiceInfoJson(currentNode.attrs))
        }
      }

      function onToTextTriggerClick() {
        const bridge = getVoiceBridge()
        if (!bridge || translating) return
        if (bridge.jsRequestVoiceToText) {
          bridge.jsRequestVoiceToText(buildVoiceInfoJson(currentNode.attrs))
        }
      }

      function onClosePlaybackClick() {
        const bridge = getVoiceBridge()
        if (bridge && bridge.jsRequestVoicePlaybackStop) {
          bridge.jsRequestVoicePlaybackStop()
        }
        playing = false
        paused = false
        progress = 0
        refreshState()
      }

      function onFoldToggle() {
        const pos = getPos()
        if (pos == null || pos < 0) return
        view.dispatch(view.state.tr.setNodeMarkup(pos, undefined, {
          ...currentNode.attrs,
          translateUnfold: currentNode.attrs.translateUnfold === false ? true : false,
        }))
      }

      let restoreVoiceDrag = null
      function restoreWrapperDrag() {
        if (!restoreVoiceDrag) return
        restoreVoiceDrag()
        restoreVoiceDrag = null
      }

      function onTranslateTextPointerDown(event) {
        // The voiceBlock node is draggable as a whole.  When the user starts in
        // transcript text, keep the event inside the DOM so the browser can make
        // a normal text selection instead of ProseMirror dragging the atom node.
        event.stopPropagation()
        restoreWrapperDrag()

        const previousDraggable = wrapper.getAttribute('draggable')
        wrapper.setAttribute('draggable', 'false')
        restoreVoiceDrag = () => {
          if (previousDraggable === null) {
            wrapper.removeAttribute('draggable')
          } else {
            wrapper.setAttribute('draggable', previousDraggable)
          }
          document.removeEventListener('mouseup', restoreWrapperDrag, true)
          document.removeEventListener('pointerup', restoreWrapperDrag, true)
          document.removeEventListener('touchend', restoreWrapperDrag, true)
          document.removeEventListener('dragend', restoreWrapperDrag, true)
        }

        document.addEventListener('mouseup', restoreWrapperDrag, true)
        document.addEventListener('pointerup', restoreWrapperDrag, true)
        document.addEventListener('touchend', restoreWrapperDrag, true)
        document.addEventListener('dragend', restoreWrapperDrag, true)
      }

      function onTranslateTextDragStart(event) {
        event.stopPropagation()
        event.preventDefault()
      }

      voiceBtn.addEventListener('click', onVoiceBtnClick)
      toTextTrigger.addEventListener('click', onToTextTriggerClick)
      closePlaybackBarBtn.addEventListener('click', onClosePlaybackClick)
      foldBtn.addEventListener('click', onFoldToggle)
      translateText.addEventListener('mousedown', onTranslateTextPointerDown)
      translateText.addEventListener('pointerdown', onTranslateTextPointerDown)
      translateText.addEventListener('touchstart', onTranslateTextPointerDown)
      translateText.addEventListener('dragstart', onTranslateTextDragStart)
      translateHeader.addEventListener('click', (e) => {
        // 折叠/展开头部点击切换
        if (e.target === foldBtn) return
        onFoldToggle()
      })

      // 进度条拖拽
      progressBar.addEventListener('input', () => {
        seeking = true
        progress = Number(progressBar.value)
        progressBar.style.setProperty('--progressValue',
          (duration > 0 ? (progress / duration) * 100 : 0) + '%')
        timePassed.textContent = formatTime(progress)
      })
      progressBar.addEventListener('change', () => {
        const bridge = getVoiceBridge()
        if (bridge && bridge.jsRequestVoiceSeek) {
          bridge.jsRequestVoiceSeek(String(Number(progressBar.value)))
        }
        seeking = false
      })

      // --- voice 事件订阅 ---
      const unsubscribe = subscribeVoiceEvents(node.attrs.voiceId, {
        onPlaybackStateChanged(state) {
          // state: 0=Playing, 1=Paused, 2=End
          if (state === 0) {
            playing = true
            paused = false
          } else if (state === 1) {
            playing = false
            paused = true
          } else {
            playing = false
            paused = false
            progress = 0
          }
          refreshState()
        },
        onPositionChanged(ms) {
          if (seeking) return
          progress = ms
          refreshState()
        },
        onDurationChanged(ms) {
          duration = ms
          refreshState()
        },
        onFileError() {
          unplayable = true
          playing = false
          paused = false
          refreshState()
        },
        onToTextStarted() {
          translating = true
          refreshState()
        },
        onToTextFailed() {
          translating = false
          refreshState()
        },
        onToTextCompleted(text) {
          translating = false
          // 写回 attrs.text，触发保存
          const pos = getPos()
          if (pos != null && pos >= 0) {
            view.dispatch(view.state.tr.setNodeMarkup(pos, undefined, {
              ...currentNode.attrs,
              text,
            }))
          }
          refreshState()
        },
      })

      refreshState()

      return {
        dom: wrapper,
        update(updatedNode) {
          if (updatedNode.type.name !== 'voiceBlock') return false
          currentNode = updatedNode
          titleEl.textContent = updatedNode.attrs.title || ''
          createTimeEl.textContent = formatCreateTime(updatedNode.attrs.createTime)
          translateText.textContent = updatedNode.attrs.text || ''
          wrapper.setAttribute('data-voice-meta', buildVoiceMenuJson(updatedNode.attrs))
          box.setAttribute('data-voice-meta', buildVoiceMenuJson(updatedNode.attrs))
          refreshState()
          return true
        },
        stopEvent(event) {
          const target = event.target
          if (target instanceof HTMLElement) {
            if (target.closest('.voiceBtn, .progressBar, .voiceToTextTrigger, .closePlaybackBarBtn, .foldBtn, .translateHeader, .translateText')) {
              return true
            }
          }
          return false
        },
        ignoreMutation() {
          return true
        },
        selectNode() {
          wrapper.classList.add('ProseMirror-selectednode', 'active')
        },
        deselectNode() {
          wrapper.classList.remove('ProseMirror-selectednode', 'active')
        },
        destroy() {
          if (destroyed) return
          destroyed = true
          unsubscribe()
          voiceBtn.removeEventListener('click', onVoiceBtnClick)
          toTextTrigger.removeEventListener('click', onToTextTriggerClick)
          closePlaybackBarBtn.removeEventListener('click', onClosePlaybackClick)
          foldBtn.removeEventListener('click', onFoldToggle)
          translateText.removeEventListener('mousedown', onTranslateTextPointerDown)
          translateText.removeEventListener('pointerdown', onTranslateTextPointerDown)
          translateText.removeEventListener('touchstart', onTranslateTextPointerDown)
          translateText.removeEventListener('dragstart', onTranslateTextDragStart)
          restoreWrapperDrag()
        },
      }
    }
  },

  addProseMirrorPlugins() {
    let internalVoiceDrag = false
    const resetInternalVoiceDrag = () => {
      internalVoiceDrag = false
    }

    return [
      new Plugin({
        key: new PluginKey('voiceBlockCopyClear'),
        props: {
          handleDOMEvents: {
            dragstart(view, event) {
              const target = event.target
              if (target instanceof HTMLElement
                && target.closest('.voiceBox')
                && !target.closest('.translateText')) {
                internalVoiceDrag = true
              }
              return false
            },
            drop() {
              resetInternalVoiceDrag()
              return false
            },
            dragend() {
              resetInternalVoiceDrag()
              return false
            },
            copy() {
              resetInternalVoiceDrag()
              return false
            },
          },
          transformCopied(slice) {
            if (internalVoiceDrag) return slice

            let modified = false
            function transformFragment(fragment) {
              const newNodes = []
              for (let i = 0; i < fragment.childCount; i++) {
                const node = fragment.child(i)
                if (node.type.name === 'voiceBlock') {
                  modified = true
                  newNodes.push(node.type.create({
                    ...node.attrs,
                    text: null,
                    voiceId: generateVoiceId(),
                    translateUnfold: true,
                  }, node.content, node.marks))
                } else if (node.content && node.content.size > 0) {
                  const newContent = transformFragment(node.content)
                  if (newContent !== node.content) {
                    newNodes.push(node.copy(newContent))
                  } else {
                    newNodes.push(node)
                  }
                } else {
                  newNodes.push(node)
                }
              }
              return Fragment.from(newNodes)
            }
            const newContent = transformFragment(slice.content)
            if (!modified) return slice
            return new Slice(newContent, slice.openStart, slice.openEnd)
          },
        },
      }),
      new Plugin({
        key: new PluginKey('voiceBlockPasteId'),
        appendTransaction(transactions, oldState, newState) {
          const isPaste = transactions.some((t) => t.getMeta('paste'))
          if (!isPaste) return null

          const seen = new Set()
          let tr = newState.tr
          let modified = false

          newState.doc.descendants((node, pos) => {
            if (node.type.name !== 'voiceBlock') return
            const id = node.attrs.voiceId
            if (seen.has(id)) {
              const newId = generateVoiceId()
              tr = tr.setNodeMarkup(pos, undefined, { ...node.attrs, voiceId: newId })
              modified = true
            } else {
              seen.add(id)
            }
          })

          return modified ? tr : null
        },
      }),
    ]
  },
})
