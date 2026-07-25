// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/dbmigration/migrationbackup.h"

#include "gtest/gtest.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include <sys/stat.h>  // chmod
#include <unistd.h>     // getuid

namespace {

// 在指定路径创建一个真实 SQLite 数据库（含一张表 + 一行数据），用作备份源库。
bool createSqliteDb(const QString &path)
{
    const QString conn = QStringLiteral("ut_mkdb_%1").arg(
        QDateTime::currentDateTime().toMSecsSinceEpoch());
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            return false;
        }
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral("CREATE TABLE vnote_items_tbl (note_id INTEGER PRIMARY KEY, content TEXT)"))) {
            db.close();
            return false;
        }
        if (!q.exec(QStringLiteral("INSERT INTO vnote_items_tbl VALUES (1, 'hello-tiptap')"))) {
            db.close();
            return false;
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return true;
}

// 读取数据库中 vnote_items_tbl 的第一行 content，用于校验恢复后数据一致。
QString readFirstContent(const QString &path)
{
    const QString conn = QStringLiteral("ut_read_%1").arg(
        QDateTime::currentDateTime().toMSecsSinceEpoch());
    QString content;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return content;
        }
        QSqlQuery q(db);
        if (q.exec(QStringLiteral("SELECT content FROM vnote_items_tbl ORDER BY note_id LIMIT 1")) && q.next()) {
            content = q.value(0).toString();
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return content;
}

} // namespace

// --- 默认路径定位 ---

TEST(UT_MigrationBackup, DefaultPathsAreAbsolute)
{
    const QString dbPath = MigrationBackup::defaultDbPath();
    const QString backupDir = MigrationBackup::defaultBackupDir();
    EXPECT_FALSE(dbPath.isEmpty());
    EXPECT_FALSE(backupDir.isEmpty());
    EXPECT_TRUE(dbPath.endsWith(QStringLiteral(".db")));
    EXPECT_TRUE(backupDir.endsWith(QStringLiteral("migration/backup")));
}

// --- 备份成功 ---

TEST(UT_MigrationBackup, BackupSuccess)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    ASSERT_TRUE(createSqliteDb(dbPath));
    const qint64 sourceSize = QFileInfo(dbPath).size();

    MigrationBackup mb(dbPath, backupDir);
    mb.setBackupNameOverride(QStringLiteral("test.db.success"));
    const BackupResult result = mb.backup();

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::None);
    const QString expectedBackup = backupDir + QStringLiteral("/test.db.success");
    EXPECT_EQ(result.backupPath, expectedBackup);
    EXPECT_TRUE(QFileInfo::exists(expectedBackup));
    EXPECT_EQ(QFileInfo(expectedBackup).size(), sourceSize);
    // 源库未被修改 / 移动
    EXPECT_TRUE(QFileInfo(dbPath).exists());
    EXPECT_EQ(QFileInfo(dbPath).size(), sourceSize);
}

// --- 失败：源不存在 ---

TEST(UT_MigrationBackup, BackupSourceNotFound)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/nonexistent.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");

    MigrationBackup mb(dbPath, backupDir);
    const BackupResult result = mb.backup();

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::SourceNotFound);
}

// --- 失败：目标已存在（同秒冲突防御）---

TEST(UT_MigrationBackup, BackupAlreadyExists)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    ASSERT_TRUE(createSqliteDb(dbPath));

    // 预先在目标路径放一个占位文件，模拟同秒命名冲突
    const QString conflictPath = backupDir + QStringLiteral("/test.db.exists");
    ASSERT_TRUE(QDir().mkpath(backupDir));
    {
        QFile f(conflictPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("placeholder");
        f.close();
    }

    MigrationBackup mb(dbPath, backupDir);
    mb.setBackupNameOverride(QStringLiteral("test.db.exists"));
    const BackupResult result = mb.backup();

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::AlreadyExists);
    // 不覆盖：占位文件内容不变
    QFile f(conflictPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QByteArray("placeholder"));
    f.close();
}

// --- 失败：复制失败（源文件不可读）---

TEST(UT_MigrationBackup, BackupCopyFailed)
{
    // chmod 0000 使源文件不可读，但 root 绕过文件权限仍可读，
    // 故以 root 身份运行时跳过（CI 通常非 root）。
    if (getuid() == 0) {
        GTEST_SKIP() << "chmod 0000 ineffective as root; run as non-root user";
    }
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    ASSERT_TRUE(createSqliteDb(dbPath));

    // 移除源文件所有权限，使 QFile::copy 无法打开源文件（open 失败 → CopyFailed）
    chmod(QFile::encodeName(dbPath).constData(), 0000);

    MigrationBackup mb(dbPath, backupDir);
    mb.setBackupNameOverride(QStringLiteral("test.db.copyfail"));
    const BackupResult result = mb.backup();

    // 恢复权限以便 QTemporaryDir 清理
    chmod(QFile::encodeName(dbPath).constData(), 0600);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::CopyFailed);
    // 不应产生副本
    EXPECT_FALSE(QFileInfo::exists(backupDir + QStringLiteral("/test.db.copyfail")));
}

// --- 失败：备份目录创建失败（路径被普通文件占据）---

TEST(UT_MigrationBackup, BackupDestDirCreateFailed)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    ASSERT_TRUE(createSqliteDb(dbPath));

    // 备份目录路径的父级是一个普通文件，mkpath 无法在其下创建目录 → DestDirCreateFailed
    const QString blocker = dir.path() + QStringLiteral("/blocker");
    {
        QFile f(blocker);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
    }
    const QString badBackupDir = blocker + QStringLiteral("/backup");

    MigrationBackup mb(dbPath, badBackupDir);
    mb.setBackupNameOverride(QStringLiteral("test.db.destdir"));
    const BackupResult result = mb.backup();

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::DestDirCreateFailed);
}

// --- 失败：完整性校验失败（源非 SQLite 文件，副本无法通过 integrity_check）---

TEST(UT_MigrationBackup, BackupIntegrityCheckFailed)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/garbage.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    // 写入非 SQLite 的垃圾内容作为源库
    {
        QFile f(dbPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("this is not a sqlite database file");
        f.close();
    }

    MigrationBackup mb(dbPath, backupDir);
    mb.setBackupNameOverride(QStringLiteral("test.db.integrity"));
    const BackupResult result = mb.backup();

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::IntegrityCheckFailed);
    // 残留副本已清理
    EXPECT_FALSE(QFileInfo::exists(backupDir + QStringLiteral("/test.db.integrity")));
}

// --- WAL 旁路复制 ---

TEST(UT_MigrationBackup, BackupWalSidecar)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    ASSERT_TRUE(createSqliteDb(dbPath));

    // 手工在源库旁造 -wal / -shm 占位文件
    {
        QFile w(dbPath + QStringLiteral("-wal"));
        ASSERT_TRUE(w.open(QIODevice::WriteOnly));
        w.write("wal-placeholder");
        w.close();
    }
    {
        QFile s(dbPath + QStringLiteral("-shm"));
        ASSERT_TRUE(s.open(QIODevice::WriteOnly));
        s.write("shm-placeholder");
        s.close();
    }

    MigrationBackup mb(dbPath, backupDir);
    mb.setBackupNameOverride(QStringLiteral("test.db.wal"));
    const BackupResult result = mb.backup();

    EXPECT_TRUE(result.success);
    const QString backupBase = backupDir + QStringLiteral("/test.db.wal");
    EXPECT_TRUE(QFileInfo::exists(backupBase + QStringLiteral("-wal")));
    EXPECT_TRUE(QFileInfo::exists(backupBase + QStringLiteral("-shm")));
}

// --- 回滚往返：备份 → 篡改源库 → restoreFromBackup → 源库恢复 ---

TEST(UT_MigrationBackup, RestoreRoundTrip)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    ASSERT_TRUE(createSqliteDb(dbPath));
    const QString originalContent = readFirstContent(dbPath);
    ASSERT_EQ(originalContent, QStringLiteral("hello-tiptap"));

    // 1. 备份
    MigrationBackup mbBackup(dbPath, backupDir);
    mbBackup.setBackupNameOverride(QStringLiteral("test.db.roundtrip"));
    const BackupResult backupResult = mbBackup.backup();
    ASSERT_TRUE(backupResult.success);
    const QString backupPath = backupResult.backupPath;

    // 2. 篡改源库（覆盖为垃圾内容）
    {
        QFile f(dbPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("corrupted-data");
        f.close();
    }
    EXPECT_NE(readFirstContent(dbPath), originalContent);

    // 3. 恢复
    MigrationBackup mbRestore(dbPath, backupDir);
    const BackupResult restoreResult = mbRestore.restoreFromBackup(backupPath);
    EXPECT_TRUE(restoreResult.success);
    EXPECT_EQ(restoreResult.code, BackupErrorCode::None);

    // 4. 源库内容已恢复
    EXPECT_EQ(readFirstContent(dbPath), originalContent);
}


// --- 失败：恢复时目标目录创建失败（路径被普通文件占据）---

TEST(UT_MigrationBackup, RestoreDestDirCreateFailed)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    // 先造一个有效备份文件
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    ASSERT_TRUE(createSqliteDb(dbPath));
    MigrationBackup mbBackup(dbPath, backupDir);
    mbBackup.setBackupNameOverride(QStringLiteral("test.db.restoredestdir"));
    const BackupResult br = mbBackup.backup();
    ASSERT_TRUE(br.success);
    const QString backupPath = br.backupPath;

    // 目标 db 路径的祖父级是一个普通文件，mkpath 无法在其下创建目录 → DestDirCreateFailed
    // 需用深层路径：targetDir = blocker/sub 不存在，mkpath 尝试创建 blocker（文件）失败。
    const QString blocker = dir.path() + QStringLiteral("/blocker");
    {
        QFile f(blocker);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
    }
    const QString badDbPath = blocker + QStringLiteral("/sub/deepin-voice-note1.0.db");

    MigrationBackup mbRestore(badDbPath, backupDir);
    const BackupResult result = mbRestore.restoreFromBackup(backupPath);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::DestDirCreateFailed);
}

// --- 失败：恢复时 rename 失败（目标路径是已存在的目录）---

TEST(UT_MigrationBackup, RestoreRenameFailed)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    // 造一个有效备份文件
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    ASSERT_TRUE(createSqliteDb(dbPath));
    MigrationBackup mbBackup(dbPath, backupDir);
    mbBackup.setBackupNameOverride(QStringLiteral("test.db.renamefail"));
    const BackupResult br = mbBackup.backup();
    ASSERT_TRUE(br.success);
    const QString backupPath = br.backupPath;

    // 目标 db 路径指向一个已存在的目录：tmp 文件可创建，但 rename(file, directory)
    // 在 Linux 上返回 EISDIR → CopyFailed
    const QString targetDir = dir.path() + QStringLiteral("/targetdir");
    ASSERT_TRUE(QDir().mkpath(targetDir));

    MigrationBackup mbRestore(targetDir, backupDir);
    const BackupResult result = mbRestore.restoreFromBackup(backupPath);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::CopyFailed);
    // 临时文件已清理
    EXPECT_FALSE(QFileInfo::exists(targetDir + QStringLiteral(".restore.tmp")));
}

// --- 回滚失败：备份文件不存在 ---

TEST(UT_MigrationBackup, RestoreSourceNotFound)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    const QString missingBackup = dir.path() + QStringLiteral("/no-such-backup.db");

    MigrationBackup mb(dbPath, dir.path());
    const BackupResult result = mb.restoreFromBackup(missingBackup);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, BackupErrorCode::SourceNotFound);
}
