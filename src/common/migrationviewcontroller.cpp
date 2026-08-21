// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationviewcontroller.h"

#include "tiptapchannelbridge.h"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcMigrationView, "voice_note_migration_view")
}

MigrationViewController *MigrationViewController::instance()
{
    static MigrationViewController instance;
    return &instance;
}

MigrationViewController::MigrationViewController(QObject *parent)
    : QObject(parent)
{
}

void MigrationViewController::start()
{
    // M2：跨线程 QueuedConnection 投递 ProgressSnapshot/MigrationState 前必须注册 metatype，
    // 否则后台线程 emit 的进度/阶段/终态信号无法跨线程队列投递。
    qRegisterMetaType<MigrationOrchestrator::ProgressSnapshot>();
    qRegisterMetaType<MigrationState>();

    if (!TiptapChannelBridge::instance()->debugEnabled()) {
        qCInfo(lcMigrationView) << "start: Tiptap migration disabled by environment";
        setMigrationActive(false);
        setTerminalState(QString());
        return;
    }

    if (m_orchestrator || m_migrationActive) {
        qCInfo(lcMigrationView) << "start: migration already in progress, ignore";
        return;
    }

    // 据状态机判定是否需迁移：Pending 或可续传（BackingUp/Scanning/Migrating/PartialCompleted）
    // 需展示进度界面；NotNeeded/Completed/Failed（已完成）直接放行不展示。
    MigrationStateMachine state(MigrationStateMachine::defaultFilePath());
    state.load();
    const MigrationState current = state.currentState();
    const bool needRun = (current == MigrationState::Pending) || state.isResumable();
    qCInfo(lcMigrationView) << "start: currentState=" << migrationStateToString(current)
                            << "needRun=" << needRun;
    if (!needRun) {
        setMigrationActive(false);
        return;
    }

    // 展示进度界面并拉起后台编排器；startIfNeeded 内部在后台线程启动前以
    // Qt::QueuedConnection 把进度/阶段/终态/中断信号连接到本控制器。
    setMigrationActive(true);
    MigrationOrchestrator *orchestrator = MigrationOrchestrator::startIfNeeded(this);
    if (!orchestrator) {
        // 状态在判定与启动之间变化（磁盘异常边界）：回退为不放行展示。
        qCWarning(lcMigrationView) << "start: startIfNeeded returned null, hide overlay";
        setMigrationActive(false);
        return;
    }
    m_orchestrator = orchestrator;
}

void MigrationViewController::requestCancel()
{
    if (m_cancelling) {
        return;
    }
    setCancelling(true);
    if (m_orchestrator) {
        // requestCancel 仅置原子标志，线程安全，可直接跨线程调用（不经过事件队列）。
        m_orchestrator->requestCancel();
        qCInfo(lcMigrationView) << "requestCancel: forwarded to orchestrator";
    } else {
        qCWarning(lcMigrationView) << "requestCancel: no active orchestrator";
    }
}

void MigrationViewController::enterApp()
{
    qCInfo(lcMigrationView) << "enterApp: dismiss terminal view";
    setTerminalState(QString());
    setBackupPath(QString());
    setReportPath(QString());
    setMigrationActive(false);
    releaseOrchestrator();
    emit appEntered();
}

void MigrationViewController::onProgressChanged(const MigrationOrchestrator::ProgressSnapshot &snapshot)
{
    // M1：仅消费 progressChanged 信号，不跨线程调 progressSnapshot()。
    setProcessed(snapshot.processed);
    setTotal(snapshot.total);
    setSuccess(snapshot.success);
    setFail(snapshot.fail);
}

void MigrationViewController::onStageChanged(MigrationState stage)
{
    setStage(migrationStateToString(stage));
}

void MigrationViewController::onTerminalInfo(MigrationState finalState, const QString &backupPath, const QString &reportPath)
{
    qCInfo(lcMigrationView) << "onTerminalInfo: finalState=" << migrationStateToString(finalState)
                            << "backupPath=" << backupPath << "reportPath=" << reportPath;
    setBackupPath(backupPath);
    setReportPath(reportPath);
    setCancelling(false);
    setMigrationActive(false);
    if (finalState == MigrationState::NotNeeded) {
        // NotNeeded 不展示终态视图，直接放行。
        setTerminalState(QString());
    } else {
        // Completed/PartialCompleted/Failed：终态视图由 terminalState 非空驱动展示，
        // 供用户查看备份/报告路径并通过"进入应用"放行。
        setTerminalState(migrationStateToString(finalState));
    }
    releaseOrchestrator();
}

void MigrationViewController::onAborted()
{
    qCInfo(lcMigrationView) << "onAborted: non-terminal exit, release overlay";
    setCancelling(false);
    setMigrationActive(false);
    releaseOrchestrator();
}

// --- 私有 ---

void MigrationViewController::releaseOrchestrator()
{
    if (m_orchestrator) {
        // 断开编排器到本控制器的所有信号连接；编排器由 deleteLater 回收，不在此 delete。
        m_orchestrator->disconnect(this);
        m_orchestrator = nullptr;
    }
}

void MigrationViewController::setMigrationActive(bool active)
{
    if (m_migrationActive == active) {
        return;
    }
    m_migrationActive = active;
    emit migrationActiveChanged();
}

void MigrationViewController::setStage(const QString &stage)
{
    if (m_stage == stage) {
        return;
    }
    m_stage = stage;
    emit stageChanged();
}

void MigrationViewController::setProcessed(int value)
{
    if (m_processed == value) {
        return;
    }
    m_processed = value;
    emit processedChanged();
}

void MigrationViewController::setTotal(int value)
{
    if (m_total == value) {
        return;
    }
    m_total = value;
    emit totalChanged();
}

void MigrationViewController::setSuccess(int value)
{
    if (m_success == value) {
        return;
    }
    m_success = value;
    emit successChanged();
}

void MigrationViewController::setFail(int value)
{
    if (m_fail == value) {
        return;
    }
    m_fail = value;
    emit failChanged();
}

void MigrationViewController::setTerminalState(const QString &state)
{
    if (m_terminalState == state) {
        return;
    }
    m_terminalState = state;
    emit terminalStateChanged();
}

void MigrationViewController::setBackupPath(const QString &path)
{
    if (m_backupPath == path) {
        return;
    }
    m_backupPath = path;
    emit backupPathChanged();
}

void MigrationViewController::setReportPath(const QString &path)
{
    if (m_reportPath == path) {
        return;
    }
    m_reportPath = path;
    emit reportPathChanged();
}

void MigrationViewController::setCancelling(bool value)
{
    if (m_cancelling == value) {
        return;
    }
    m_cancelling = value;
    emit cancellingChanged();
}
