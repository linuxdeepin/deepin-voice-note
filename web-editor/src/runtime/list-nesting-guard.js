// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Extension } from '@tiptap/core'

import { isInListItem, liftActiveListItem, sinkActiveListItem } from './list-behavior.js'

// 统一列表/待办列表键盘缩进规则：Tab 最多进入三层，Shift+Tab 反缩进。
export const ListNestingGuard = Extension.create({
  name: 'listNestingGuard',
  priority: 1000,

  addKeyboardShortcuts() {
    return {
      Tab: () => {
        if (!isInListItem(this.editor)) return false
        return sinkActiveListItem(this.editor)
      },
      'Shift-Tab': () => {
        if (!isInListItem(this.editor)) return false
        return liftActiveListItem(this.editor)
      },
    }
  },
})
