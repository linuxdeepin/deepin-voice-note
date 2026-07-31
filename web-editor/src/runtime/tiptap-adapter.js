// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 兼容适配层 — 宿主资源路径 ↔ 前端插入节点所需 src/data 衔接。
// 只读复用 jscontent.cpp 的 WEB_PATH / images/ 相对路径约定，不改 JsContent 源码。

/**
 * 将宿主下发的相对路径（如 "images/xxx.png"）拼接为前端可访问的绝对 file:// URL。
 * @param {string} resourceBaseUrl — 宿主资源根 URL（file:// + WEB_PATH）
 * @param {string} relPath — 相对路径（如 "images/xxx.png"）
 * @returns {string} 完整 file:// URL
 */
export function resolveResourceUrl(resourceBaseUrl, relPath) {
  if (!relPath) return ''
  // 已经是绝对 URL（file://, http://, https://, data:）则原样返回
  if (/^[a-z]+:\/\//i.test(relPath) || relPath.startsWith('data:')) {
    return relPath
  }
  const base = resourceBaseUrl.replace(/\/+$/, '')
  const rel = relPath.replace(/^\/+/, '')
  return `${base}/${rel}`
}

/**
 * 解析宿主下发的 imageInfoJson，返回前端插入图片节点所需的 attrs。
 * @param {string} imageInfoJson — JSON 字符串，如 {"relPath":"images/xxx.png"}
 * @param {string} resourceBaseUrl — 宿主资源根 URL
 * @returns {{ ok: true, attrs: object } | { ok: false, reason: string }}
 */
export function parseImageInfo(imageInfoJson, resourceBaseUrl) {
  let info
  try {
    info = JSON.parse(imageInfoJson)
  } catch {
    return { ok: false, reason: 'invalid imageInfoJson: JSON parse failed' }
  }

  const relPath = info?.relPath
  if (!relPath || typeof relPath !== 'string') {
    return { ok: false, reason: 'invalid imageInfoJson: relPath is required' }
  }

  return {
    ok: true,
    attrs: {
      src: resolveResourceUrl(resourceBaseUrl, relPath),
      relPath,
      alt: info.alt ?? null,
      title: info.title ?? null,
    },
  }
}

/**
 * 解析宿主下发的 voiceInfoJson，返回前端插入语音块节点所需的 attrs。
 * @param {string} voiceInfoJson — JSON 字符串
 * @param {string} resourceBaseUrl — 宿主资源根 URL
 * @returns {{ ok: true, attrs: object } | { ok: false, reason: string }}
 */
export function parseVoiceInfo(voiceInfoJson, resourceBaseUrl) {
  let info
  try {
    info = JSON.parse(voiceInfoJson)
  } catch {
    return { ok: false, reason: 'invalid voiceInfoJson: JSON parse failed' }
  }

  const voiceId = info?.voiceId
  if (!voiceId || typeof voiceId !== 'string') {
    return { ok: false, reason: 'invalid voiceInfoJson: voiceId is required' }
  }

  const voicePath = info?.voicePath
  if (!voicePath || typeof voicePath !== 'string') {
    return { ok: false, reason: 'invalid voiceInfoJson: voicePath is required' }
  }

  return {
    ok: true,
    attrs: {
      voiceId,
      voicePath,
      voiceSize: typeof info.voiceSize === 'number' ? info.voiceSize : 0,
      createTime: info.createTime ?? null,
      title: info.title ?? null,
      text: info.text ?? null,
    },
  }
}
