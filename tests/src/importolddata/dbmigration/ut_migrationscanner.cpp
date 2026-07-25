// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/dbmigration/migrationscanner.h"

#include "gtest/gtest.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

namespace {

// 唯一连接名后缀，避免多用例连接名冲突。
QString uniqueConn(const char *prefix)
{
    return QStringLiteral("%1_%2").arg(QLatin1String(prefix),
                                       QDateTime::currentDateTime().toMSecsSinceEpoch());
}

// 建一张与运行态同名列的 vnote_items_tbl（含 expand_filed2 = encrypt 列）。
bool createScanTable(QSqlQuery &q)
{
    return q.exec(QStringLiteral(
        "CREATE TABLE vnote_items_tbl ("
        "note_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "folder_id INTEGER, "
        "note_type INTEGER, "
        "note_title TEXT, "
        "meta_data TEXT, "
        "note_state INTEGER, "
        "create_time TEXT, "
        "modify_time TEXT, "
        "delete_time TEXT, "
        "expand_filed1 INTEGER, "
        "expand_filed2 INTEGER)"));
}

// 插入一行笔记（encrypt 落在 expand_filed2 列）。
bool insertNote(QSqlQuery &q, qint32 noteId, const QString &metaData, int encrypt = 0)
{
    q.prepare(QStringLiteral("INSERT INTO vnote_items_tbl "
                              "(note_id, folder_id, note_type, note_title, meta_data, "
                              "note_state, create_time, modify_time, delete_time, "
                              "expand_filed1, expand_filed2) "
                              "VALUES (?, 0, 0, 't', ?, 0, '', '', '', 0, ?)"));
    q.addBindValue(noteId);
    q.addBindValue(metaData);
    q.addBindValue(encrypt);
    return q.exec();
}

// 构造覆盖六类格式 + 加密笔记的 7 行 fixture 库。
bool createFixtureDb(const QString &path)
{
    const QString conn = uniqueConn("ut_scan_mk");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        QSqlQuery q(db);
        ok = createScanTable(q);
        // 1. 已是 Tiptap 信封
        ok = ok && insertNote(q, 1, QStringLiteral("{\"format\":\"tiptap\",\"content\":{}}"));
        // 2. ProseMirror 文档
        ok = ok && insertNote(q, 2, QStringLiteral("{\"type\":\"doc\",\"content\":[]}"));
        // 3. 旧 HTML（JSON htmlCode）
        ok = ok && insertNote(q, 3, QStringLiteral("{\"htmlCode\":\"<p>hi</p>\"}"));
        // 4. 旧 noteDatas
        ok = ok && insertNote(q, 4, QStringLiteral("{\"noteDatas\":[]}"));
        // 5. 纯文本
        ok = ok && insertNote(q, 5, QStringLiteral("plain text note"));
        // 6. 非法 JSON 对象（无法归类）
        ok = ok && insertNote(q, 6, QStringLiteral("{\"unknown\":\"obj\"}"));
        // 7. 加密笔记：meta_data 为 base64(旧 HTML JSON)，encrypt=1
        const QByteArray encoded = QByteArrayLiteral("{\"htmlCode\":\"<p>enc</p>\"}").toBase64();
        ok = ok && insertNote(q, 7, QString::fromLatin1(encoded), 1);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

// 构造空表库（仅有表结构，无数据）。
bool createEmptyDb(const QString &path)
{
    const QString conn = uniqueConn("ut_scan_empty");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        QSqlQuery q(db);
        ok = createScanTable(q);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

// 构造 N 行需迁移笔记的库（用于取消测试）。
bool createBulkDb(const QString &path, int rows)
{
    const QString conn = uniqueConn("ut_scan_bulk");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        QSqlQuery q(db);
        ok = createScanTable(q);
        for (int i = 1; i <= rows; ++i) {
            ok = ok && insertNote(q, i, QStringLiteral("plain %1").arg(i));
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

QByteArray fileHash(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256);
}

}  // namespace

// --- 默认路径定位 ---

TEST(UT_MigrationScanner, DefaultDbPathIsAbsolute)
{
    const QString dbPath = MigrationScanner::defaultDbPath();
    EXPECT_FALSE(dbPath.isEmpty());
    EXPECT_TRUE(dbPath.endsWith(QStringLiteral(".db")));
    EXPECT_TRUE(dbPath.contains(QStringLiteral("deepin-voice-note")));
}

// --- 六类格式 + 加密笔记归类 ---

TEST(UT_MigrationScanner, ClassifyAllFormatsAndEncrypted)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/deepin-voice-note1.0.db");
    ASSERT_TRUE(createFixtureDb(dbPath));

    MigrationScanner scanner(dbPath);
    const ScanResult r = scanner.scan();

    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.code, ScanErrorCode::None);
    EXPECT_EQ(r.totalCount, 7);
    EXPECT_EQ(r.alreadyTiptapCount, 1);
    EXPECT_EQ(r.needMigrateCount, 5);
    EXPECT_EQ(r.abnormalCount, 1);

    // 需迁移清单仅 note_id，且不含加密误判（加密笔记 7 正确归为需迁移）。
    ASSERT_EQ(r.needMigrateNoteIds.size(), 5);
    EXPECT_EQ(r.needMigrateNoteIds[0], 2);
    EXPECT_EQ(r.needMigrateNoteIds[1], 3);
    EXPECT_EQ(r.needMigrateNoteIds[2], 4);
    EXPECT_EQ(r.needMigrateNoteIds[3], 5);
    EXPECT_EQ(r.needMigrateNoteIds[4], 7);

    // 异常清单 note_id + 原因
    ASSERT_EQ(r.abnormals.size(), 1);
    EXPECT_EQ(r.abnormals[0].noteId, 6);
    EXPECT_FALSE(r.abnormals[0].reason.isEmpty());
}

// --- 空表：totalCount=0，成功（状态机侧→NotNeeded）---

TEST(UT_MigrationScanner, EmptyTableIsNotNeeded)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/empty.db");
    ASSERT_TRUE(createEmptyDb(dbPath));

    MigrationScanner scanner(dbPath);
    const ScanResult r = scanner.scan();

    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.code, ScanErrorCode::None);
    EXPECT_EQ(r.totalCount, 0);
    EXPECT_EQ(r.needMigrateCount, 0);
    EXPECT_EQ(r.alreadyTiptapCount, 0);
    EXPECT_EQ(r.abnormalCount, 0);
    EXPECT_TRUE(r.needMigrateNoteIds.isEmpty());
    EXPECT_TRUE(r.abnormals.isEmpty());
}

// --- 只读：扫描前后 DB 文件内容不变 ---

TEST(UT_MigrationScanner, DoesNotModifyDatabase)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/readonly.db");
    ASSERT_TRUE(createFixtureDb(dbPath));

    const QByteArray before = fileHash(dbPath);
    ASSERT_FALSE(before.isEmpty());

    MigrationScanner scanner(dbPath);
    const ScanResult r = scanner.scan();
    ASSERT_TRUE(r.success);

    const QByteArray after = fileHash(dbPath);
    EXPECT_EQ(before, after);
    // 不产生 WAL 旁路文件
    EXPECT_FALSE(QFileInfo::exists(dbPath + QStringLiteral("-wal")));
    EXPECT_FALSE(QFileInfo::exists(dbPath + QStringLiteral("-shm")));
}

// --- 失败：源库文件不存在 ---

TEST(UT_MigrationScanner, DbOpenFailedWhenMissing)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/nonexistent.db");

    MigrationScanner scanner(dbPath);
    const ScanResult r = scanner.scan();

    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, ScanErrorCode::DbOpenFailed);
    EXPECT_FALSE(r.message.isEmpty());
    // 不应创建空库文件
    EXPECT_FALSE(QFileInfo::exists(dbPath));
}

// --- 失败：损坏库（非 SQLite 文件）查询失败 ---

TEST(UT_MigrationScanner, QueryFailedOnCorruptFile)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/garbage.db");
    {
        QFile f(dbPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("this is not a sqlite database file");
        f.close();
    }

    MigrationScanner scanner(dbPath);
    const ScanResult r = scanner.scan();

    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, ScanErrorCode::QueryFailed);
    EXPECT_FALSE(r.message.isEmpty());
}

// --- 取消：处理若干行后协作取消 ---

namespace {
// 子类：处理满 cancelAfter 行后触发 cancel()，模拟 TTP-020 中途取消。
class CancelScanner : public MigrationScanner
{
public:
    using MigrationScanner::MigrationScanner;
    int cancelAfter = 3;

protected:
    void onRowProcessed(int processedCount) override
    {
        if (processedCount >= cancelAfter) {
            cancel();
        }
    }
};
}  // namespace

TEST(UT_MigrationScanner, AbortMidScan)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/bulk.db");
    ASSERT_TRUE(createBulkDb(dbPath, 20));

    CancelScanner scanner(dbPath);
    scanner.cancelAfter = 3;
    const ScanResult r = scanner.scan();

    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, ScanErrorCode::Aborted);
    // 处理满 3 行后下一行检查点退出
    EXPECT_EQ(r.totalCount, 3);
    EXPECT_FALSE(r.message.isEmpty());
}

// --- 需迁移清单仅 note_id（GAP-1：不持 meta_data）---

TEST(UT_MigrationScanner, NeedMigrateListContainsOnlyNoteIds)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/ids.db");
    ASSERT_TRUE(createFixtureDb(dbPath));

    MigrationScanner scanner(dbPath);
    const ScanResult r = scanner.scan();
    ASSERT_TRUE(r.success);

    // ScanAbnormal 仅含 noteId + reason 字符串，无 meta_data 载荷；
    // needMigrateNoteIds 元素类型为 qint32，结构上不可能携带 meta_data。
    for (const qint32 id : r.needMigrateNoteIds) {
        EXPECT_GT(id, 0);
    }
    EXPECT_EQ(r.needMigrateNoteIds.size(), r.needMigrateCount);
    EXPECT_EQ(r.abnormals.size(), r.abnormalCount);
}
