// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONSTATEPERSISTENT_H
#define MIGRATIONSTATEPERSISTENT_H

#include "migrationstate.h"

#include <QString>

// TTP-015: 迁移状态持久化（D4=A，单一载体 migration-state.json）。
// 负责 JSON 序列化与原子写（临时文件 + QFile::rename），不读写笔记 DB。
// 状态逻辑由 MigrationStateMachine 持有，本类仅做载体读写。
class MigrationStatePersistent
{
public:
    explicit MigrationStatePersistent(const QString &filePath);

    // 原子写入：序列化 -> 写 *.tmp -> rename 覆盖目标。目录不存在则 mkpath。
    bool save(MigrationState state, MigrationSubstage substage,
              const QJsonObject &cursor, bool cancelled, const QString &updatedAt,
              const QVector<HistoryEntry> &history);

    // 读取并反序列化到输出参数。
    // 文件缺失：回退初始态，返回 true（合法初始）。
    // 文件存在但解析失败/损坏：回退初始态，返回 false（不崩溃）。
    bool load(MigrationState &state, MigrationSubstage &substage,
              QJsonObject &cursor, bool &cancelled, QString &updatedAt,
              QVector<HistoryEntry> &history);

    QString filePath() const;

private:
    QString m_filePath;
};

#endif // MIGRATIONSTATEPERSISTENT_H
