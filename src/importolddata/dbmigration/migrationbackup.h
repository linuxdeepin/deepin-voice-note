// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONBACKUP_H
#define MIGRATIONBACKUP_H

#include <QString>

// TTP-016: 数据库备份错误码。
// 对齐集成数据契约「备份结果」：成功给路径，失败给原因码 + 描述。
enum class BackupErrorCode {
    None,                 // 无错误（成功）
    SourceNotFound,       // 源数据库文件不存在
    DestDirCreateFailed,  // 备份目录创建失败
    CopyFailed,           // 文件复制失败
    IntegrityCheckFailed, // 完整性校验失败
    AlreadyExists         // 目标备份已存在（同秒冲突防御）
};

// TTP-016: 备份结果结构。
// success=true 时 backupPath 为生成的备份路径；失败时 code/message 给出原因。
struct BackupResult {
    bool success = false;
    QString backupPath;
    BackupErrorCode code = BackupErrorCode::None;
    QString message;
};

// TTP-016: 数据库备份模块。
// 在 Tiptap 迁移写回前对业务数据库主文件做文件级复制备份，作为迁移失败时
// 可回退的安全基线。由 TTP-020 编排层在 BackingUp 态调用 backup()；
// 模块本身不感知迁移语义、不写状态机、不决定流程去留。
//
// R1 选型：维持纯 QFile::copy（系统无 sqlite3.h，且不改 CMake 引依赖）。
// 运行态一致性窗口由 TTP-020 调度时序保证：备份发生在 BackingUp 态
// （写回前最早环节，无迁移写操作），DB 处默认 rollback-journal 模式。
class MigrationBackup
{
public:
    // 默认构造：使用 defaultDbPath()/defaultBackupDir() 定位。
    MigrationBackup();
    // 注入 db 路径 / 备份目录（测试用）。
    MigrationBackup(const QString &dbPath, const QString &backupDir);

    // 执行备份：定位源库 → 确保备份目录 → 时间戳命名 → QFile::copy
    // （含 -wal/-shm 旁路）→ 完整性校验 → 返回 BackupResult。
    BackupResult backup();

    // 回滚原语：将备份复制覆盖回源库路径（先写临时文件再 rename，避免半覆盖）。
    // 不含自动触发、不接 UI；由 TTP-020 在 Failed 态显式调用。
    BackupResult restoreFromBackup(const QString &backupPath);

    // 默认路径定位（只读复用 VNoteDbManager::DBVERSION / DEEPIN_VOICE_NOTE 逻辑）。
    static QString defaultDbPath();
    static QString defaultBackupDir();

    QString dbPath() const;
    QString backupDir() const;

    // 测试注入：覆盖自动生成的时间戳备份名。为空则自动生成 <dbname>.<yyyyMMdd-HHmmss>。
    void setBackupNameOverride(const QString &name);

private:
    // 完整性校验：源/目标文件大小一致 + 目标非零 + 可打开为只读 SQLite 执行
    // PRAGMA integrity_check。校验失败返回 false，调用方负责清理已生成副本。
    bool verifyIntegrity(const QString &sourcePath, const QString &backupPath);
    // 生成时间戳备份文件名：<dbFileName>.<yyyyMMdd-HHmmss>。
    QString generateBackupName() const;
    // WAL 旁路复制：检测 <db>-wal / <db>-shm 同级文件是否存在，存在则一并复制。
    // 返回 true：所有存在的 sidecar 均复制成功（或无 sidecar）；false：存在但复制失败。
    bool copyWalSidecar(const QString &sourceDb, const QString &backupDb);
    // 解析当前生效的源库路径（注入优先，否则 defaultDbPath()）。
    QString effectiveDbPath() const;

    QString m_dbPath;
    QString m_backupDir;
    QString m_backupNameOverride;
};

#endif // MIGRATIONBACKUP_H
