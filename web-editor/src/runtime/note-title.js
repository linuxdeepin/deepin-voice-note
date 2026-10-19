// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

const LEGACY_GENERATED_NOTE_TITLE = /^未命名文本\d+$/

function escapeRegExp(value) {
  return String(value).replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
}

export function normalizeNoteTitle(value) {
  return String(value ?? '').replace(/[\r\n]+/g, ' ').trim().slice(0, 24)
}

export function sanitizeNoteTitleInput(value) {
  return String(value ?? '').replace(/[\r\n]+/g, ' ').slice(0, 24)
}

export function isGeneratedNoteTitle(value, generatedTitlePrefix = '文本') {
  const normalized = String(value ?? '')
  if (!normalized) return false
  if (LEGACY_GENERATED_NOTE_TITLE.test(normalized)) return true

  const prefix = String(generatedTitlePrefix ?? '').trim()
  if (!prefix) return false
  return new RegExp(`^${escapeRegExp(prefix)}\\d+$`).test(normalized)
}

export function noteTitlePresentation(value, generatedTitlePrefix = '文本') {
  const normalized = normalizeNoteTitle(value)
  const generated = isGeneratedNoteTitle(normalized, generatedTitlePrefix)
  return {
    normalized,
    value: generated ? '' : normalized,
    placeholder: generated ? normalized : '',
    isCustom: Boolean(normalized) && !generated,
  }
}
