// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VOICENOTE_LOGGER_H
#define VOICENOTE_LOGGER_H

#include <QObject>
#include <QString>
#include <dtkcore_global.h>

#ifdef DTKCORE_CLASS_DConfigFile
DCORE_BEGIN_NAMESPACE
class DConfig;
DCORE_END_NAMESPACE
#endif

// 动态日志规则监听器（参考 deepin-reader 的 MLogger 实现）：
// 1. main() 最前面实例化（任何日志打印、DApplication 创建之前），
//    先读取并清除 QT_LOGGING_RULES 环境变量，避免环境变量规则抢先生效；
// 2. 读取 DConfig(appId=org.deepin.voice-note, resource=org.deepin.voice-note)
//    的 log_rules 配置并应用到 Qt 日志过滤器；
// 3. 监听 DConfig valueChanged，实现运行中动态调整日志级别。
class MLogger : public QObject
{
    Q_OBJECT
public:
    explicit MLogger(QObject *parent = nullptr);
    ~MLogger() override;

    inline QString rules() const { return m_rules; }
    void setRules(const QString &rules);

private:
    void appendRules(const QString &rules);

private:
    QString m_rules;
#ifdef DTKCORE_CLASS_DConfigFile
    Dtk::Core::DConfig *m_config = nullptr;
#endif
};

#endif   // VOICENOTE_LOGGER_H
