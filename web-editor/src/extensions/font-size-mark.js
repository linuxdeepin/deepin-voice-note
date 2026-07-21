// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Mark, mergeAttributes } from '@tiptap/core'

export const FontSizeMark = Mark.create({
  name: 'fontSize',

  addAttributes() {
    return {
      fontSize: {
        default: null,
        parseHTML: element => element.style.fontSize,
        renderHTML: attributes => attributes.fontSize
          ? { style: `font-size: ${attributes.fontSize}` }
          : {},
      },
    }
  },

  parseHTML() {
    return [{ tag: 'span[style*=font-size]' }]
  },

  renderHTML({ HTMLAttributes }) {
    return ['span', mergeAttributes(HTMLAttributes), 0]
  },
})
