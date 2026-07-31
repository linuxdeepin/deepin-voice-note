// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Node, mergeAttributes } from '@tiptap/core'
import { Plugin, PluginKey } from '@tiptap/pm/state'
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
  const minutes = Math.floor(totalSeconds / 60)
  const seconds = totalSeconds % 60
  return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`
}

/**
 * 由节点 attrs 构造发给宿主的 voiceInfoJson（MetaDataParser 可解析格式）。
 * @param {object} attrs
 * @returns {string}
 */
function buildVoiceInfoJson(attrs) {
  return JSON.stringify({
    dataType: 2,
    voiceId: attrs.voiceId,
    voicePath: attrs.voicePath,
    voiceSize: attrs.voiceSize,
    title: attrs.title,
    createTime: attrs.createTime,
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
      const box = document.createElement('div')
      box.className = 'voiceInfoBox'
      box.setAttribute('data-type', 'voice-block')

      const playback = document.createElement('div')
      playback.className = 'voicePlayback'
      box.appendChild(playback)

      const left = document.createElement('div')
      left.className = 'left'
      const voiceBtn = document.createElement('div')
      voiceBtn.className = 'voiceBtn'
      const titleEl = document.createElement('span')
      titleEl.className = 'title'
      titleEl.textContent = node.attrs.title || ''
      left.appendChild(voiceBtn)
      left.appendChild(titleEl)
      playback.appendChild(left)

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
      const timePassed = document.createElement('span')
      timePassed.className = 'timePassed'
      timePassed.textContent = '00:00'
      const timeTotal = document.createElement('span')
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
      translatingLabel.textContent = '转写中'
      toTextLabel.appendChild(toTextIcon)
      toTextLabel.appendChild(translatingLabel)
      right.appendChild(toTextLabel)

      const toTextTrigger = document.createElement('div')
      toTextTrigger.className = 'voiceToTextTrigger'
      toTextTrigger.textContent = '转文字'
      right.appendChild(toTextTrigger)

      playback.appendChild(right)

      // 转写结果区
      const translate = document.createElement('div')
      translate.className = 'translate'
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

        const unfold = currentNode.attrs.translateUnfold !== false
        translateHeader.classList.toggle('unfold', unfold)
        translateText.style.display = (hasText && unfold) ? '' : 'none'

        // 进度
        const pct = duration > 0 ? Math.min((progress / duration) * 100, 100) : 0
        progressBar.style.setProperty('--progressValue', pct + '%')
        timePassed.textContent = formatTime(progress)
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

      function onFoldToggle() {
        const pos = getPos()
        if (pos == null || pos < 0) return
        view.dispatch(view.state.tr.setNodeMarkup(pos, undefined, {
          ...currentNode.attrs,
          translateUnfold: currentNode.attrs.translateUnfold === false ? true : false,
        }))
      }

      voiceBtn.addEventListener('click', onVoiceBtnClick)
      toTextTrigger.addEventListener('click', onToTextTriggerClick)
      foldBtn.addEventListener('click', onFoldToggle)
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
        dom: box,
        update(updatedNode) {
          if (updatedNode.type.name !== 'voiceBlock') return false
          currentNode = updatedNode
          titleEl.textContent = updatedNode.attrs.title || ''
          translateText.textContent = updatedNode.attrs.text || ''
          refreshState()
          return true
        },
        stopEvent(event) {
          const target = event.target
          if (target instanceof HTMLElement) {
            if (target.closest('.voiceBtn, .progressBar, .voiceToTextTrigger, .foldBtn, .translateHeader, .translateText')) {
              return true
            }
          }
          return false
        },
        ignoreMutation() {
          return true
        },
        selectNode() {
          box.classList.add('ProseMirror-selectednode')
        },
        deselectNode() {
          box.classList.remove('ProseMirror-selectednode')
        },
        destroy() {
          if (destroyed) return
          destroyed = true
          unsubscribe()
          voiceBtn.removeEventListener('click', onVoiceBtnClick)
          toTextTrigger.removeEventListener('click', onToTextTriggerClick)
          foldBtn.removeEventListener('click', onFoldToggle)
        },
      }
    }
  },

  addProseMirrorPlugins() {
    return [
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
