// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Test-only CSS module loader: returns empty module for .css imports
// so that source files importing stylesheets (bundled by Vite in production)
// don't break under `node --test`.
export async function load(url, context, nextLoad) {
  if (url.split('?')[0].endsWith('.css')) {
    return {
      format: 'module',
      source: 'export default ""',
      shortCircuit: true,
    }
  }
  return nextLoad(url, context)
}
