// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationbackup.h"

#include "globaldef.h"            // DEEPIN_VOICE_NOTE
#include "db/vnotedbmanager.h"    // VNoteDbManager::DBVERSION

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include <cerrno>       // errno
#include <cstdio>       // std::rename
#include <cstring>      // strerror

namespace {

const char *kRestoreTmpSuffix = ".restore.tmp";
const char *kWalSuffix = "-wal";
const char *kShmSuffix = "-shm";

// 删除备份产物及其 WAL 旁路副本（完整性校验失败 / 复制失败后清理残留）。
void cleanupBackupArtifacts(const QString &backupPath)
{
    QFile::remove(backupPath);
    QFile::remove(backupPath + QLatin1String(kWalSuffix));
    QFile::remove(backupPath + QLatin1String(kShmSuffix));
}

// 清理目标侧残留的 -wal / -shm（恢复前调用，避免恢复后 SQLite 读到陈旧 WAL）。
void removeWalSidecar(const QString &dbPath)
{
    QFile::remove(dbPath + QLatin1String(kWalSuffix));
    QFile::remove(dbPath + QLatin1String(kShmSuffix));
}

} // namespace

MigrationBackup::MigrationBackup()
{
}

MigrationBackup::MigrationBackup(const QString &dbPath, const QString &backupDir)
    : m_dbPath(dbPath)
    , m_backupDir(backupDir)
{
}

QString MigrationBackup::defaultDbPath()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + QDir::separator() + DEEPIN_VOICE_NOTE
           + QString(VNoteDbManager::DBVERSION) + QStringLiteral(".db");
}

QString MigrationBackup::defaultBackupDir()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + QDir::separator() + QStringLiteral("migration/backup");
}

QString MigrationBackup::dbPath() const
{
    return m_dbPath;
}

QString MigrationBackup::backupDir() const
{
    return m_backupDir;
}

void MigrationBackup::setBackupNameOverride(const QString &name)
{
    m_backupNameOverride = name;
}

QString MigrationBackup::effectiveDbPath() const
{
    return m_dbPath.isEmpty() ? defaultDbPath() : m_dbPath;
}

QString MigrationBackup::generateBackupName() const
{
    const QString baseName = QFileInfo(effectiveDbPath()).fileName();
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return baseName + QStringLiteral(".") + timestamp;
}

bool MigrationBackup::copyWalSidecar(const QString &sourceDb, const QString &backupDb)
{
    // 返回 true：所有存在的 sidecar 均复制成功（或无 sidecar）。
    // 返回 false：存在 sidecar 但复制失败（P3-1：调用方据此判定备份不一致）。
    bool allOk = true;
    const QStringList suffixes = { QLatin1String(kWalSuffix), QLatin1String(kShmSuffix) };
    for (const QString &suffix : suffixes) {
        const QString sidecar = sourceDb + suffix;
        if (QFileInfo::exists(sidecar)) {
            const QString target = backupDb + suffix;
            if (!QFile::copy(sidecar, target)) {
                allOk = false;
                qWarning("MigrationBackup: failed to copy WAL sidecar %s -> %s",
                         qPrintable(sidecar), qPrintable(target));
            }
        }
    }
    return allOk;
}

bool MigrationBackup::verifyIntegrity(const QString &sourcePath, const QString &backupPath)
{
    // 1. 大小一致 + 目标非零
    const qint64 sourceSize = QFileInfo(sourcePath).size();
    const qint64 backupSize = QFileInfo(backupPath).size();
    if (backupSize <= 0) {
        qWarning("MigrationBackup: backup is empty (0 bytes)");
        return false;
    }
    if (sourceSize != backupSize) {
        qWarning("MigrationBackup: size mismatch source=%lld backup=%lld",
                 static_cast<long long>(sourceSize), static_cast<long long>(backupSize));
        return false;
    }

    // 2. 可打开为只读 SQLite 并执行 PRAGMA integrity_check（D13 增强）
    const QString connName = QStringLiteral("migrationbackup_verify_%1").arg(
        QDateTime::currentDateTime().toMSecsSinceEpoch());
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(backupPath);
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (!db.open()) {
            qWarning("MigrationBackup: cannot open backup readonly: %s",
                     qPrintable(db.lastError().text()));
        } else {
            QSqlQuery query(db);
            if (!query.exec(QStringLiteral("PRAGMA integrity_check"))) {
                qWarning("MigrationBackup: integrity_check exec failed: %s",
                         qPrintable(query.lastError().text()));
            } else if (query.next()) {
                const QString value = query.value(0).toString();
                ok = (value.compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0);
                if (!ok) {
                    qWarning("MigrationBackup: integrity_check returned: %s", qPrintable(value));
                }
            } else {
                qWarning("MigrationBackup: integrity_check returned no rows");
            }
            db.close();
        }
    }
    // db 已离开作用域，安全移除连接（避免 "connection still in use" 警告）
    QSqlDatabase::removeDatabase(connName);
    return ok;
}

BackupResult MigrationBackup::backup()
{
    BackupResult result;
    const QString sourcePath = effectiveDbPath();
    const QString backupDirPath = m_backupDir.isEmpty() ? defaultBackupDir() : m_backupDir;

    // 1. 源库存在检查（不可读/目录等源留给 QFile::copy 失败 → CopyFailed）
    if (!QFileInfo::exists(sourcePath)) {
        result.code = BackupErrorCode::SourceNotFound;
        result.message = QStringLiteral("Source database not found: %1").arg(sourcePath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 2. 确保备份目录存在（沿用 migrationstatepersistent.cpp 的 mkpath 模式）
    QDir dir(backupDirPath);
    if (!dir.exists() && !dir.mkpath(backupDirPath)) {
        result.code = BackupErrorCode::DestDirCreateFailed;
        result.message = QStringLiteral("Failed to create backup directory: %1").arg(backupDirPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 3. 时间戳命名（D12 多份共存不覆盖）
    const QString backupName = m_backupNameOverride.isEmpty()
                                   ? generateBackupName()
                                   : m_backupNameOverride;
    const QString backupPath = backupDirPath + QDir::separator() + backupName;

    // 4. 防御性 AlreadyExists 检查（极端同秒冲突返回失败而非覆盖）
    if (QFileInfo::exists(backupPath)) {
        result.code = BackupErrorCode::AlreadyExists;
        result.message = QStringLiteral("Backup target already exists: %1").arg(backupPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 5. 复制（QFile::copy，源库保持原样可继续读写，复制非移动）
    if (!QFile::copy(sourcePath, backupPath)) {
        result.code = BackupErrorCode::CopyFailed;
        result.message = QStringLiteral("Failed to copy database: %1 -> %2")
                             .arg(sourcePath, backupPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 6. WAL 旁路复制（D14 防御性：当前默认 rollback-journal 无 -wal/-shm，未来启用 WAL 亦兼容）
    //    P3-1：sidecar 复制失败纳入 CopyFailed，不允许"半成品备份"回报成功。
    if (!copyWalSidecar(sourcePath, backupPath)) {
        cleanupBackupArtifacts(backupPath);
        result.code = BackupErrorCode::CopyFailed;
        result.message = QStringLiteral("Failed to copy WAL sidecar for backup: %1").arg(backupPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 7. 完整性校验（D13：大小一致 + 可打开 + integrity_check）
    if (!verifyIntegrity(sourcePath, backupPath)) {
        cleanupBackupArtifacts(backupPath);
        result.code = BackupErrorCode::IntegrityCheckFailed;
        result.message = QStringLiteral("Integrity check failed for backup: %1").arg(backupPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 成功
    result.success = true;
    result.backupPath = backupPath;
    result.code = BackupErrorCode::None;
    result.message = QStringLiteral("Backup succeeded: %1").arg(backupPath);
    qInfo("MigrationBackup: %s (size=%lld bytes)", qPrintable(result.message),
          static_cast<long long>(QFileInfo(backupPath).size()));
    return result;
}

BackupResult MigrationBackup::restoreFromBackup(const QString &backupPath)
{
    BackupResult result;

    // 1. 备份文件存在检查
    if (!QFileInfo(backupPath).isFile()) {
        result.code = BackupErrorCode::SourceNotFound;
        result.message = QStringLiteral("Backup file not found: %1").arg(backupPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    const QString targetPath = effectiveDbPath();

    // 2. 确保目标目录存在
    const QString targetDir = QFileInfo(targetPath).absolutePath();
    if (!QDir().exists(targetDir) && !QDir().mkpath(targetDir)) {
        result.code = BackupErrorCode::DestDirCreateFailed;
        result.message = QStringLiteral("Failed to create target directory: %1").arg(targetDir);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 3. 先写临时文件（避免半覆盖目标）
    const QString tmpPath = targetPath + QLatin1String(kRestoreTmpSuffix);
    if (QFileInfo::exists(tmpPath)) {
        QFile::remove(tmpPath);
    }
    if (!QFile::copy(backupPath, tmpPath)) {
        result.code = BackupErrorCode::CopyFailed;
        result.message = QStringLiteral("Failed to copy backup to temp: %1 -> %2").arg(backupPath, tmpPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 3b. 复制 sidecar 到临时文件（P3-3：WAL 模式下校验需一致快照）
    copyWalSidecar(backupPath, tmpPath);

    // 4. 完整性校验临时文件（含 sidecar，WAL 模式下快照一致）
    if (!verifyIntegrity(backupPath, tmpPath)) {
        cleanupBackupArtifacts(tmpPath);
        result.code = BackupErrorCode::IntegrityCheckFailed;
        result.message = QStringLiteral("Integrity check failed for restore temp: %1").arg(tmpPath);
        qWarning("MigrationBackup: %s", qPrintable(result.message));
        return result;
    }

    // 4b. 清理临时 sidecar（rename 只处理主文件，target sidecar 在 rename 后复制）
    removeWalSidecar(tmpPath);

    // 4c. 清理目标侧残留 sidecar（P3-2：避免恢复后 SQLite 读到陈旧 WAL）
    removeWalSidecar(targetPath);

    // 5. 原子替换目标：POSIX rename(2) 原子覆盖（与 migrationstatepersistent.cpp 一致）
    //    Qt 的 QFile::rename 在目标已存在时返回 false 不覆盖，故用 std::rename。
    if (std::rename(QFile::encodeName(tmpPath).constData(),
                    QFile::encodeName(targetPath).constData()) != 0) {
        qWarning("MigrationBackup: rename failed: %s -> %s (%s)",
                 qPrintable(tmpPath), qPrintable(targetPath),
                 qPrintable(QString::fromLocal8Bit(strerror(errno))));
        QFile::remove(tmpPath);
        result.code = BackupErrorCode::CopyFailed;
        result.message = QStringLiteral("Failed to rename temp to target: %1 -> %2").arg(tmpPath, targetPath);
        return result;
    }

    // 6. 恢复 WAL 旁路（best-effort）
    copyWalSidecar(backupPath, targetPath);

    result.success = true;
    result.backupPath = targetPath;
    result.code = BackupErrorCode::None;
    result.message = QStringLiteral("Restore succeeded: %1 -> %2").arg(backupPath, targetPath);
    qInfo("MigrationBackup: %s", qPrintable(result.message));
    return result;
}
