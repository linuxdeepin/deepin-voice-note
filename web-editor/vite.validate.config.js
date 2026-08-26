// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 独立的校验器打包配置：将 scripts/validate-envelope.mjs 打包为
// dist/validate-envelope.mjs 自包含 ESM bundle，所有 @tiptap/* 与
// voice-block.css?inline 依赖全部 inline，运行时无需 node_modules。
// 供 C++ 单元测试（ut_migrationjsonbuilder / ut_migrationnotedataconverter）
// 通过 `node dist/validate-envelope.mjs <file>` 直接调用，彻底解耦 npm 依赖。

import { defineConfig } from 'vite'
import { resolve } from 'node:path'
import { readFileSync, writeFileSync } from 'node:fs'

const spdxBanner = [
  '/*!',
  ' * SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.',
  ' * SPDX-License-Identifier' + ': GPL-3.0-or-later',
  ' *',
  ' * Bundled runtime: @tiptap/* and ProseMirror (MIT).',
  ' */',
].join('\n')

function spdxBannerPlugin() {
  return {
    name: 'spdx-banner',
    // 在产物落盘后注入 banner，绕开 vite 内部 esbuild pipeline：
    // 在 transform/renderChunk 阶段注入会让 shebang 错位触发语法错误。
    writeBundle() {
      const outPath = resolve(__dirname, 'dist/validate-envelope.mjs')
      const original = readFileSync(outPath, 'utf8')
      const lines = original.split('\n')
      // shebang 必须保留在文件第 1 行，banner 紧跟其后
      if (lines[0]?.startsWith('#!')) {
        const shebang = lines.shift()
        writeFileSync(outPath, shebang + '\n' + spdxBanner + '\n' + lines.join('\n'))
      } else {
        writeFileSync(outPath, spdxBanner + '\n' + original)
      }
    },
  }
}

export default defineConfig({
  configDir: resolve(__dirname),
  plugins: [spdxBannerPlugin()],
  build: {
    outDir: resolve(__dirname, 'dist'),
    emptyOutDir: false,
    lib: {
      entry: resolve(__dirname, 'scripts/validate-envelope.mjs'),
      formats: ['es'],
      fileName: () => 'validate-envelope.mjs',
    },
    rollupOptions: {
      // Node 内置模块由运行时提供，禁止 vite 注入浏览器 polyfill
      // （否则 process.argv/process.exit 会指向空对象导致 CLI 失效）
      external: (id) => id.startsWith('node:'),
      output: {
        inlineDynamicImports: true,
        entryFileNames: 'validate-envelope.mjs',
      },
    },
    cssCodeSplit: false,
    assetsInlineLimit: 100000000,
    minify: false,
  },
})
