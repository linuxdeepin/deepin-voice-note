// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Copy support for read-only voice transcription text rendered inside the
// atom voiceBlock NodeView.  ProseMirror selections cannot represent partial
// text inside atom-node DOM, so transcript copy must be driven from the native
// DOM Selection and isolated from ProseMirror's regular copy pipeline.

let lastTranscriptSelection = {
  element: null,
  text: '',
  time: 0,
}

const TRANSCRIPT_SELECTION_CACHE_TTL_MS = 30000

function clearContextTranscript() {
  if (hasDom()) {
    window.__dvnTiptapContextTranscript = null
  }
}

function setContextTranscript(transcript) {
  if (!hasDom()) return null
  window.__dvnTiptapContextTranscript = transcript && document.contains(transcript) ? transcript : null
  return window.__dvnTiptapContextTranscript
}

function hasDom() {
  return typeof window !== 'undefined' && typeof document !== 'undefined'
}

function selectionRangeIntersectsElement(range, element) {
  if (!range || !element) return false
  if (typeof range.intersectsNode === 'function') {
    try {
      return range.intersectsNode(element)
    } catch (_) {
      // Fall through to boundary comparison for engines that throw on detached
      // or non-standard nodes.
    }
  }

  const elementRange = document.createRange()
  elementRange.selectNodeContents(element)
  return range.compareBoundaryPoints(Range.END_TO_START, elementRange) > 0
    && range.compareBoundaryPoints(Range.START_TO_END, elementRange) < 0
}

function clippedRangeText(range, element) {
  if (!selectionRangeIntersectsElement(range, element)) return ''

  const elementRange = document.createRange()
  elementRange.selectNodeContents(element)

  const clipped = range.cloneRange()
  if (clipped.compareBoundaryPoints(Range.START_TO_START, elementRange) < 0) {
    clipped.setStart(elementRange.startContainer, elementRange.startOffset)
  }
  if (clipped.compareBoundaryPoints(Range.END_TO_END, elementRange) > 0) {
    clipped.setEnd(elementRange.endContainer, elementRange.endOffset)
  }
  return clipped.toString()
}

export function selectedTextWithinElement(element, selection = (hasDom() ? window.getSelection?.() : null)) {
  if (!element || !selection || selection.rangeCount === 0 || selection.isCollapsed) return ''

  let text = ''
  for (let i = 0; i < selection.rangeCount; i++) {
    const part = clippedRangeText(selection.getRangeAt(i), element)
    if (!part) continue
    if (text) text += '\n'
    text += part
  }
  return text
}

function closestTranscriptFromNode(node) {
  const element = node && node.nodeType === 1 ? node : node?.parentElement
  return element?.closest?.('.translateText') || null
}

function liveSelectedTranscriptElement(selection) {
  if (!selection || selection.rangeCount === 0 || selection.isCollapsed) return null

  // Only take over keyboard/native copy when the DOM selection belongs to one
  // transcript.  Mixed selections should continue through ProseMirror/WebEngine
  // so normal rich-text copy semantics are preserved.
  const anchorTranscript = closestTranscriptFromNode(selection.anchorNode)
  const focusTranscript = closestTranscriptFromNode(selection.focusNode)
  if (anchorTranscript && anchorTranscript === focusTranscript && document.contains(anchorTranscript)) {
    return anchorTranscript
  }
  return null
}

export function updateTranscriptSelectionCache(selection = (hasDom() ? window.getSelection?.() : null)) {
  if (!hasDom() || !selection || selection.rangeCount === 0 || selection.isCollapsed) return false

  const matches = []
  const directTranscript = liveSelectedTranscriptElement(selection)
  if (directTranscript) {
    const text = selectedTextWithinElement(directTranscript, selection)
    if (text) matches.push({ element: directTranscript, text })
  } else {
    for (const transcript of document.querySelectorAll('.translateText')) {
      const text = selectedTextWithinElement(transcript, selection)
      if (text) matches.push({ element: transcript, text })
    }
  }

  if (matches.length === 0) {
    lastTranscriptSelection = { element: null, text: '', time: 0 }
    return false
  }
  if (matches.length !== 1) return false
  lastTranscriptSelection = { ...matches[0], time: Date.now() }
  return true
}

export function currentTranscriptElement({ useContext = false } = {}) {
  if (!hasDom()) return null

  if (useContext) {
    const transcript = window.__dvnTiptapContextTranscript
    if (transcript && document.contains(transcript)) return transcript
  }

  return liveSelectedTranscriptElement(window.getSelection?.())
}

export function currentTranscriptCopyText({ useContext = false, allowCachedContext = false, allowWholeContext = false, allowCachedRecent = false } = {}) {
  const transcript = currentTranscriptElement({ useContext })
  if (!transcript) {
    if (allowCachedRecent
      && lastTranscriptSelection.text
      && lastTranscriptSelection.element
      && document.contains(lastTranscriptSelection.element)
      && Date.now() - lastTranscriptSelection.time <= TRANSCRIPT_SELECTION_CACHE_TTL_MS) {
      return lastTranscriptSelection.text
    }
    return ''
  }

  const liveText = selectedTextWithinElement(transcript)
  if (liveText) {
    lastTranscriptSelection = { element: transcript, text: liveText, time: Date.now() }
    return liveText
  }

  // QtWebEngine may collapse or move the DOM Selection while opening the native
  // context menu.  The QML menu first records the clicked transcript element;
  // only in that explicit context do we reuse the last non-empty transcript
  // selection, avoiding stale hijacks of normal Ctrl+C/rich-text copy.
  if (allowCachedContext
    && useContext
    && lastTranscriptSelection.element === transcript
    && document.contains(lastTranscriptSelection.element)) {
    return lastTranscriptSelection.text
  }

  // Context-menu fallback: when the user right-clicks transcript text without
  // a usable selection, copy the whole transcript instead of doing nothing.
  // This is intentionally opt-in so Ctrl+C on a collapsed caret does not
  // unexpectedly copy an entire voice transcript.
  if (allowWholeContext && useContext) {
    return transcript.textContent || ''
  }
  return ''
}

function htmlFromPlainText(text) {
  const div = document.createElement('div')
  div.textContent = text
  return div.innerHTML.replace(/\n/g, '<br>')
}

export function writeTranscriptCopyEvent(event, text = currentTranscriptCopyText()) {
  if (!text || !event?.clipboardData) return false
  event.clipboardData.setData('text/plain', text)
  event.clipboardData.setData('text/html', htmlFromPlainText(text))
  event.preventDefault()
  event.stopPropagation()
  return true
}

export function copyTranscriptTextViaBridge(bridge, text = currentTranscriptCopyText({ useContext: true, allowCachedContext: true, allowWholeContext: true, allowCachedRecent: true })) {
  if (!text || !bridge || typeof bridge.jsCopyPlainTextToClipboard !== 'function') return false
  bridge.jsCopyPlainTextToClipboard(text)
  return true
}

export function setContextTranscriptFromEvent(event) {
  const transcript = event?.target?.closest?.('.translateText') || null
  setContextTranscript(transcript)
  if (transcript) {
    updateTranscriptSelectionCache()
  }
  return transcript
}

export function installTranscriptCopyHandler() {
  if (!hasDom()) return () => {}
  const onCopy = (event) => {
    writeTranscriptCopyEvent(event)
  }
  const updateCache = () => {
    updateTranscriptSelectionCache()
  }
  const clearCacheWhenStartingOutsideTranscript = (event) => {
    if (!event.target?.closest?.('.translateText')) {
      lastTranscriptSelection = { element: null, text: '', time: 0 }
      clearContextTranscript()
    }
  }
  const onContextMenu = (event) => {
    setContextTranscriptFromEvent(event)
  }
  document.addEventListener('copy', onCopy, true)
  document.addEventListener('mousedown', clearCacheWhenStartingOutsideTranscript, true)
  document.addEventListener('pointerdown', clearCacheWhenStartingOutsideTranscript, true)
  document.addEventListener('contextmenu', onContextMenu, true)
  document.addEventListener('selectionchange', updateCache, true)
  document.addEventListener('mouseup', updateCache, true)
  document.addEventListener('pointerup', updateCache, true)
  document.addEventListener('touchend', updateCache, true)
  document.addEventListener('keyup', updateCache, true)
  return () => {
    document.removeEventListener('copy', onCopy, true)
    document.removeEventListener('mousedown', clearCacheWhenStartingOutsideTranscript, true)
    document.removeEventListener('pointerdown', clearCacheWhenStartingOutsideTranscript, true)
    document.removeEventListener('contextmenu', onContextMenu, true)
    document.removeEventListener('selectionchange', updateCache, true)
    document.removeEventListener('mouseup', updateCache, true)
    document.removeEventListener('pointerup', updateCache, true)
    document.removeEventListener('touchend', updateCache, true)
    document.removeEventListener('keyup', updateCache, true)
  }
}
