// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONSTATE_H
#define MIGRATIONSTATE_H

#include <QJsonObject>
#include <QVector>
#include <QString>
#include <QMetaType>

// TTP-015: 迁移状态机状态集合（D1=A，不含独立 RollbackRequired）。
// 回滚动作由 TTP-016 在 Failed 态执行，状态机本身不设回滚态。
enum class MigrationState {
    NotNeeded,        // 扫描后确认无需迁移
    Pending,          // 待开始（初始态 / 重置后）
    BackingUp,        // 备份中
    Scanning,         // 扫描中
    Migrating,        // 写回中
    Completed,        // 全部写回成功
    PartialCompleted, // 取消或部分完成（D3 取消映射）
    Failed            // 失败（回滚由 TTP-016 执行）
};

// TTP-015: 环节级续传子阶段（D2=A）。
// BackupDone/ScanDone 标记已完成的环节，续传据此跳过对应环节。
enum class MigrationSubstage {
    None,            // 无子阶段标记
    BackupDone,      // 备份已完成
    ScanDone,        // 扫描已完成（续传不重扫）
    MigratingCursor  // 游标推进中
};

// 跨线程 QueuedConnection 投递 MigrationState（TTP-021 控制器在主线程消费
// stageChanged/terminalInfo）需注册为 metatype。
Q_DECLARE_METATYPE(MigrationState)

// 变更日志条目，随状态同载体持久化（D4 单一载体）。
struct HistoryEntry {
    MigrationState from = MigrationState::Pending;
    MigrationState to = MigrationState::Pending;
    MigrationSubstage substage = MigrationSubstage::None;
    QString reason;
    QString timestamp;   // ISO8601
    QJsonObject cursor;  // 转换发生时的游标快照
};

// 状态名 <-> 枚举转换（持久化与日志用）。
QString migrationStateToString(MigrationState state);
MigrationState migrationStateFromString(const QString &name);
QString migrationSubstageToString(MigrationSubstage sub);
MigrationSubstage migrationSubstageFromString(const QString &name);

// 合法转换判定（纯函数，仅下列边合法，其余返回 false）。
bool canTransition(MigrationState from, MigrationState to);

// 迁移状态机：只管理状态，不读写笔记 DB、不执行备份/扫描/写回/报告/调度。
// 可查询接口供 TTP-019（报告）/TTP-021（UI）只读消费；
// requestTransition/setCursor/markCancelled 由 TTP-020（编排层）驱动。
class MigrationStateMachine
{
public:
    MigrationStateMachine();
    // 指定持久化文件路径（测试注入用）；默认 <AppDataLocation>/migration/migration-state.json
    explicit MigrationStateMachine(const QString &filePath);

    // --- 只读查询接口 ---
    MigrationState currentState() const;
    MigrationSubstage substage() const;
    QJsonObject cursor() const;
    bool isCancelled() const;
    QString updatedAt() const;
    // 处于中间态可续传：BackingUp/Scanning/Migrating/PartialCompleted
    bool isResumable() const;
    QVector<HistoryEntry> history() const;

    // --- 状态推进（TTP-020 驱动）---
    // 非法转换返回 false 并记日志，不抛异常；成功后追加 history 并原子落盘。
    bool requestTransition(MigrationState to, MigrationSubstage sub, const QString &reason);
    // 写游标并原子落盘（TTP-020 每条回报推进游标）。
    void setCursor(const QJsonObject &cursor);
    // D3 取消映射：置 cancelled=true 并转 PartialCompleted（仅 Migrating 可转）。
    void markCancelled();

    // --- 持久化 ---
    // load: 文件缺失视为初始态；解析失败/损坏回退初始态不崩溃，返回 false。
    bool load();
    // save: 临时文件 + rename 原子写（缓解 R2 游标与状态不一致）。
    bool save();

    // 默认持久化文件路径。
    static QString defaultFilePath();

private:
    void resetToInitial();
    QString currentTimestamp() const;

    MigrationState m_state = MigrationState::Pending;
    MigrationSubstage m_substage = MigrationSubstage::None;
    QJsonObject m_cursor;
    bool m_cancelled = false;
    QString m_updatedAt;
    QVector<HistoryEntry> m_history;
    QString m_filePath;
};

#endif // MIGRATIONSTATE_H
