// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// QWebChannel 建立前使用的兜底文案。运行时会由宿主通过 TiptapChannelBridge
// 下发当前系统语言对应的文案，覆盖这里的默认值。
export const DEFAULT_TIPTAP_UI_STRINGS = Object.freeze({
  generatedTitlePrefix: '文本',
  noteTitle: '笔记标题',
  bodyPlaceholder: '请输入正文',
  bold: '粗体',
  italic: '斜体',
  underline: '下划线',
  strikethrough: '删除线',
  bullets: '无序列表',
  numbering: '有序列表',
  taskList: '待办列表',
  heading: '标题',
  headingLevel: '标题%1',
  paragraph: '正文',
  font: '字体',
  fontSize: '字号',
  fontColor: '文字颜色',
  textHighlightColor: '背景颜色',
  moreColors: '更多颜色',
  transparent: '透明',
  quote: '引用',
  increaseIndent: '增加缩进',
  decreaseIndent: '减少缩进',
  recordVoice: '录音',
  insertPicture: '插入图片',
  moreFormats: '更多格式',
  voiceToText: '语音转文字',
  convertingVoiceToText: '正在转为文字',
  toText: '转文字',
})

export function normalizeTiptapUiStrings(value) {
  let source = value
  if (typeof value === 'string') {
    try {
      source = JSON.parse(value)
    } catch {
      source = null
    }
  }

  const normalized = { ...DEFAULT_TIPTAP_UI_STRINGS }
  if (!source || typeof source !== 'object' || Array.isArray(source)) return normalized

  for (const key of Object.keys(DEFAULT_TIPTAP_UI_STRINGS)) {
    if (typeof source[key] === 'string' && source[key].trim()) {
      normalized[key] = source[key]
    }
  }
  return normalized
}

export function formatHeadingLevelLabel(strings, level) {
  const template = strings?.headingLevel || DEFAULT_TIPTAP_UI_STRINGS.headingLevel
  return String(template).replace('%1', String(level))
}
