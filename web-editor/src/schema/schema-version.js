// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

export const TIPTAP_FORMAT = 'tiptap'
export const TIPTAP_SCHEMA_VERSION = 1

export const SCHEMA_V1_NODES = Object.freeze([
  'doc',
  'paragraph',
  'text',
  'hardBreak',
  'heading',
  'blockquote',
  'bulletList',
  'orderedList',
  'listItem',
  'taskList',
  'taskItem',
  'image',
  'voiceBlock',
])

export const SCHEMA_V1_MARKS = Object.freeze([
  'bold',
  'italic',
  'underline',
  'strike',
  'color',
  'highlight',
  'fontFamily',
  'fontSize',
])
