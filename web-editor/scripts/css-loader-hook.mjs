// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ESM loader hook: intercepts Vite-style CSS inline imports so that the
// validator can run under plain Node.js (no bundler). Returns an empty
// string for the CSS module default export.
export async function load(url, context, nextLoad) {
  if (url.includes('.css')) {
    return {
      format: 'module',
      source: 'export default ""',
      shortCircuit: true,
    };
  }
  return nextLoad(url, context);
}
