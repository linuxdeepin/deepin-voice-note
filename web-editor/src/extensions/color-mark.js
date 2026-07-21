// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Mark, mergeAttributes } from '@tiptap/core'

export const ColorMark = Mark.create({
  name: 'color',

  addAttributes() {
    return {
      color: {
        default: null,
        parseHTML: element => element.style.color || element.getAttribute('data-color'),
        renderHTML: attributes => attributes.color ? { style: `color: ${attributes.color}` } : {},
      },
    }
  },

  parseHTML() {
    return [
      {
        tag: 'span[style*=color]',
        getAttrs: element => element.style.color ? null : false,
      },
      { tag: 'font[color]' },
    ]
  },

  renderHTML({ HTMLAttributes }) {
    return ['span', mergeAttributes(HTMLAttributes), 0]
  },
})
