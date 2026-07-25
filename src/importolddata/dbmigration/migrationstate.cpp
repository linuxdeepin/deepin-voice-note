// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationstate.h"
#include "migrationstatepersistent.h"

#include <QDateTime>
#include <QLoggingCategory>
#include <QStandardPaths>

namespace {
Q_LOGGING_CATEGORY(lcMigrationState, "voice_note_migration_state")
}

// --- 枚举字符串转换 ---

QString migrationStateToString(MigrationState state)
{
    switch (state) {
    case MigrationState::NotNeeded:       return QStringLiteral("NotNeeded");
    case MigrationState::Pending:         return QStringLiteral("Pending");
    case MigrationState::BackingUp:       return QStringLiteral("BackingUp");
    case MigrationState::Scanning:        return QStringLiteral("Scanning");
    case MigrationState::Migrating:       return QStringLiteral("Migrating");
    case MigrationState::Completed:       return QStringLiteral("Completed");
    case MigrationState::PartialCompleted:return QStringLiteral("PartialCompleted");
    case MigrationState::Failed:          return QStringLiteral("Failed");
    }
    return QStringLiteral("Pending");
}

MigrationState migrationStateFromString(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed == QStringLiteral("NotNeeded"))       return MigrationState::NotNeeded;
    if (trimmed == QStringLiteral("Pending"))         return MigrationState::Pending;
    if (trimmed == QStringLiteral("BackingUp"))       return MigrationState::BackingUp;
    if (trimmed == QStringLiteral("Scanning"))        return MigrationState::Scanning;
    if (trimmed == QStringLiteral("Migrating"))       return MigrationState::Migrating;
    if (trimmed == QStringLiteral("Completed"))       return MigrationState::Completed;
    if (trimmed == QStringLiteral("PartialCompleted"))return MigrationState::PartialCompleted;
    if (trimmed == QStringLiteral("Failed"))          return MigrationState::Failed;
    return MigrationState::Pending; // 未知字符串由调用方回检
}

QString migrationSubstageToString(MigrationSubstage sub)
{
    switch (sub) {
    case MigrationSubstage::None:            return QStringLiteral("None");
    case MigrationSubstage::BackupDone:      return QStringLiteral("BackupDone");
    case MigrationSubstage::ScanDone:        return QStringLiteral("ScanDone");
    case MigrationSubstage::MigratingCursor: return QStringLiteral("MigratingCursor");
    }
    return QStringLiteral("None");
}

MigrationSubstage migrationSubstageFromString(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed == QStringLiteral("None"))            return MigrationSubstage::None;
    if (trimmed == QStringLiteral("BackupDone"))      return MigrationSubstage::BackupDone;
    if (trimmed == QStringLiteral("ScanDone"))        return MigrationSubstage::ScanDone;
    if (trimmed == QStringLiteral("MigratingCursor")) return MigrationSubstage::MigratingCursor;
    return MigrationSubstage::None;
}

// --- 合法转换表（仅下列边合法，其余拒绝） ---

bool canTransition(MigrationState from, MigrationState to)
{
    switch (from) {
    case MigrationState::Pending:
        return to == MigrationState::BackingUp;
    case MigrationState::BackingUp:
        return to == MigrationState::Scanning || to == MigrationState::Failed;
    case MigrationState::Scanning:
        return to == MigrationState::Migrating
               || to == MigrationState::NotNeeded
               || to == MigrationState::Failed;
    case MigrationState::Migrating:
        return to == MigrationState::Completed
               || to == MigrationState::PartialCompleted
               || to == MigrationState::Failed;
    case MigrationState::PartialCompleted:
        return to == MigrationState::Pending; // 续传重试（TTP-020 驱动）
    case MigrationState::Failed:
        return to == MigrationState::Pending; // 回滚完成后重置以允许重试
    case MigrationState::Completed:
        return to == MigrationState::Pending; // 重置/重新迁移（开发与测试用）
    case MigrationState::NotNeeded:
        return to == MigrationState::Pending; // 重置/重新迁移（开发与测试用）
    }
    return false;
}

// --- MigrationStateMachine ---

MigrationStateMachine::MigrationStateMachine()
    : MigrationStateMachine(defaultFilePath())
{
}

MigrationStateMachine::MigrationStateMachine(const QString &filePath)
    : m_filePath(filePath)
{
    resetToInitial();
}

void MigrationStateMachine::resetToInitial()
{
    m_state = MigrationState::Pending;
    m_substage = MigrationSubstage::None;
    m_cursor = QJsonObject();
    m_cancelled = false;
    m_updatedAt.clear();
    m_history.clear();
}

QString MigrationStateMachine::currentTimestamp() const
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

MigrationState MigrationStateMachine::currentState() const
{
    return m_state;
}

MigrationSubstage MigrationStateMachine::substage() const
{
    return m_substage;
}

QJsonObject MigrationStateMachine::cursor() const
{
    return m_cursor;
}

bool MigrationStateMachine::isCancelled() const
{
    return m_cancelled;
}

QString MigrationStateMachine::updatedAt() const
{
    return m_updatedAt;
}

bool MigrationStateMachine::isResumable() const
{
    switch (m_state) {
    case MigrationState::BackingUp:
    case MigrationState::Scanning:
    case MigrationState::Migrating:
    case MigrationState::PartialCompleted:
        return true;
    default:
        return false;
    }
}

QVector<HistoryEntry> MigrationStateMachine::history() const
{
    return m_history;
}

bool MigrationStateMachine::requestTransition(MigrationState to, MigrationSubstage sub,
                                              const QString &reason)
{
    if (!canTransition(m_state, to)) {
        qCWarning(lcMigrationState)
            << "illegal transition:" << migrationStateToString(m_state)
            << "->" << migrationStateToString(to) << "rejected";
        return false;
    }

    HistoryEntry entry;
    entry.from = m_state;
    entry.to = to;
    entry.substage = sub;
    entry.reason = reason;
    entry.timestamp = currentTimestamp();
    entry.cursor = m_cursor;

    m_state = to;
    m_substage = sub;
    m_updatedAt = entry.timestamp;
    m_history.append(entry);

    save();
    return true;
}

void MigrationStateMachine::setCursor(const QJsonObject &cursor)
{
    m_cursor = cursor;
    m_updatedAt = currentTimestamp();
    save();
}

void MigrationStateMachine::markCancelled()
{
    m_cancelled = true;
    // D3 取消映射 PartialCompleted：仅 Migrating 可合法转入
    if (canTransition(m_state, MigrationState::PartialCompleted)) {
        requestTransition(MigrationState::PartialCompleted, m_substage,
                          QStringLiteral("cancelled by user"));
    } else {
        // 非可取消态：仅记录 cancelled 标记并落盘，状态不变
        m_updatedAt = currentTimestamp();
        qCWarning(lcMigrationState)
            << "markCancelled called in non-cancellable state:"
            << migrationStateToString(m_state);
        save();
    }
}

bool MigrationStateMachine::load()
{
    MigrationStatePersistent persistent(m_filePath);
    return persistent.load(m_state, m_substage, m_cursor, m_cancelled,
                           m_updatedAt, m_history);
}

bool MigrationStateMachine::save()
{
    MigrationStatePersistent persistent(m_filePath);
    return persistent.save(m_state, m_substage, m_cursor, m_cancelled,
                           m_updatedAt, m_history);
}

QString MigrationStateMachine::defaultFilePath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + QStringLiteral("/migration/migration-state.json");
}
