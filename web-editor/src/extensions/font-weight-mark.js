// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Mark, mergeAttributes } from '@tiptap/core'

function normalizeFontWeight(value) {
  const normalized = String(value ?? '').trim().toLowerCase()
  if (!normalized) return null
  if (normalized === 'normal' || normalized === '400') return 'normal'
  if (normalized === 'bold' || normalized === '700') return '700'
  return normalized
}

export const FontWeightMark = Mark.create({
  name: 'fontWeight',

  addAttributes() {
    return {
      fontWeight: {
        default: null,
        parseHTML: element => normalizeFontWeight(element.style.fontWeight),
        renderHTML: attributes => attributes.fontWeight
          ? { style: `font-weight: ${attributes.fontWeight}` }
          : {},
      },
    }
  },

  parseHTML() {
    return [{ tag: 'span[style*=font-weight]' }]
  },

  renderHTML({ HTMLAttributes }) {
    return ['span', mergeAttributes(HTMLAttributes), 0]
  },
})
