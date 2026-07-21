// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import Image from '@tiptap/extension-image'

export const ImageBlock = Image.extend({
  name: 'image',

  addAttributes() {
    return {
      ...this.parent?.(),
      relPath: {
        default: null,
        parseHTML: element => element.getAttribute('data-rel-path'),
        renderHTML: attributes => attributes.relPath
          ? { 'data-rel-path': attributes.relPath }
          : {},
      },
    }
  },
})
