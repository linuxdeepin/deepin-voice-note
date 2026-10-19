// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

const GENERATED_NOTE_TITLE = /^未命名文本\d+$/

export function normalizeNoteTitle(value) {
  return String(value ?? '').replace(/[\r\n]+/g, ' ').trim().slice(0, 24)
}

export function sanitizeNoteTitleInput(value) {
  return String(value ?? '').replace(/[\r\n]+/g, ' ').slice(0, 24)
}

export function isGeneratedNoteTitle(value) {
  return GENERATED_NOTE_TITLE.test(String(value ?? ''))
}

export function noteTitlePresentation(value) {
  const normalized = normalizeNoteTitle(value)
  const generated = isGeneratedNoteTitle(normalized)
  return {
    normalized,
    value: generated ? '' : normalized,
    placeholder: generated ? normalized : '',
    isCustom: Boolean(normalized) && !generated,
  }
}
