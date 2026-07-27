// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import { defineConfig } from 'vite'
import { resolve } from 'node:path'

// Vite 打包配置，将 Tiptap 运行时入口打包为离线自包含 bundle。
// root 设为 src/runtime 使产物路径扁平：dist/tiptap-editor.html + dist/tiptap-editor.js
// 所有 @tiptap/* 依赖打入 bundle，运行时不联网。

// spdxBannerPlugin 为打包产物注入 SPDX 版权/许可证头，满足 REUSE 合规检查。
const spdxBanner = [
  '/*!',
  ' * SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.',
  ' * SPDX-License-Identifier: GPL-3.0-or-later',
  ' *',
  ' * Bundled runtime: @tiptap/* and ProseMirror (MIT).',
  ' */',
].join('\n')

function spdxBannerPlugin() {
  return {
    name: 'spdx-banner',
    generateBundle(_options, bundle) {
      for (const chunk of Object.values(bundle)) {
        if (chunk.type === 'chunk' && chunk.fileName.endsWith('.js')) {
          chunk.code = spdxBanner + '\n' + chunk.code
        }
      }
    },
  }
}

export default defineConfig({
  root: resolve(__dirname, 'src/runtime'),
  configDir: resolve(__dirname),
  base: './',
  plugins: [spdxBannerPlugin()],
  build: {
    outDir: resolve(__dirname, 'dist'),
    emptyOutDir: true,
    cssCodeSplit: false,
    assetsInlineLimit: 100000000,
    rollupOptions: {
      input: resolve(__dirname, 'src/runtime/tiptap-editor.html'),
      output: {
        entryFileNames: 'tiptap-editor.js',
        chunkFileNames: 'tiptap-editor.js',
        assetFileNames: 'tiptap-editor.[ext]',
        inlineDynamicImports: true,
      },
    },
  },
})
