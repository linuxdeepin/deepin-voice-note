// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 固定格式的取色板与字号档位常量，供格式工具栏与测试引用。
// 颜色取值对齐当前 Summernote 工具栏的 simpleLight/simpleDark 颜色板，
// 维持 5 列 × 2 行的轻量调色板；背景色第一个 transparent 色块表示清除背景色。

export const LIGHT_FORE_COLORS = [
  ['rgb(0, 0, 0)', 'rgb(158, 164, 170)', 'rgb(24, 4, 255)', 'rgb(1, 100, 255)', 'rgb(51, 174, 203)'],
  ['rgb(0, 170, 73)', 'rgb(174, 7, 227)', 'rgb(205, 35, 62)', 'rgb(236, 128, 0)', 'rgb(135, 69, 13)'],
]

export const LIGHT_BACK_COLORS = [
  ['transparent', 'rgba(255, 215, 0, 0.2)', 'rgba(254, 175, 64, 0.2)', 'rgba(109, 255, 0, 0.2)', 'rgba(134, 201, 106, 0.2)'],
  ['rgba(114, 228, 255, 0.2)', 'rgba(130, 178, 255, 0.2)', 'rgba(163, 94, 255, 0.2)', 'rgba(217, 101, 119, 0.2)', 'rgba(192, 192, 192, 0.2)'],
]

export const DARK_FORE_COLORS = [
  ['rgb(220, 220, 220)', 'rgb(141, 147, 152)', 'rgb(177, 157, 255)', 'rgb(77, 146, 255)', 'rgb(112, 198, 218)'],
  ['rgb(102, 204, 145)', 'rgb(214, 131, 241)', 'rgb(215, 79, 100)', 'rgb(239, 153, 51)', 'rgb(251, 215, 103)'],
]

export const DARK_BACK_COLORS = [
  ['transparent', 'rgba(251, 212, 0, 0.2)', 'rgba(255, 148, 0, 0.2)', 'rgba(168, 255, 102, 0.2)', 'rgba(112, 255, 51, 0.2)'],
  ['rgba(102, 228, 255, 0.2)', 'rgba(51, 129, 255, 0.2)', 'rgba(197, 153, 255, 0.2)', 'rgba(248, 124, 143, 0.2)', 'rgba(255, 255, 255, 0.2)'],
]

export const FORE_COLORS = LIGHT_FORE_COLORS
export const BACK_COLORS = LIGHT_BACK_COLORS

const THEME_COLOR_PALETTES = Object.freeze({
  light: Object.freeze({ foreColor: LIGHT_FORE_COLORS, backColor: LIGHT_BACK_COLORS }),
  dark: Object.freeze({ foreColor: DARK_FORE_COLORS, backColor: DARK_BACK_COLORS }),
})

export function colorPaletteForTheme(kind, theme) {
  const paletteTheme = theme === 'dark' ? 'dark' : 'light'
  return THEME_COLOR_PALETTES[paletteTheme][kind] || []
}

export const FONT_SIZES = ['8', '9', '10', '11', '12', '14', '18', '24', '36']

// 字号数值转 Npx 应用值，与 formatted-text.json 口径一致
export function toPxSize(value) {
  return `${value}px`
}
