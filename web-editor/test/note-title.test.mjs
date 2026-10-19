// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import fs from 'node:fs'
import test from 'node:test'

import {
  isGeneratedNoteTitle,
  normalizeNoteTitle,
  noteTitlePresentation,
  sanitizeNoteTitleInput,
} from '../src/runtime/note-title.js'

test('系统生成的默认标题作为 placeholder，不作为 textarea 的真实 value', () => {
  assert.deepEqual(noteTitlePresentation('未命名文本12'), {
    normalized: '未命名文本12',
    value: '',
    placeholder: '未命名文本12',
    isCustom: false,
  })
})

test('用户自定义标题保留为真实 value，不显示 placeholder', () => {
  assert.deepEqual(noteTitlePresentation('会议纪要'), {
    normalized: '会议纪要',
    value: '会议纪要',
    placeholder: '',
    isCustom: true,
  })
})

test('空标题归一后为空且不标记为自定义', () => {
  assert.deepEqual(noteTitlePresentation('  '), {
    normalized: '',
    value: '',
    placeholder: '',
    isCustom: false,
  })
})

test('标题归一化和输入清洗保持 24 字符上限，并移除换行', () => {
  assert.equal(normalizeNoteTitle('  第一行\n第二行  '), '第一行 第二行')
  assert.equal(normalizeNoteTitle('1234567890123456789012345'), '123456789012345678901234')
  assert.equal(sanitizeNoteTitleInput('第一行\n第二行'), '第一行 第二行')
  assert.equal(sanitizeNoteTitleInput('1234567890123456789012345'), '123456789012345678901234')
})

test('生成标题识别不把普通标题误判为默认标题', () => {
  assert.equal(isGeneratedNoteTitle('未命名文本1'), true)
  assert.equal(isGeneratedNoteTitle('未命名文本'), false)
  assert.equal(isGeneratedNoteTitle('我的未命名文本1'), false)
  assert.equal(isGeneratedNoteTitle('未命名文本1（副本）'), false)
})

test('生成标题识别使用宿主下发的当前语言前缀', () => {
  assert.equal(isGeneratedNoteTitle('Text12', 'Text'), true)
  assert.equal(isGeneratedNoteTitle('文本12', '文本'), true)
  assert.equal(isGeneratedNoteTitle('TexT12', 'Text'), false)
  assert.equal(isGeneratedNoteTitle('My Text12', 'Text'), false)
  assert.deepEqual(noteTitlePresentation('Text12', 'Text'), {
    normalized: 'Text12',
    value: '',
    placeholder: 'Text12',
    isCustom: false,
  })
})

test('正文和标题输入使用同一个默认前景色作为 caret-color', () => {
  const editorHtml = fs.readFileSync(new URL('../src/runtime/tiptap-editor.html', import.meta.url), 'utf8')
  assert.match(editorHtml, /#note-title-input \{[^\n]*caret-color: var\(--dvn-editor-fg, rgba\(0, 0, 0, 1\)\)/)
  assert.match(editorHtml, /\.ProseMirror \{[^\n]*caret-color: var\(--dvn-editor-fg, rgba\(0, 0, 0, 1\)\)/)
  assert.doesNotMatch(editorHtml, /caret-color: var\(--highlightColor/)
})

test('标题入口确实把生成标题写入 placeholder，而不是 textarea value', () => {
  const editorSource = fs.readFileSync(new URL('../src/runtime/tiptap-editor.js', import.meta.url), 'utf8')
  const editorHtml = fs.readFileSync(new URL('../src/runtime/tiptap-editor.html', import.meta.url), 'utf8')
  assert.match(editorSource, /titleInput\.value = presentation\.value/)
  assert.match(editorSource, /titleInput\.placeholder = presentation\.placeholder/)
  assert.match(editorHtml, /#note-title-input::placeholder \{[\s\S]*opacity: 1;/)
})

test('正文占位符由宿主下发的文案驱动，不在 CSS 中固定单一语言', () => {
  const editorSource = fs.readFileSync(new URL('../src/runtime/tiptap-editor.js', import.meta.url), 'utf8')
  const editorHtml = fs.readFileSync(new URL('../src/runtime/tiptap-editor.html', import.meta.url), 'utf8')
  assert.match(editorHtml, /content: attr\(data-empty-placeholder\)/)
  assert.match(editorHtml, /<div id="app" data-empty-placeholder="[^"]+"><\/div>/)
  assert.match(editorSource, /editor\.view\.dom\.dataset\.emptyPlaceholder = uiStrings\.bodyPlaceholder/)
})
