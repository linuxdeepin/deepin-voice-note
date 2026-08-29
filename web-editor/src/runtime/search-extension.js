// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'
import { Plugin, PluginKey } from '@tiptap/pm/state'
import { Decoration, DecorationSet } from '@tiptap/pm/view'
import { notifySearchState } from './search-state.js'

if (typeof document !== 'undefined' && !document.getElementById('dvn-search-style')) {
  const style = document.createElement('style')
  style.id = 'dvn-search-style'
  style.textContent = `
    .dvn-search-match { background: var(--dvn-search-match-bg, rgba(255, 214, 0, 0.55)); border-radius: 2px; }
    .dvn-search-current { background: var(--dvn-search-current-bg, rgba(255, 150, 0, 0.75)); }
    .dvn-search-voice-match .voiceInfoBox { outline: 2px solid var(--dvn-search-match-outline, rgba(255, 214, 0, 0.75)); outline-offset: 1px; }
    .dvn-search-current-voice .voiceInfoBox { outline-color: var(--dvn-search-current-outline, rgba(255, 150, 0, 0.95)); }
  `
  document.head.appendChild(style)
}

export const searchPluginKey = new PluginKey('dvnSearch')

function normalize(value) {
  return String(value ?? '').toLocaleLowerCase()
}

function findInlineMatches(text, query) {
  const matches = []
  if (!text || !query) return matches
  const haystack = normalize(text)
  const needle = normalize(query)
  let index = haystack.indexOf(needle)
  while (index >= 0) {
    matches.push({ fromOffset: index, toOffset: index + query.length })
    index = haystack.indexOf(needle, index + Math.max(1, query.length))
  }
  return matches
}

function buildSearchState(doc, query, currentIndex = 0) {
  const normalizedQuery = String(query ?? '').trim()
  if (!normalizedQuery) {
    return {
      query: '',
      matches: [],
      currentIndex: 0,
      decorationSet: DecorationSet.empty,
      matchedVoiceIds: new Set(),
      matchedVoiceTranscriptIds: new Set(),
      currentVoiceId: null,
    }
  }

  const decorations = []
  const matches = []
  const matchedVoiceIds = new Set()
  const matchedVoiceTranscriptIds = new Set()

  doc.descendants((node, pos) => {
    if (node.isText) {
      for (const match of findInlineMatches(node.text, normalizedQuery)) {
        const from = pos + match.fromOffset
        const to = pos + match.toOffset
        const index = matches.length
        matches.push({ type: 'text', from, to })
        decorations.push(Decoration.inline(from, to, {
          class: index === currentIndex
            ? 'dvn-search-match dvn-search-current'
            : 'dvn-search-match',
        }))
      }
      return false
    }

    if (node.type?.name === 'voiceBlock') {
      const voiceId = node.attrs?.voiceId
      const titleMatched = normalize(node.attrs?.title).includes(normalize(normalizedQuery))
      const transcriptMatched = normalize(node.attrs?.text).includes(normalize(normalizedQuery))
      if (voiceId && (titleMatched || transcriptMatched)) {
        const index = matches.length
        matchedVoiceIds.add(voiceId)
        if (transcriptMatched) matchedVoiceTranscriptIds.add(voiceId)
        matches.push({ type: 'voiceBlock', from: pos, to: pos + node.nodeSize, voiceId })
        decorations.push(Decoration.node(pos, pos + node.nodeSize, {
          class: index === currentIndex
            ? 'dvn-search-voice-match dvn-search-current-voice'
            : 'dvn-search-voice-match',
        }))
      }
      return false
    }
    return true
  })

  const safeCurrentIndex = matches.length === 0
    ? 0
    : Math.max(0, Math.min(currentIndex, matches.length - 1))
  const current = matches[safeCurrentIndex]

  return {
    query: normalizedQuery,
    matches,
    currentIndex: safeCurrentIndex,
    decorationSet: DecorationSet.create(doc, decorations),
    matchedVoiceIds,
    matchedVoiceTranscriptIds,
    currentVoiceId: current?.voiceId ?? null,
  }
}

function scrollCurrentIntoView(editor) {
  const state = searchPluginKey.getState(editor.state)
  if (!state?.query || state.matches.length === 0) return
  window.requestAnimationFrame(() => {
    const current = editor.view.dom.querySelector('.dvn-search-current, .dvn-search-current-voice')
      || editor.view.dom.querySelector('.dvn-search-match, .dvn-search-voice-match')
    current?.scrollIntoView?.({ block: 'center', inline: 'nearest' })
  })
}

export function setTiptapSearchQuery(editor, query) {
  if (!editor?.view) return false
  editor.view.dispatch(editor.state.tr.setMeta(searchPluginKey, {
    type: 'setQuery',
    query: String(query ?? ''),
  }))
  scrollCurrentIntoView(editor)
  return true
}

export function clearTiptapSearch(editor) {
  return setTiptapSearchQuery(editor, '')
}

export const SearchExtension = Extension.create({
  name: 'dvnSearch',

  addProseMirrorPlugins() {
    return [new Plugin({
      key: searchPluginKey,
      state: {
        init(_, state) {
          return buildSearchState(state.doc, '')
        },
        apply(tr, previous, _oldState, newState) {
          const meta = tr.getMeta(searchPluginKey)
          if (meta?.type === 'setQuery') {
            return buildSearchState(newState.doc, meta.query, 0)
          }
          if (tr.docChanged && previous?.query) {
            return buildSearchState(newState.doc, previous.query, previous.currentIndex)
          }
          if (tr.mapping && previous?.decorationSet) {
            return {
              ...previous,
              decorationSet: previous.decorationSet.map(tr.mapping, tr.doc),
            }
          }
          return previous
        },
      },
      props: {
        decorations(state) {
          return searchPluginKey.getState(state)?.decorationSet || DecorationSet.empty
        },
      },
      view(view) {
        let lastState = null
        const publish = () => {
          const state = searchPluginKey.getState(view.state)
          if (state === lastState) return
          lastState = state
          notifySearchState(state)
        }
        publish()
        return {
          update() { publish() },
          destroy() { notifySearchState(buildSearchState(view.state.doc, '')) },
        }
      },
    })]
  },
})
