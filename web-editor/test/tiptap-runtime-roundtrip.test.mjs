// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// TTP-022: 前端往返单测。
// 使用 happy-dom 提供 DOM 环境，验证 Tiptap Editor 的 envelope 加载/保存闭环。

import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import test from 'node:test'
import { fileURLToPath } from 'node:url'

import { Window } from 'happy-dom'

import { Editor } from '@tiptap/core'
import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import {
  createEmptyDoc,
  createEnvelope,
  serializeEnvelope,
  parseEnvelope,
  validateEnvelope,
} from '../src/schema/document-envelope.js'
import { SCHEMA_V1_MARKS, SCHEMA_V1_NODES } from '../src/schema/schema-version.js'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const fixturesDir = path.join(__dirname, 'fixtures')

const validFixtures = [
  'empty-document.json',
  'formatted-text.json',
  'heading.json',
  'nested-list.json',
  'ordered-list-start.json',
  'task-list.json',
  'task-list-checked.json',
  'image.json',
  'voice-block.json',
]

// ---------------------------------------------------------------------------
// happy-dom DOM 环境设置
// ---------------------------------------------------------------------------

function defineGlobal(name, value) {
  Object.defineProperty(globalThis, name, {
    value,
    writable: true,
    configurable: true,
  })
}

function setupDom() {
  const window = new Window()
  defineGlobal('window', window)
  defineGlobal('document', window.document)
  defineGlobal('navigator', window.navigator)
  defineGlobal('Node', window.Node)
  defineGlobal('Element', window.Element)
  defineGlobal('HTMLElement', window.HTMLElement)
  defineGlobal('DocumentFragment', window.DocumentFragment)
  defineGlobal('Event', window.Event)
  defineGlobal('CustomEvent', window.CustomEvent)
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('getComputedStyle', window.getComputedStyle.bind(window))
  return window
}

function createEditor() {
  const window = setupDom()
  const element = document.createElement('div')
  document.body.appendChild(element)
  const editor = new Editor({
    element,
    extensions: createTiptapExtensions(),
    content: createEmptyDoc(),
  })
  return { editor, window }
}

// ---------------------------------------------------------------------------
// Schema 漂移断言：Editor 节点/标记名 == SCHEMA_V1_NODES / SCHEMA_V1_MARKS
// ---------------------------------------------------------------------------

test('Editor schema nodes match SCHEMA_V1_NODES', () => {
  const { editor } = createEditor()
  const nodeNames = Object.keys(editor.schema.nodes).sort()
  assert.deepEqual(nodeNames, [...SCHEMA_V1_NODES].sort())
  editor.destroy()
})

test('Editor schema marks match SCHEMA_V1_MARKS', () => {
  const { editor } = createEditor()
  const markNames = Object.keys(editor.schema.marks).sort()
  assert.deepEqual(markNames, [...SCHEMA_V1_MARKS].sort())
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 7 fixture 往返测试：setContent → getJSON → createEnvelope → validateEnvelope
// ---------------------------------------------------------------------------

for (const fixture of validFixtures) {
  test(`roundtrip ${fixture}: load envelope → getJSON → validate`, () => {
    const envelope = JSON.parse(fs.readFileSync(path.join(fixturesDir, fixture), 'utf8'))
    const { editor } = createEditor()

    editor.commands.setContent(envelope.content)
    const json = editor.getJSON()
    const resultEnvelope = createEnvelope(json)
    const result = validateEnvelope(resultEnvelope)

    assert.equal(result.ok, true, JSON.stringify(result.errors, null, 2))
    editor.destroy()
  })
}

// ---------------------------------------------------------------------------
// 空文档：单空段落 envelope
// ---------------------------------------------------------------------------

test('empty document produces single empty paragraph envelope', () => {
  const { editor } = createEditor()
  editor.commands.setContent(createEmptyDoc())
  const json = editor.getJSON()
  const envelope = createEnvelope(json)

  assert.deepEqual(json, { type: 'doc', content: [{ type: 'paragraph' }] })

  const result = validateEnvelope(envelope)
  assert.equal(result.ok, true, JSON.stringify(result.errors, null, 2))
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 保存→重载深相等（D9）：加载 → getJSON → 序列化 → 解析 → 重载 → getJSON → 深相等
// ---------------------------------------------------------------------------

for (const fixture of validFixtures) {
  test(`reload consistency ${fixture}: save → reload → deep equal`, () => {
    const envelope = JSON.parse(fs.readFileSync(path.join(fixturesDir, fixture), 'utf8'))

    // 第一次加载 → 保存
    const { editor: editor1 } = createEditor()
    editor1.commands.setContent(envelope.content)
    const json1 = editor1.getJSON()
    const serialized = serializeEnvelope(createEnvelope(json1))
    editor1.destroy()

    // 重载 → 再保存
    const { editor: editor2 } = createEditor()
    const reloaded = parseEnvelope(serialized)
    editor2.commands.setContent(reloaded.content)
    const json2 = editor2.getJSON()
    editor2.destroy()

    assert.deepEqual(json1, json2)
  })
}
