// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { Node, mergeAttributes } from '@tiptap/core'

export const VoiceBlock = Node.create({
  name: 'voiceBlock',

  group: 'block',
  atom: true,
  selectable: true,
  draggable: true,

  addAttributes() {
    return {
      voiceId: { default: null },
      voicePath: { default: null },
      voiceSize: { default: 0 },
      createTime: { default: null },
      title: { default: null },
      text: { default: null },
      translateUnfold: { default: true },
    }
  },

  parseHTML() {
    return [{ tag: 'div[data-type="voice-block"]' }]
  },

  renderHTML({ HTMLAttributes }) {
    return ['div', mergeAttributes(HTMLAttributes, { 'data-type': 'voice-block' })]
  },
})
