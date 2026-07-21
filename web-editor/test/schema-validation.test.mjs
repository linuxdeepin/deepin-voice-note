// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import test from 'node:test'
import { fileURLToPath } from 'node:url'

import { createTiptapSchemaV1 } from '../src/schema/create-schema.js'
import {
  createEmptyDoc,
  createEnvelope,
  parseEnvelope,
  serializeEnvelope,
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
  'task-list.json',
  'image.json',
  'voice-block.json',
]

function readFixture(name) {
  return JSON.parse(fs.readFileSync(path.join(fixturesDir, name), 'utf8'))
}

for (const fixture of validFixtures) {
  test(`Schema V1 accepts ${fixture}`, () => {
    const envelope = readFixture(fixture)
    const result = validateEnvelope(envelope)

    assert.equal(result.ok, true, JSON.stringify(result.errors, null, 2))
  })
}

test('Schema V1 exposes the frozen node and mark names', () => {
  const schema = createTiptapSchemaV1()

  assert.deepEqual(Object.keys(schema.nodes).sort(), [...SCHEMA_V1_NODES].sort())
  assert.deepEqual(Object.keys(schema.marks).sort(), [...SCHEMA_V1_MARKS].sort())
})

test('Envelope helpers create and serialize a valid empty document', () => {
  const envelope = createEnvelope()
  const serialized = serializeEnvelope(envelope)
  const parsed = parseEnvelope(serialized)
  const result = validateEnvelope(parsed)

  assert.deepEqual(envelope.content, createEmptyDoc())
  assert.equal(result.ok, true, JSON.stringify(result.errors, null, 2))
})

test('Schema V1 rejects an unknown node', () => {
  const result = validateEnvelope(readFixture('invalid-document.json'))

  assert.equal(result.ok, false)
  assert.equal(result.errors.some(error => error.code === 'unsupported-node'), true)
})

test('Schema V1 rejects a doc without a block node', () => {
  const result = validateEnvelope(createEnvelope({ type: 'doc', content: [] }))

  assert.equal(result.ok, false)
  assert.equal(result.errors.some(error => error.code === 'empty-doc-content'), true)
})

test('Envelope validation rejects unsupported versions', () => {
  const result = validateEnvelope({
    format: 'tiptap',
    schemaVersion: 2,
    content: createEmptyDoc(),
  })

  assert.equal(result.ok, false)
  assert.equal(result.errors.some(error => error.code === 'unsupported-schema-version'), true)
})

test('Envelope validation rejects legacy field names', () => {
  const result = validateEnvelope({
    format: 'tiptap-json',
    version: 1,
    content: createEmptyDoc(),
  })

  assert.equal(result.ok, false)
  assert.equal(result.errors.some(error => error.code === 'invalid-format'), true)
  assert.equal(result.errors.some(error => error.code === 'unsupported-schema-version'), true)
})

test('Voice blocks require stable identity, path and duration', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [
      {
        type: 'voiceBlock',
        attrs: {
          voiceId: '',
          voicePath: '',
          voiceSize: -1,
          translating: false,
        },
      },
    ],
  })
  const result = validateEnvelope(envelope)

  assert.equal(result.ok, false)
  assert.equal(result.errors.some(error => error.code === 'invalid-voice-id'), true)
  assert.equal(result.errors.some(error => error.code === 'invalid-voice-path'), true)
  assert.equal(result.errors.some(error => error.code === 'invalid-voice-size'), true)
  assert.equal(result.errors.some(error => error.code === 'runtime-state-persisted'), true)
})

test('Images reject dangerous URL schemes', () => {
  const envelope = createEnvelope({
    type: 'doc',
    content: [
      {
        type: 'image',
        attrs: { src: 'javascript:alert(1)' },
      },
    ],
  })
  const result = validateEnvelope(envelope)

  assert.equal(result.ok, false)
  assert.equal(result.errors.some(error => error.code === 'unsafe-image-src'), true)
})
