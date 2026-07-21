// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Mark, mergeAttributes } from '@tiptap/core'

export const FontFamilyMark = Mark.create({
  name: 'fontFamily',

  addAttributes() {
    return {
      fontFamily: {
        default: null,
        parseHTML: element => element.style.fontFamily,
        renderHTML: attributes => attributes.fontFamily
          ? { style: `font-family: ${attributes.fontFamily}` }
          : {},
      },
    }
  },

  parseHTML() {
    return [{ tag: 'span[style*=font-family]' }]
  },

  renderHTML({ HTMLAttributes }) {
    return ['span', mergeAttributes(HTMLAttributes), 0]
  },
})
