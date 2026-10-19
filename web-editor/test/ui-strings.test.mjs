// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import test from 'node:test'

import {
  DEFAULT_TIPTAP_UI_STRINGS,
  formatHeadingLevelLabel,
  normalizeTiptapUiStrings,
} from '../src/runtime/ui-strings.js'

test('宿主文案 JSON 覆盖默认值并保留缺失项兜底', () => {
  const strings = normalizeTiptapUiStrings(JSON.stringify({
    generatedTitlePrefix: 'Text',
    noteTitle: 'Note title',
    bodyPlaceholder: 'Enter text',
    headingLevel: 'Heading %1',
    bold: 'Bold',
  }))

  assert.equal(strings.generatedTitlePrefix, 'Text')
  assert.equal(strings.noteTitle, 'Note title')
  assert.equal(strings.bodyPlaceholder, 'Enter text')
  assert.equal(strings.bold, 'Bold')
  assert.equal(strings.italic, DEFAULT_TIPTAP_UI_STRINGS.italic)
})

test('非法或空文案回退到完整默认值', () => {
  assert.deepEqual(normalizeTiptapUiStrings('{invalid'), DEFAULT_TIPTAP_UI_STRINGS)
  assert.deepEqual(normalizeTiptapUiStrings({ bodyPlaceholder: '   ' }), DEFAULT_TIPTAP_UI_STRINGS)
})

test('标题级别模板按数字替换', () => {
  assert.equal(formatHeadingLevelLabel({ headingLevel: 'Heading %1' }, 3), 'Heading 3')
  assert.equal(formatHeadingLevelLabel({ headingLevel: '标题%1' }, 5), '标题5')
})
