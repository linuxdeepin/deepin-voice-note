// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { getSchema } from '@tiptap/core'
import Document from '@tiptap/extension-document'
import Paragraph from '@tiptap/extension-paragraph'
import Text from '@tiptap/extension-text'
import HardBreak from '@tiptap/extension-hard-break'
import Heading from '@tiptap/extension-heading'
import Blockquote from '@tiptap/extension-blockquote'
import BulletList from '@tiptap/extension-bullet-list'
import OrderedList from '@tiptap/extension-ordered-list'
import ListItem from '@tiptap/extension-list-item'
import TaskList from '@tiptap/extension-task-list'
import TaskItem from '@tiptap/extension-task-item'
import Bold from '@tiptap/extension-bold'
import Italic from '@tiptap/extension-italic'
import Underline from '@tiptap/extension-underline'
import Strike from '@tiptap/extension-strike'
import Highlight from '@tiptap/extension-highlight'

import { ColorMark } from '../extensions/color-mark.js'
import { FontFamilyMark } from '../extensions/font-family-mark.js'
import { FontSizeMark } from '../extensions/font-size-mark.js'
import { ImageBlock } from '../extensions/image-block.js'
import { VoiceBlock } from '../extensions/voice-block.js'

export function createTiptapSchemaV1() {
  return getSchema([
    Document,
    Paragraph,
    Text,
    HardBreak,
    Heading.configure({ levels: [1, 2, 3, 4, 5, 6] }),
    Blockquote,
    BulletList,
    OrderedList,
    ListItem,
    TaskList,
    TaskItem.configure({ nested: true }),
    ImageBlock,
    VoiceBlock,
    Bold,
    Italic,
    Underline,
    Strike,
    ColorMark,
    Highlight.configure({ multicolor: true }),
    FontFamilyMark,
    FontSizeMark,
  ])
}
