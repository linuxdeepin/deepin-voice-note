// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { readFile } from 'node:fs/promises'

// Test-only CSS module loader: returns empty module for .css imports
// so that source files importing stylesheets (bundled by Vite in production)
// don't break under `node --test`.
export async function load(url, context, nextLoad) {
  if (url.endsWith('?raw')) {
    const source = await readFile(new URL(url.slice(0, -4)), 'utf8')
    return {
      format: 'module',
      source: `export default ${JSON.stringify(source)}`,
      shortCircuit: true,
    }
  }
  if (url.endsWith('?url')) {
    return {
      format: 'module',
      source: `export default ${JSON.stringify(new URL(url.slice(0, -4)).href)}`,
      shortCircuit: true,
    }
  }
  if (url.split('?')[0].endsWith('.css')) {
    return {
      format: 'module',
      source: 'export default ""',
      shortCircuit: true,
    }
  }
  return nextLoad(url, context)
}
