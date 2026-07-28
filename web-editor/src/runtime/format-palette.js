// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 固定格式的取色板与字号档位常量，供格式工具栏与测试引用。
// 颜色取值与 Summernote 调色板一致（foreColors 用于文字色、colors 用于背景色），
// 统一以小写 hex 存储，与文档 fixture 口径保持一致。

export const FORE_COLORS = [
  ['#000000', '#424242', '#636363', '#414d68', '#c0c6d4', '#d0c6cf', '#efefef', '#ffffff'],
  ['#f6989a', '#ffc395', '#ffe691', '#abd7a0', '#9bc7cf', '#8ec7f3', '#b9a4da', '#e1a2be'],
  ['#e50000', '#f78f12', '#f9c400', '#53a73a', '#337d8e', '#0086cc', '#6848ab', '#b4427d'],
  ['#ae0000', '#c45e00', '#c69200', '#017d02', '#004b5c', '#005399', '#351578', '#800643'],
]

export const BACK_COLORS = [
  ['transparent', '#424242', '#636363', '#414d68', '#c0c6d4', '#d0c6cf', '#efefef', '#ffffff'],
  ['#f6989a', '#ffc395', '#ffe691', '#abd7a0', '#9bc7cf', '#8ec7f3', '#b9a4da', '#e1a2be'],
  ['#e50000', '#f78f12', '#f9c400', '#53a73a', '#337d8e', '#0086cc', '#6848ab', '#b4427d'],
  ['#ae0000', '#c45e00', '#c69200', '#017d02', '#004b5c', '#005399', '#351578', '#000000'],
]

export const FONT_SIZES = ['8', '9', '10', '11', '12', '14', '18', '24', '36']

// 字号数值转 Npx 应用值，与 formatted-text.json 口径一致
export function toPxSize(value) {
  return `${value}px`
}
