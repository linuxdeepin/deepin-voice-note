// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationorchestrator.h"

#include "migrationbackup.h"
#include "migrationreport.h"
#include "migrationscanner.h"
#include "migrationwriter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QThread>

namespace {
Q_LOGGING_CATEGORY(lcMigrationOrchestrator, "voice_note_migration_orchestrator")
}

MigrationOrchestrator::MigrationOrchestrator(QObject *parent)
    : MigrationOrchestrator(MigrationBackup::defaultDbPath(),
                            MigrationStateMachine::defaultFilePath(),
                            MigrationBackup::defaultBackupDir(),
                            MigrationReport::defaultReportDir(),
                            parent)
{
}

MigrationOrchestrator::MigrationOrchestrator(const QString &dbPath, const QString &stateFilePath,
                                             const QString &backupDir, const QString &reportDir,
                                             QObject *parent)
    : QObject(parent)
    , m_state(stateFilePath)
    , m_dbPath(dbPath)
    , m_backupDir(backupDir)
    , m_reportDir(reportDir)
{
    m_state.load();
}

// --- 静态启动入口 ---

void MigrationOrchestrator::startIfNeeded()
{
    MigrationOrchestrator *orchestrator = new MigrationOrchestrator();
    if (!orchestrator->shouldRun()) {
        qCInfo(lcMigrationOrchestrator) << "startIfNeeded: no migration to run, state="
                                        << migrationStateToString(orchestrator->m_state.currentState());
        delete orchestrator;
        return;
    }

    QThread *thread = new QThread();
    orchestrator->moveToThread(thread);
    connect(thread, &QThread::started, orchestrator, &MigrationOrchestrator::run);
    // 终态与非终态退出都退出线程并回收 orchestrator（P2：覆盖不发 finished 的提前返回路径）。
    connect(orchestrator, &MigrationOrchestrator::finished, thread, &QThread::quit);
    connect(orchestrator, &MigrationOrchestrator::aborted, thread, &QThread::quit);
    connect(orchestrator, &MigrationOrchestrator::finished, orchestrator, &MigrationOrchestrator::deleteLater);
    connect(orchestrator, &MigrationOrchestrator::aborted, orchestrator, &MigrationOrchestrator::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    qCInfo(lcMigrationOrchestrator) << "startIfNeeded: launching migration in background thread, state="
                                    << migrationStateToString(orchestrator->m_state.currentState());
    thread->start();
}

bool MigrationOrchestrator::shouldRun() const
{
    const MigrationState state = m_state.currentState();
    return state == MigrationState::Pending || m_state.isResumable();
}

// --- 主循环 ---

void MigrationOrchestrator::run()
{
    // 包装 runInternal()：若内部未发 finished（非终态退出），补发 aborted() 以保证
    // 后台 QThread 总是退出回收（P2）。测试直调 run() 时 aborted 无监听者，无副作用。
    m_finishedEmitted.store(false);
    runInternal();
    if (!m_finishedEmitted.load()) {
        qCInfo(lcMigrationOrchestrator) << "run: non-terminal exit, emit aborted";
        emit aborted();
    }
}

void MigrationOrchestrator::runInternal()
{
    m_timer.start();
    m_state.load();

    MigrationState state = m_state.currentState();
    qCInfo(lcMigrationOrchestrator) << "run: enter, state=" << migrationStateToString(state);

    // 已处成功终态：无需动作。
    if (state == MigrationState::Completed || state == MigrationState::NotNeeded) {
        return;
    }

    // 续传/重试入口：PartialCompleted 可合法回到 Pending 重做（重扫自动排除已迁移笔记）。
    // 注意：BackingUp/Scanning/Migrating 不能直接转 Pending（TTP-015 合法转换边），
    // 故这些中间态就地续传，不回 Pending；仅 PartialCompleted 回 Pending。
    if (state == MigrationState::PartialCompleted) {
        if (!m_state.requestTransition(MigrationState::Pending, MigrationSubstage::None,
                                       QStringLiteral("resume from partial"))) {
            return;
        }
    }

    // 恢复游标数据：仅中间态续传时从游标恢复 backupPath/noteIds/计数。
    // fresh Pending（含 PartialCompleted 回到 Pending 的重试）不恢复，重新备份/扫描，
    // 避免沿用已过期的清单（重扫会自动排除已迁移笔记）。
    if (state == MigrationState::BackingUp
        || state == MigrationState::Scanning
        || state == MigrationState::Migrating) {
        restoreCursorFromState();
    }

    // 阶段 1：BackingUp
    if (!doBackingUp()) {
        return;
    }

    // 阶段 2：Scanning
    if (m_state.currentState() == MigrationState::Scanning) {
        if (!doScanning()) {
            return;
        }
    }

    // 阶段 3：Migrating
    if (m_state.currentState() == MigrationState::Migrating) {
        if (!doMigrating()) {
            return;
        }
    }
}

// --- 阶段：BackingUp ---

bool MigrationOrchestrator::doBackingUp()
{
    MigrationState state = m_state.currentState();

    if (state == MigrationState::Pending) {
        if (m_cancelRequested.load()) {
            qCInfo(lcMigrationOrchestrator) << "doBackingUp: cancelled before start, leave Pending";
            return false;
        }
        if (!m_state.requestTransition(MigrationState::BackingUp, MigrationSubstage::None,
                                       QStringLiteral("backup start"))) {
            finalize(MigrationState::Failed);
            return false;
        }
        emitStage(MigrationState::BackingUp);
    } else if (state == MigrationState::BackingUp) {
        emitStage(MigrationState::BackingUp);
    } else {
        // 已过备份阶段（Scanning/Migrating）：backupPath 已从游标恢复。
        return true;
    }

    if (m_cancelRequested.load()) {
        qCInfo(lcMigrationOrchestrator) << "doBackingUp: cancelled at entry, leave mid-state for resume";
        return false;
    }

    // 续传跳过备份（GAP-4）：BackupDone 已置 或 游标已含 backupPath。
    const bool backupDone = (m_state.substage() == MigrationSubstage::BackupDone)
                            || !m_backupPath.isEmpty();
    if (backupDone) {
        if (!m_state.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone,
                                       QStringLiteral("backup resume skip"))) {
            finalize(MigrationState::Failed);
            return false;
        }
        return true;
    }

    MigrationBackup backup(m_dbPath, m_backupDir);
    const BackupResult result = backup.backup();
    if (!result.success) {
        qCWarning(lcMigrationOrchestrator) << "doBackingUp: backup failed:" << result.message;
        finalize(MigrationState::Failed);
        return false;
    }

    m_backupPath = result.backupPath;
    persistCursor();
    if (!m_state.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone,
                                   QStringLiteral("backup done"))) {
        finalize(MigrationState::Failed);
        return false;
    }
    return true;
}

// --- 阶段：Scanning ---

bool MigrationOrchestrator::doScanning()
{
    emitStage(MigrationState::Scanning);

    if (m_cancelRequested.load()) {
        qCInfo(lcMigrationOrchestrator) << "doScanning: cancelled at entry, leave mid-state for resume";
        return false;
    }

    // 续传跳过扫描（D2）：ScanDone 已置 或 游标已含清单。
    const bool scanDone = (m_state.substage() == MigrationSubstage::ScanDone)
                          || !m_noteIds.isEmpty();
    if (scanDone) {
        if (!m_state.requestTransition(MigrationState::Migrating, MigrationSubstage::MigratingCursor,
                                       QStringLiteral("migrate start (scan resume)"))) {
            finalize(MigrationState::Failed);
            return false;
        }
        return true;
    }

    MigrationScanner scanner(m_dbPath);
    const ScanResult result = scanner.scan();
    if (!result.success) {
        qCWarning(lcMigrationOrchestrator) << "doScanning: scan failed/aborted:" << result.message;
        finalize(MigrationState::Failed);
        return false;
    }

    m_scanTotalCount = result.totalCount;
    m_scanNeedMigrateCount = result.needMigrateCount;
    m_scanAlreadyTiptapCount = result.alreadyTiptapCount;
    m_scanAbnormalCount = result.abnormalCount;
    m_scanAbnormalNoteIds.clear();
    for (const ScanAbnormal &ab : result.abnormals) {
        m_scanAbnormalNoteIds.append(ab.noteId);
    }
    m_noteIds = result.needMigrateNoteIds;
    m_total = m_noteIds.size();
    m_nextIndex = 0;
    m_processed = 0;
    m_success = 0;
    m_fail = 0;

    if (result.needMigrateCount == 0) {
        if (result.abnormalCount > 0) {
            // D7：有异常但无需迁移 → Failed。
            persistCursor();
            finalize(MigrationState::Failed);
            return false;
        }
        // 无需迁移 → NotNeeded（不出报告、不写回）。
        m_state.requestTransition(MigrationState::NotNeeded, MigrationSubstage::None,
                                  QStringLiteral("not needed"));
        finalize(MigrationState::NotNeeded);
        return false;
    }

    persistCursor();
    if (!m_state.requestTransition(MigrationState::Migrating, MigrationSubstage::MigratingCursor,
                                   QStringLiteral("migrate start"))) {
        finalize(MigrationState::Failed);
        return false;
    }
    return true;
}

// --- 阶段：Migrating ---

bool MigrationOrchestrator::doMigrating()
{
    emitStage(MigrationState::Migrating);

    m_snapshot.stage = MigrationState::Migrating;
    m_snapshot.total = m_total;
    m_snapshot.processed = m_processed;
    m_snapshot.success = m_success;
    m_snapshot.fail = m_fail;
    emitProgress();

    MigrationWriter writer(m_dbPath);
    for (int i = m_nextIndex; i < m_noteIds.size(); ++i) {
        // 条目边界检查取消（D3）：不中断进行中的 writeOne（同步无取消钩子）。
        if (m_cancelRequested.load()) {
            qCInfo(lcMigrationOrchestrator) << "doMigrating: cancelled at item boundary" << i;
            m_state.markCancelled();  // Migrating → PartialCompleted，置 cancelled=true
            finalize(MigrationState::PartialCompleted);
            return false;
        }

        const qint32 noteId = m_noteIds[i];
        const WriteResult wr = writer.writeOne(0, noteId);  // folderId=0（IF-1）
        if (wr.success) {
            ++m_success;
        } else {
            ++m_fail;
            qCWarning(lcMigrationOrchestrator) << "doMigrating: writeOne failed for note" << noteId
                                               << ":" << wr.message;
        }
        m_writeResults.append(wr);

        m_nextIndex = i + 1;
        m_processed = m_nextIndex;
        persistCursor();  // 原子落盘，支撑进程异常退出后续传

        m_snapshot.processed = m_processed;
        m_snapshot.success = m_success;
        m_snapshot.fail = m_fail;
        emitProgress();  // D19 事件推送

        // 测试钩子：模拟进程中断（停止且不终态处理，状态留 Migrating、游标保留 nextIndex）。
        if (simulateInterrupt(m_processed)) {
            qCInfo(lcMigrationOrchestrator) << "doMigrating: simulated interrupt at" << m_processed;
            return false;
        }
    }

    // 全部条目处理完：依据计数定终态。
    MigrationState finalState;
    if (m_fail == 0) {
        finalState = MigrationState::Completed;
    } else if (m_success > 0) {
        finalState = MigrationState::PartialCompleted;
    } else {
        finalState = MigrationState::Failed;
    }

    if (!m_state.requestTransition(finalState, MigrationSubstage::MigratingCursor,
                                   QStringLiteral("migrate done"))) {
        finalize(finalState);
        return false;
    }
    finalize(finalState);
    return true;
}

// --- 终态 + 报告触发（D15）---

void MigrationOrchestrator::finalize(MigrationState finalState)
{
    qCInfo(lcMigrationOrchestrator) << "finalize: finalState=" << migrationStateToString(finalState)
                                    << "currentState=" << migrationStateToString(m_state.currentState())
                                    << "cancelled=" << m_state.isCancelled()
                                    << "success=" << m_success << "fail=" << m_fail;

    // 确保状态机进入终态：失败/中间态路径可能尚未转换（取消/自然完成路径已转换）。
    // 子阶段沿用当前态（按来源，如 BackupDone/ScanDone/MigratingCursor），比统一 MigratingCursor 更可读。
    if (m_state.currentState() != finalState) {
        if (!m_state.requestTransition(finalState, m_state.substage(),
                                       QStringLiteral("finalize"))) {
            // P3：受 TTP-015 合法转换边约束，某些 finalState 从当前态不可达
            // （如 Pending→Failed：Pending 仅允许 →BackingUp）。此时状态机停留原态，
            // 但 finished(finalState, reportPath) 与报告仍照常发出——持久状态与
            // 报告/信号不一致，属磁盘异常边界（save 失败导致）。此处显式记录，不静默。
            qCWarning(lcMigrationOrchestrator) << "finalize: cannot legally transition"
                                               << migrationStateToString(m_state.currentState())
                                               << "->" << migrationStateToString(finalState)
                                               << "; state left as-is, report/signal still emitted";
        }
    }

    // 仅 Failed 态回滚到备份基线（D1）；取消/中断绝不回滚。
    if (finalState == MigrationState::Failed && !m_backupPath.isEmpty()) {
        MigrationBackup backup(m_dbPath, m_backupDir);
        const BackupResult rr = backup.restoreFromBackup(m_backupPath);
        if (!rr.success) {
            qCWarning(lcMigrationOrchestrator) << "finalize: restoreFromBackup failed:" << rr.message;
        }
    }

    // 报告：仅终态（Completed/PartialCompleted/Failed）产出一次；NotNeeded 不出报告（D15）。
    QString reportPath;
    if (finalState != MigrationState::NotNeeded) {
        MigrationReportInput input;
        input.totalCount = m_scanTotalCount;
        input.needMigrateCount = m_scanNeedMigrateCount;
        input.alreadyTiptapCount = m_scanAlreadyTiptapCount;
        input.abnormalCount = m_scanAbnormalCount;
        input.abnormalNoteIds = m_scanAbnormalNoteIds;
        input.writeResults = m_writeResults;
        input.backupPath = m_backupPath;
        input.elapsedMs = m_timer.elapsed();
        input.finalState = finalState;
        input.cancelled = m_state.isCancelled();

        MigrationReport report(m_reportDir);
        const ReportResult rr = report.generate(input);
        reportPath = rr.reportPath;
        if (!rr.success) {
            qCWarning(lcMigrationOrchestrator) << "finalize: report generate failed:" << rr.message;
        }
    }

    m_finishedEmitted.store(true);
    emit finished(finalState, reportPath);
}

// --- 取消通道 ---

void MigrationOrchestrator::requestCancel()
{
    m_cancelRequested.store(true);
    qCInfo(lcMigrationOrchestrator) << "requestCancel: cancel requested";
}

// --- 进度查询 ---

MigrationOrchestrator::ProgressSnapshot MigrationOrchestrator::progressSnapshot() const
{
    return m_snapshot;
}

// --- 测试钩子 ---

bool MigrationOrchestrator::simulateInterrupt(int processedCount)
{
    Q_UNUSED(processedCount)
    return false;
}

// --- 私有辅助 ---

void MigrationOrchestrator::emitProgress()
{
    emit progressChanged(m_snapshot);
}

void MigrationOrchestrator::emitStage(MigrationState stage)
{
    m_snapshot.stage = stage;
    emit stageChanged(stage);
}

void MigrationOrchestrator::persistCursor()
{
    QJsonObject scan;
    scan[QStringLiteral("totalCount")] = m_scanTotalCount;
    scan[QStringLiteral("needMigrateCount")] = m_scanNeedMigrateCount;
    scan[QStringLiteral("alreadyTiptapCount")] = m_scanAlreadyTiptapCount;
    scan[QStringLiteral("abnormalCount")] = m_scanAbnormalCount;
    QJsonArray abnormalIds;
    for (qint32 id : m_scanAbnormalNoteIds) {
        abnormalIds.append(id);
    }
    scan[QStringLiteral("abnormalNoteIds")] = abnormalIds;

    QJsonObject cursor;
    cursor[QStringLiteral("backupPath")] = m_backupPath;
    QJsonArray ids;
    for (qint32 id : m_noteIds) {
        ids.append(id);
    }
    cursor[QStringLiteral("noteIds")] = ids;
    cursor[QStringLiteral("nextIndex")] = m_nextIndex;
    cursor[QStringLiteral("processed")] = m_processed;
    cursor[QStringLiteral("success")] = m_success;
    cursor[QStringLiteral("fail")] = m_fail;
    cursor[QStringLiteral("total")] = m_total;
    cursor[QStringLiteral("scan")] = scan;

    m_state.setCursor(cursor);
}

void MigrationOrchestrator::restoreCursorFromState()
{
    const QJsonObject c = m_state.cursor();
    m_backupPath = c.value(QStringLiteral("backupPath")).toString();

    m_noteIds.clear();
    const QJsonArray ids = c.value(QStringLiteral("noteIds")).toArray();
    for (const QJsonValue &v : ids) {
        m_noteIds.append(static_cast<qint32>(v.toInt()));
    }

    m_nextIndex = c.value(QStringLiteral("nextIndex")).toInt();
    m_processed = c.value(QStringLiteral("processed")).toInt();
    m_success = c.value(QStringLiteral("success")).toInt();
    m_fail = c.value(QStringLiteral("fail")).toInt();
    m_total = c.value(QStringLiteral("total")).toInt();

    const QJsonObject scan = c.value(QStringLiteral("scan")).toObject();
    m_scanTotalCount = scan.value(QStringLiteral("totalCount")).toInt();
    m_scanNeedMigrateCount = scan.value(QStringLiteral("needMigrateCount")).toInt();
    m_scanAlreadyTiptapCount = scan.value(QStringLiteral("alreadyTiptapCount")).toInt();
    m_scanAbnormalCount = scan.value(QStringLiteral("abnormalCount")).toInt();

    m_scanAbnormalNoteIds.clear();
    const QJsonArray abnormalIds = scan.value(QStringLiteral("abnormalNoteIds")).toArray();
    for (const QJsonValue &v : abnormalIds) {
        m_scanAbnormalNoteIds.append(static_cast<qint32>(v.toInt()));
    }
}
