// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/dbmigration/migrationwriter.h"

#include "importolddata/tiptapmigration/legacyformatdetector.h"

#include "gtest/gtest.h"

#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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
bool createTable(QSqlQuery &q)
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

// 插入一行笔记（encrypt 落在 expand_filed2 列，modify_time 固定便于断言未变）。
bool insertNote(QSqlQuery &q, qint32 noteId, const QString &metaData,
                int encrypt = 0, const QString &modifyTime = QStringLiteral("2026-01-01 00:00:00"))
{
    q.prepare(QStringLiteral("INSERT INTO vnote_items_tbl "
                              "(note_id, folder_id, note_type, note_title, meta_data, "
                              "note_state, create_time, modify_time, delete_time, "
                              "expand_filed1, expand_filed2) "
                              "VALUES (?, 0, 0, 't', ?, 0, '', ?, '', 0, ?)"));
    q.addBindValue(noteId);
    q.addBindValue(metaData);
    q.addBindValue(modifyTime);
    q.addBindValue(encrypt);
    return q.exec();
}

// 建库并插入若干行（按需由用例填充），返回库可读写连接。
bool createDb(const QString &path)
{
    const QString conn = uniqueConn("ut_writer_mk");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        QSqlQuery q(db);
        ok = createTable(q);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

// 在已存在的库上插入一行（独立连接）。
bool insertNoteInto(const QString &path, qint32 noteId, const QString &metaData,
                    int encrypt = 0,
                    const QString &modifyTime = QStringLiteral("2026-01-01 00:00:00"))
{
    const QString conn = uniqueConn("ut_writer_ins");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        QSqlQuery q(db);
        ok = insertNote(q, noteId, metaData, encrypt, modifyTime);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

// 读回单行 meta_data / expand_filed2 / modify_time。
struct NoteRow {
    bool found = false;
    QString metaData;
    int encrypt = 0;
    QString modifyTime;
};

NoteRow readNote(const QString &path, qint32 noteId)
{
    NoteRow row;
    const QString conn = uniqueConn("ut_writer_rd");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return row;
        }
        QSqlQuery q(db);
        q.prepare(QStringLiteral("SELECT meta_data, expand_filed2, modify_time "
                                  "FROM vnote_items_tbl WHERE note_id=?"));
        q.addBindValue(noteId);
        if (q.exec() && q.next()) {
            row.found = true;
            row.metaData = q.value(0).toString();
            row.encrypt = q.value(1).toInt();
            row.modifyTime = q.value(2).toString();
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return row;
}

// 在库上执行任意 DDL（如建触发器），独立连接。
bool execDdl(const QString &path, const QString &ddl)
{
    const QString conn = uniqueConn("ut_writer_ddl");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        QSqlQuery q(db);
        ok = q.exec(ddl);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

}  // namespace

// --- 默认路径定位 ---

TEST(UT_MigrationWriter, DefaultDbPathIsAbsolute)
{
    const QString dbPath = MigrationWriter::defaultDbPath();
    ASSERT_FALSE(dbPath.isEmpty());
    EXPECT_TRUE(QFileInfo(dbPath).isAbsolute());
}

// --- #1 noteDatas → 信封 ---

TEST(UT_MigrationWriter, NoteDatasConvertedToEnvelope)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    ASSERT_TRUE(createDb(dbPath));
    const QString metaData = QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"hello\"}]}");
    ASSERT_TRUE(insertNoteInto(dbPath, 10, metaData));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 10);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.code, WriteErrorCode::None);

    const NoteRow row = readNote(dbPath, 10);
    ASSERT_TRUE(row.found);
    // 写回后为 Tiptap 信封。
    EXPECT_EQ(LegacyFormatDetector::detect(row.metaData), LegacyFormat::TiptapEnvelope);
    // modify_time 未被触碰。
    EXPECT_EQ(row.modifyTime, QStringLiteral("2026-01-01 00:00:00"));
}

// --- #2 htmlCode → 信封（清空）---

TEST(UT_MigrationWriter, HtmlCodeReplacedByEnvelopeWithoutHtmlCodeKey)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/html.db");
    ASSERT_TRUE(createDb(dbPath));
    const QString metaData = QStringLiteral("{\"htmlCode\":\"<p>hi</p>\"}");
    ASSERT_TRUE(insertNoteInto(dbPath, 11, metaData));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 11);
    EXPECT_TRUE(r.success);

    const NoteRow row = readNote(dbPath, 11);
    ASSERT_TRUE(row.found);
    EXPECT_EQ(LegacyFormatDetector::detect(row.metaData), LegacyFormat::TiptapEnvelope);
    const QJsonObject obj = QJsonDocument::fromJson(row.metaData.toUtf8()).object();
    // 信封内不含旧 htmlCode 键。
    EXPECT_FALSE(obj.contains(QStringLiteral("htmlCode")));
    EXPECT_EQ(obj.value(QStringLiteral("format")).toString(), QStringLiteral("tiptap"));
}

// --- #3 加密 round-trip ---

TEST(UT_MigrationWriter, EncryptedNoteRoundTrip)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/enc.db");
    ASSERT_TRUE(createDb(dbPath));
    const QString plain = QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"secret\"}]}");
    const QString encoded = QString::fromLatin1(plain.toUtf8().toBase64());
    ASSERT_TRUE(insertNoteInto(dbPath, 12, encoded, 1));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 12);
    EXPECT_TRUE(r.success);

    const NoteRow row = readNote(dbPath, 12);
    ASSERT_TRUE(row.found);
    // 加密标志不变。
    EXPECT_EQ(row.encrypt, 1);
    // meta_data 仍为 base64，解码后为紧凑信封。
    const QString decoded = QString::fromUtf8(QByteArray::fromBase64(row.metaData.toUtf8()));
    EXPECT_EQ(LegacyFormatDetector::detect(decoded), LegacyFormat::TiptapEnvelope);
    EXPECT_FALSE(decoded.contains(QLatin1String("\n")));
}

// --- #4 校验不通过 → 不写回（ValidationFailed）---

TEST(UT_MigrationWriter, ValidationFailedPreservesOriginal)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/vfail.db");
    ASSERT_TRUE(createDb(dbPath));
    // ProseMirrorDoc 含不支持的节点：包封后 validator 拒绝。
    const QString metaData =
        QStringLiteral("{\"type\":\"doc\",\"content\":[{\"type\":\"unknownNode\"}]}");
    ASSERT_TRUE(insertNoteInto(dbPath, 13, metaData));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 13);
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, WriteErrorCode::ValidationFailed);
    EXPECT_TRUE(r.originalDataPreserved);

    const NoteRow row = readNote(dbPath, 13);
    ASSERT_TRUE(row.found);
    // 原数据原样保留。
    EXPECT_EQ(row.metaData, metaData);
}

// --- #4' 转换失败 → 不写回（ConvertFailed）---

TEST(UT_MigrationWriter, ConvertFailedPreservesOriginal)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/cfail.db");
    ASSERT_TRUE(createDb(dbPath));
    // noteDatas 语音块缺 voicePath → 转换器报错。
    const QString metaData = QStringLiteral("{\"noteDatas\":[{\"type\":2,\"voicePath\":\"\"}]}");
    ASSERT_TRUE(insertNoteInto(dbPath, 14, metaData));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 14);
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, WriteErrorCode::ConvertFailed);
    EXPECT_TRUE(r.originalDataPreserved);

    const NoteRow row = readNote(dbPath, 14);
    ASSERT_TRUE(row.found);
    EXPECT_EQ(row.metaData, metaData);
}

// --- #5 写回失败 → 原数据保留 ---

TEST(UT_MigrationWriter, WriteFailedPreservesOriginal)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/wfail.db");
    ASSERT_TRUE(createDb(dbPath));
    const QString metaData = QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"x\"}]}");
    ASSERT_TRUE(insertNoteInto(dbPath, 15, metaData));
    // 建一个 BEFORE UPDATE 触发器强制 UPDATE 失败（RAISE ABORT 回滚单语句）。
    ASSERT_TRUE(execDdl(dbPath, QStringLiteral(
        "CREATE TRIGGER block_update BEFORE UPDATE ON vnote_items_tbl "
        "BEGIN SELECT RAISE(ABORT, 'update blocked'); END;")));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 15);
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, WriteErrorCode::WriteFailed);
    EXPECT_TRUE(r.originalDataPreserved);

    const NoteRow row = readNote(dbPath, 15);
    ASSERT_TRUE(row.found);
    EXPECT_EQ(row.metaData, metaData);
}

// --- #6 已是 Tiptap → 跳过 ---

TEST(UT_MigrationWriter, AlreadyTiptapSkipped)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/tip.db");
    ASSERT_TRUE(createDb(dbPath));
    const QString metaData = QStringLiteral("{\"format\":\"tiptap\",\"content\":{}}");
    ASSERT_TRUE(insertNoteInto(dbPath, 16, metaData));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 16);
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.originalDataPreserved);

    const NoteRow row = readNote(dbPath, 16);
    ASSERT_TRUE(row.found);
    // 未写回，原样保留。
    EXPECT_EQ(row.metaData, metaData);
}

// --- #7 单条失败隔离 ---

TEST(UT_MigrationWriter, SingleFailureIsolation)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/iso.db");
    ASSERT_TRUE(createDb(dbPath));
    // 坏：不可识别格式（Invalid）。
    ASSERT_TRUE(insertNoteInto(dbPath, 20, QStringLiteral("{\"unknown\":\"obj\"}")));
    // 好：旧 noteDatas。
    ASSERT_TRUE(insertNoteInto(dbPath, 21, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"ok\"}]}")));

    MigrationWriter writer(dbPath);
    const WriteResult bad = writer.writeOne(0, 20);
    EXPECT_FALSE(bad.success);
    EXPECT_EQ(bad.code, WriteErrorCode::UnsupportedFormat);

    // 坏的不影响好的成功写回。
    const WriteResult good = writer.writeOne(0, 21);
    EXPECT_TRUE(good.success);
    const NoteRow row = readNote(dbPath, 21);
    ASSERT_TRUE(row.found);
    EXPECT_EQ(LegacyFormatDetector::detect(row.metaData), LegacyFormat::TiptapEnvelope);
}

// --- #8 warnings 透传 ---

TEST(UT_MigrationWriter, WarningsPassedThrough)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/warn.db");
    ASSERT_TRUE(createDb(dbPath));
    // 非文本/非语音块 → 转换器产出 warning（skipped-non-text-block），ok()==true。
    const QString metaData = QStringLiteral("{\"noteDatas\":[{\"type\":3}]}");
    ASSERT_TRUE(insertNoteInto(dbPath, 22, metaData));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 22);
    EXPECT_TRUE(r.success);
    ASSERT_FALSE(r.warnings.isEmpty());
    const MigrationWarning w = r.warnings.first();
    EXPECT_FALSE(w.code.isEmpty());
    EXPECT_FALSE(w.message.isEmpty());
}

// --- #9 NoteNotFound ---

TEST(UT_MigrationWriter, NoteNotFoundPreservesOriginal)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/nf.db");
    ASSERT_TRUE(createDb(dbPath));

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 9999);
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, WriteErrorCode::NoteNotFound);
    EXPECT_TRUE(r.originalDataPreserved);
}

// --- 源库文件不存在 → ReadFailed ---

TEST(UT_MigrationWriter, ReadFailedWhenMissing)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/nonexistent.db");

    MigrationWriter writer(dbPath);
    const WriteResult r = writer.writeOne(0, 1);
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.code, WriteErrorCode::ReadFailed);
    EXPECT_FALSE(QFileInfo::exists(dbPath));
}
