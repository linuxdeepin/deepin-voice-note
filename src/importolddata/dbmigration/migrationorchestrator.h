// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONORCHESTRATOR_H
#define MIGRATIONORCHESTRATOR_H

#include "migrationstate.h"  // MigrationState / MigrationSubstage / MigrationStateMachine
#include "migrationwriter.h"  // WriteResult（累计写回回报成员）

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QVector>

#include <atomic>

// TTP-020: 后台全量迁移任务编排层。
//
// 自身不实现备份/扫描/转换/校验/写回/报告/状态记录/进度 UI 中任何一项内部逻辑，
// 只把同组 TTP-015 状态机、TTP-016 备份、TTP-017 扫描、TTP-018 写回、TTP-019 报告
// 按固定顺序串成一条可运行的后台任务链：
//   Pending → BackingUp → Scanning → Migrating → 终态(Completed/PartialCompleted/Failed/NotNeeded)+报告
//
// 驱动状态机流转、维护断点续传游标（与状态同载体原子持久化）、暴露只读进度查询与
// 独立取消通道、决定终态与报告触发时机（D15：仅终态报告，无中间报告）。不接编辑器 UI
// （进度展示归 TTP-021）。
class MigrationOrchestrator : public QObject
{
    Q_OBJECT

public:
    // 对外只读进度快照（TTP-021 契约）。仅 Migrating 阶段计数有意义，
    // 其余阶段计数为 0/占位。
    struct ProgressSnapshot {
        MigrationState stage = MigrationState::Pending;
        int processed = 0;
        int total = 0;
        int success = 0;
        int fail = 0;
    };

    // 默认构造：使用各模块默认路径定位（业务库 / 状态文件 / 备份目录 / 报告目录）。
    explicit MigrationOrchestrator(QObject *parent = nullptr);
    // 注入路径（测试用）：dbPath 指向业务库，stateFilePath 指向 migration-state.json，
    // backupDir / reportDir 指向备份与报告目录。
    MigrationOrchestrator(const QString &dbPath, const QString &stateFilePath,
                          const QString &backupDir, const QString &reportDir,
                          QObject *parent = nullptr);

    // 静态启动入口（D20/T3）：若 currentState()==Pending 或 isResumable() 则在独立
    // QThread 启动 run()（不阻塞调用线程），否则不启动。应用启动 DB 就绪后单行调用。
    static void startIfNeeded();

    // 后台主循环：串行推进各阶段。可在任意线程同步调用（测试直接调用以确定性验证）。
    // 由 startIfNeeded() 在独立 QThread 内触发。
    // 包装 runInternal()：若内部未发 finished（非终态退出），补发 aborted() 以保证
    // 后台 QThread 总是退出回收（P2）。
    void run();

    // 只读进度查询（TTP-021 进度展示消费）。
    ProgressSnapshot progressSnapshot() const;

    // 取消通道（独立通道，不经过进度查询接口）：置原子标志，编排器在阶段入口/条目边界
    // 检查。Migrating 命中→停后续条目→markCancelled() 转 PartialCompleted；
    // BackingUp/Scanning 命中→等当前环节同步返回后转 PartialCompleted 或留中间态待续传。
    void requestCancel();

signals:
    // 进度变更推送（D19 事件推送为主）。每条写回结束发一次。
    void progressChanged(const MigrationOrchestrator::ProgressSnapshot &snapshot);
    // 阶段变更推送。
    void stageChanged(MigrationState stage);
    // 终态信号：finalState 为终态，reportPath 为报告路径（NotNeeded 时为空）。
    void finished(MigrationState finalState, const QString &reportPath);
    // 非终态退出信号：run() 因取消留中间态待续传、启动决策无操作、状态转换失败或
    // 测试模拟中断而未进入终态时发出。startIfNeeded() 据此退出后台 QThread
    // （P2：保证线程与 orchestrator 总是回收，不留空转事件循环）。
    void aborted();

protected:
    // 测试钩子：在 Migrating 阶段每处理完一条后调用，返回 true 时模拟进程中断——
    // 立即停止循环且不做终态处理，状态留在 Migrating、游标保留 nextIndex，供续传测试。
    // 默认实现始终返回 false（生产不中断）。
    virtual bool simulateInterrupt(int processedCount);

private:
    // run() 的内部实现，含多个提前返回路径（终态路径经 finalize 发 finished，
    // 非终态路径直接返回由 run() 补发 aborted()）。
    void runInternal();

    // 是否应当启动：Pending 或可续传（BackingUp/Scanning/Migrating/PartialCompleted）。
    bool shouldRun() const;

    // 阶段推进。返回 false 表示该阶段已处理终态或非终态退出（runInternal 据此提前返回）。
    bool doBackingUp();
    bool doScanning();
    bool doMigrating();

    // 终态处理：按需回滚（仅 Failed）+ 组装报告（NotNeeded 不出报告）+ 发 finished。
    void finalize(MigrationState finalState);

    // 游标读写（与状态同载体原子持久化，schema 见实现）。
    void persistCursor();
    void restoreCursorFromState();

    // 进度快照内部更新 + 推送。
    void emitProgress();
    void emitStage(MigrationState stage);

    MigrationStateMachine m_state;
    QString m_dbPath;
    QString m_backupDir;
    QString m_reportDir;

    std::atomic<bool> m_cancelRequested { false };
    // finalize 发 finished 后置 true，run() 据此判断是否补发 aborted()（P2）。
    std::atomic<bool> m_finishedEmitted { false };

    // 运行期数据（部分从游标恢复，部分在阶段中累积）。
    QString m_backupPath;                 // 当前备份路径（游标 backupPath）
    QVector<qint32> m_noteIds;            // 待迁移 note_id 清单（游标 noteIds）
    int m_nextIndex = 0;                  // 下一条待处理索引（游标 nextIndex）
    int m_processed = 0;
    int m_success = 0;
    int m_fail = 0;
    int m_total = 0;

    // 扫描计数（报告用，从游标 scan 恢复或扫描阶段填充）。
    int m_scanTotalCount = 0;
    int m_scanNeedMigrateCount = 0;
    int m_scanAlreadyTiptapCount = 0;
    int m_scanAbnormalCount = 0;
    QVector<qint32> m_scanAbnormalNoteIds;

    // 累计写回回报（含 warnings，报告用）。
    // 注意（Info）：游标 schema 不含 writeResults，故续传运行的终态报告仅含本次续传段
    // 的 writeResults，首段已落盘写回不在内；counts.needMigrate/total 来自游标 scan 为全量，
    // success/failed/failedNoteIds 仅反映续传段——两者口径不一致，属断点续传固有限制。
    QVector<WriteResult> m_writeResults;

    ProgressSnapshot m_snapshot;

    QElapsedTimer m_timer;
};

Q_DECLARE_METATYPE(MigrationOrchestrator::ProgressSnapshot)

#endif // MIGRATIONORCHESTRATOR_H
