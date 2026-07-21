// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { createTiptapSchemaV1 } from './create-schema.js'
import {
  SCHEMA_V1_MARKS,
  SCHEMA_V1_NODES,
  TIPTAP_FORMAT,
  TIPTAP_SCHEMA_VERSION,
} from './schema-version.js'

const MAX_NODE_DEPTH = 100
const URL_SCHEME_RE = /^([a-z][a-z0-9+.-]*):/i
const URL_SCHEME_OBFUSCATION_RE = /[\u0000-\u0020\u007f\s]/g
const URL_NETWORK_PATH_RE = /^(?:\/\/|\\\\)/
const SAFE_IMAGE_URL_SCHEMES = new Set(['http', 'https'])

export function createEmptyDoc() {
  return {
    type: 'doc',
    content: [{ type: 'paragraph' }],
  }
}

export function createEnvelope(content = createEmptyDoc()) {
  return {
    format: TIPTAP_FORMAT,
    schemaVersion: TIPTAP_SCHEMA_VERSION,
    content,
  }
}

export function serializeEnvelope(envelope) {
  return JSON.stringify(envelope)
}

export function parseEnvelope(text) {
  return JSON.parse(text)
}

export function validateEnvelope(envelope, schema = createTiptapSchemaV1()) {
  const errors = []

  if (!envelope || typeof envelope !== 'object' || Array.isArray(envelope)) {
    errors.push(error('', 'invalid-envelope', 'Envelope must be an object'))
    return { ok: false, errors }
  }

  if (envelope.format !== TIPTAP_FORMAT) {
    errors.push(error('format', 'invalid-format', `format must be ${TIPTAP_FORMAT}`))
  }

  if (envelope.schemaVersion !== TIPTAP_SCHEMA_VERSION) {
    errors.push(error('schemaVersion', 'unsupported-schema-version', `schemaVersion must be ${TIPTAP_SCHEMA_VERSION}`))
  }

  if (!envelope.content || typeof envelope.content !== 'object' || Array.isArray(envelope.content)) {
    errors.push(error('content', 'invalid-content', 'content must be a ProseMirror document object'))
    return { ok: false, errors }
  }

  if (envelope.content.type !== 'doc') {
    errors.push(error('content.type', 'invalid-root', 'content.type must be doc'))
  }

  if (envelope.content.type === 'doc' && Array.isArray(envelope.content.content) && envelope.content.content.length === 0) {
    errors.push(error('content.content', 'empty-doc-content', 'empty documents must be persisted as one empty paragraph'))
  }

  walkNode(envelope.content, 'content', errors)

  if (!errors.some(error => error.code === 'max-node-depth-exceeded')) {
    try {
      schema.nodeFromJSON(envelope.content)
    } catch (err) {
      errors.push(error('content', 'schema-validation-failed', err.message))
    }
  }

  return { ok: errors.length === 0, errors }
}

function walkNode(node, path, errors, depth = 0) {
  if (depth > MAX_NODE_DEPTH) {
    errors.push(error(path, 'max-node-depth-exceeded', `node depth must not exceed ${MAX_NODE_DEPTH}`))
    return
  }

  if (!node || typeof node !== 'object' || Array.isArray(node)) {
    errors.push(error(path, 'invalid-node', 'node must be an object'))
    return
  }

  if (!SCHEMA_V1_NODES.includes(node.type)) {
    errors.push(error(`${path}.type`, 'unsupported-node', `unsupported node type: ${node.type}`))
  }

  if (node.type === 'text') {
    if (typeof node.text !== 'string') {
      errors.push(error(`${path}.text`, 'invalid-text', 'text node requires string text'))
    }
  }

  if (node.type === 'heading') {
    const level = node.attrs?.level
    if (!Number.isInteger(level) || level < 1 || level > 6) {
      errors.push(error(`${path}.attrs.level`, 'invalid-heading-level', 'heading level must be 1..6'))
    }
  }

  if (node.type === 'image') {
    const src = node.attrs?.src
    if (typeof src !== 'string' || src.length === 0) {
      errors.push(error(`${path}.attrs.src`, 'invalid-image-src', 'image src is required'))
    } else if (!isSafeImageSrc(src)) {
      errors.push(error(`${path}.attrs.src`, 'unsafe-image-src', 'image src uses an unsafe URL scheme'))
    }
  }

  if (node.type === 'voiceBlock') {
    if (!nonEmptyString(node.attrs?.voiceId)) {
      errors.push(error(`${path}.attrs.voiceId`, 'invalid-voice-id', 'voiceId is required'))
    }
    if (!nonEmptyString(node.attrs?.voicePath)) {
      errors.push(error(`${path}.attrs.voicePath`, 'invalid-voice-path', 'voicePath is required'))
    }
    if (typeof node.attrs?.voiceSize !== 'number' || node.attrs.voiceSize < 0) {
      errors.push(error(`${path}.attrs.voiceSize`, 'invalid-voice-size', 'voiceSize must be a non-negative number'))
    }
    if (Object.hasOwn(node.attrs ?? {}, 'translating')) {
      errors.push(error(`${path}.attrs.translating`, 'runtime-state-persisted', 'translating is runtime state and is not part of Schema V1 persistence'))
    }
  }

  for (let markIndex = 0; markIndex < (node.marks ?? []).length; ++markIndex) {
    const mark = node.marks[markIndex]
    const markPath = `${path}.marks[${markIndex}]`
    if (!mark || typeof mark !== 'object' || Array.isArray(mark)) {
      errors.push(error(markPath, 'invalid-mark', 'mark must be an object'))
      continue
    }
    if (!SCHEMA_V1_MARKS.includes(mark.type)) {
      errors.push(error(`${markPath}.type`, 'unsupported-mark', `unsupported mark type: ${mark.type}`))
    }
  }

  if (node.content !== undefined) {
    if (!Array.isArray(node.content)) {
      errors.push(error(`${path}.content`, 'invalid-content-array', 'node content must be an array'))
      return
    }
    for (let index = 0; index < node.content.length; ++index) {
      walkNode(node.content[index], `${path}.content[${index}]`, errors, depth + 1)
    }
  }
}

function isSafeImageSrc(src) {
  const trimmed = src.trim()
  if (trimmed.length === 0) {
    return false
  }

  const normalized = trimmed.replace(URL_SCHEME_OBFUSCATION_RE, '')
  if (normalized.length === 0 || URL_NETWORK_PATH_RE.test(normalized)) {
    return false
  }

  const scheme = normalized.match(URL_SCHEME_RE)?.[1]?.toLowerCase()
  return !scheme || SAFE_IMAGE_URL_SCHEMES.has(scheme)
}

function nonEmptyString(value) {
  return typeof value === 'string' && value.trim().length > 0
}

function error(path, code, message) {
  return { path, code, message }
}
