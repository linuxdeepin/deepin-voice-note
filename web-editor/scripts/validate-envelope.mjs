#!/usr/bin/env node
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import fs from 'node:fs'
import process from 'node:process'
import { parseEnvelope, validateEnvelope } from '../src/schema/document-envelope.js'

const input = process.argv[2]
  ? fs.readFileSync(process.argv[2], 'utf8')
  : fs.readFileSync(0, 'utf8')

try {
  const result = validateEnvelope(parseEnvelope(input))
  if (!result.ok) {
    console.error(JSON.stringify(result.errors, null, 2))
    process.exit(1)
  }
  console.log('valid')
} catch (error) {
  console.error(error.message)
  process.exit(1)
}
