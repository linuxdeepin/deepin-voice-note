// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

const subscribers = new Set()
let currentSearchState = {
  query: '',
  matchedVoiceIds: new Set(),
  matchedVoiceTranscriptIds: new Set(),
  currentVoiceId: null,
  version: 0,
}

export function getCurrentSearchState() {
  return currentSearchState
}

export function subscribeSearchState(callback) {
  if (typeof callback !== 'function') return () => {}
  subscribers.add(callback)
  callback(currentSearchState)
  return () => subscribers.delete(callback)
}

export function notifySearchState(state) {
  currentSearchState = {
    query: String(state?.query ?? ''),
    matchedVoiceIds: state?.matchedVoiceIds instanceof Set ? state.matchedVoiceIds : new Set(state?.matchedVoiceIds || []),
    matchedVoiceTranscriptIds: state?.matchedVoiceTranscriptIds instanceof Set
      ? state.matchedVoiceTranscriptIds
      : new Set(state?.matchedVoiceTranscriptIds || []),
    currentVoiceId: state?.currentVoiceId ?? null,
    version: (currentSearchState.version || 0) + 1,
  }
  for (const callback of Array.from(subscribers)) {
    callback(currentSearchState)
  }
}

export function renderHighlightedText(element, text, query, className = 'dvn-search-match') {
  if (!element) return
  const source = String(text ?? '')
  const needle = String(query ?? '')
  element.replaceChildren()
  if (!source || !needle) {
    element.textContent = source
    return
  }

  const lowerSource = source.toLocaleLowerCase()
  const lowerNeedle = needle.toLocaleLowerCase()
  let pos = 0
  let match = lowerSource.indexOf(lowerNeedle, pos)
  while (match >= 0) {
    if (match > pos) {
      element.appendChild(document.createTextNode(source.slice(pos, match)))
    }
    const span = document.createElement('span')
    span.className = className
    span.textContent = source.slice(match, match + needle.length)
    element.appendChild(span)
    pos = match + needle.length
    match = lowerSource.indexOf(lowerNeedle, pos)
  }
  if (pos < source.length) {
    element.appendChild(document.createTextNode(source.slice(pos)))
  }
}

export function textMatchesQuery(text, query) {
  const source = String(text ?? '')
  const needle = String(query ?? '')
  return !!needle && source.toLocaleLowerCase().includes(needle.toLocaleLowerCase())
}
