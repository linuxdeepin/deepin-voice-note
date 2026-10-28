// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDLOG_H
#define DDLOG_H

#include <QDebug>
#include <QLoggingCategory>

// 全应用统一日志分类：调试模式（deepin-debug-config）按分类动态过滤日志。
// 分类名必须 org.deepin. 前缀，与 desktop 文件名（deepin-voice-note）对应，
// 便于 DConfig log_rules 以 *.voice-note* 通配过滤。
inline Q_LOGGING_CATEGORY(voiceNoteLog, "org.deepin.voice-note")

#endif   // DDLOG_H
