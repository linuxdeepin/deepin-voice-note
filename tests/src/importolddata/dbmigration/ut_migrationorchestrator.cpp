// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/dbmigration/migrationorchestrator.h"
#include "importolddata/dbmigration/migrationbackup.h"
#include "importolddata/dbmigration/migrationstate.h"
#include "importolddata/dbmigration/migrationwriter.h"
#include "importolddata/tiptapmigration/legacyformatdetector.h"

#include "gtest/gtest.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QVector>

namespace {

QString uniqueConn(const char *prefix)
{
    return QStringLiteral("%1_%2").arg(QLatin1String(prefix),
                                       QDateTime::currentDateTime().toMSecsSinceEpoch());
}

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

bool createDb(const QString &path)
{
    const QString conn = uniqueConn("ut_orch_mk");
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

bool insertNoteInto(const QString &path, qint32 noteId, const QString &metaData, int encrypt = 0)
{
    const QString conn = uniqueConn("ut_orch_ins");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        QSqlQuery q(db);
        q.prepare(QStringLiteral("INSERT INTO vnote_items_tbl "
                                  "(note_id, folder_id, note_type, note_title, meta_data, "
                                  "note_state, create_time, modify_time, delete_time, "
                                  "expand_filed1, expand_filed2) "
                                  "VALUES (?, 0, 0, 't', ?, 0, '', '2026-01-01 00:00:00', '', 0, ?)"));
        q.addBindValue(noteId);
        q.addBindValue(metaData);
        q.addBindValue(encrypt);
        ok = q.exec();
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

struct NoteRow {
    bool found = false;
    QString metaData;
};

NoteRow readNote(const QString &path, qint32 noteId)
{
    NoteRow row;
    const QString conn = uniqueConn("ut_orch_rd");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(conn);
            return row;
        }
        QSqlQuery q(db);
        q.prepare(QStringLiteral("SELECT meta_data FROM vnote_items_tbl WHERE note_id=?"));
        q.addBindValue(noteId);
        if (q.exec() && q.next()) {
            row.found = true;
            row.metaData = q.value(0).toString();
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return row;
}

bool isTiptap(const QString &path, qint32 noteId)
{
    const NoteRow row = readNote(path, noteId);
    return row.found
           && LegacyFormatDetector::detect(row.metaData) == LegacyFormat::TiptapEnvelope;
}

// 备份目录下 .db 文件计数（验证是否新增备份）。
int backupFileCount(const QString &backupDir)
{
    QDir dir(backupDir);
    if (!dir.exists()) {
        return 0;
    }
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.db"),
                                            QDir::Files);
    return files.size();
}

// 报告目录下首份报告 JSON 路径（验证报告是否生成）。
QString firstReportPath(const QString &reportDir)
{
    QDir dir(reportDir);
    if (!dir.exists()) {
        return QString();
    }
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("migration-report.*.json"),
                                            QDir::Files);
    if (files.isEmpty()) {
        return QString();
    }
    return dir.absoluteFilePath(files.first());
}

QJsonObject loadReport(const QString &reportDir)
{
    const QString path = firstReportPath(reportDir);
    if (path.isEmpty()) {
        return QJsonObject();
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.object();
}

// 直接写一份 migration-state.json（续传测试用）。
bool writeStateFile(const QString &path, const QString &stateName, const QString &substageName,
                    const QJsonObject &cursor, bool cancelled = false)
{
    QJsonObject root;
    root[QStringLiteral("state")] = stateName;
    root[QStringLiteral("substage")] = substageName;
    root[QStringLiteral("cursor")] = cursor;
    root[QStringLiteral("cancelled")] = cancelled;
    root[QStringLiteral("updatedAt")] = QStringLiteral("2026-07-25T00:00:00");
    root[QStringLiteral("history")] = QJsonArray();

    const QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    f.write(payload);
    f.close();
    return true;
}

QJsonObject makeScanObject(int total, int need, int already, int abnormal,
                           const QVector<qint32> &abnormalIds)
{
    QJsonObject scan;
    scan[QStringLiteral("totalCount")] = total;
    scan[QStringLiteral("needMigrateCount")] = need;
    scan[QStringLiteral("alreadyTiptapCount")] = already;
    scan[QStringLiteral("abnormalCount")] = abnormal;
    QJsonArray ids;
    for (qint32 id : abnormalIds) {
        ids.append(id);
    }
    scan[QStringLiteral("abnormalNoteIds")] = ids;
    return scan;
}

QJsonObject makeCursor(const QString &backupPath, const QVector<qint32> &noteIds,
                       int nextIndex, int processed, int success, int fail, int total,
                       const QJsonObject &scan)
{
    QJsonObject cursor;
    cursor[QStringLiteral("backupPath")] = backupPath;
    QJsonArray ids;
    for (qint32 id : noteIds) {
        ids.append(id);
    }
    cursor[QStringLiteral("noteIds")] = ids;
    cursor[QStringLiteral("nextIndex")] = nextIndex;
    cursor[QStringLiteral("processed")] = processed;
    cursor[QStringLiteral("success")] = success;
    cursor[QStringLiteral("fail")] = fail;
    cursor[QStringLiteral("total")] = total;
    cursor[QStringLiteral("scan")] = scan;
    return cursor;
}

// 可中断编排器：处理到第 stopAfter 条后模拟进程中断（用于续传测试）。
class InterruptableOrchestrator : public MigrationOrchestrator
{
public:
    int stopAfter = -1;
    explicit InterruptableOrchestrator(const QString &dbPath, const QString &stateFilePath,
                                       const QString &backupDir, const QString &reportDir)
        : MigrationOrchestrator(dbPath, stateFilePath, backupDir, reportDir)
    {
    }

protected:
    bool simulateInterrupt(int processedCount) override
    {
        return stopAfter > 0 && processedCount >= stopAfter;
    }
};

}  // namespace

// --- 场景 1：全量成功迁移 ---

TEST(UT_MigrationOrchestrator, FullMigrationCompleted)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 2, QStringLiteral("{\"htmlCode\":\"<p>b</p>\"}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 3, QStringLiteral("plain text")));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState finalState = MigrationState::Pending;
    QString reportPath;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &p) { finalState = s; reportPath = p; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::Completed);
    EXPECT_FALSE(reportPath.isEmpty());
    EXPECT_TRUE(QFile::exists(reportPath));
    EXPECT_EQ(o.progressSnapshot().success, 3);

    EXPECT_TRUE(isTiptap(dbPath, 1));
    EXPECT_TRUE(isTiptap(dbPath, 2));
    EXPECT_TRUE(isTiptap(dbPath, 3));

    const QJsonObject report = loadReport(reportDir);
    EXPECT_EQ(report.value(QStringLiteral("finalState")).toString(), QStringLiteral("Completed"));
    EXPECT_EQ(report.value(QStringLiteral("counts")).toObject().value(QStringLiteral("success")).toInt(), 3);
}

// --- 场景 2：部分完成（单条失败隔离）---

TEST(UT_MigrationOrchestrator, PartialCompletedWithSingleFailureIsolation)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 2, QStringLiteral("{\"htmlCode\":\"<p>b</p>\"}")));
    // ProseMirror 含不支持节点：扫描归为需迁移，写回校验失败（ValidationFailed）。
    ASSERT_TRUE(insertNoteInto(dbPath, 3, QStringLiteral("{\"type\":\"doc\",\"content\":[{\"type\":\"unknownNode\"}]}")));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState finalState = MigrationState::Pending;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &) { finalState = s; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::PartialCompleted);
    EXPECT_EQ(o.progressSnapshot().success, 2);
    EXPECT_EQ(o.progressSnapshot().fail, 1);

    // 失败条目隔离：其他条目仍成功写回。
    EXPECT_TRUE(isTiptap(dbPath, 1));
    EXPECT_TRUE(isTiptap(dbPath, 2));
    // 失败条目原数据保留。
    const NoteRow row = readNote(dbPath, 3);
    ASSERT_TRUE(row.found);
    EXPECT_EQ(row.metaData, QStringLiteral("{\"type\":\"doc\",\"content\":[{\"type\":\"unknownNode\"}]}"));

    // 报告含失败 note_id。
    const QJsonObject report = loadReport(reportDir);
    const QJsonArray failedIds = report.value(QStringLiteral("failedNoteIds")).toArray();
    ASSERT_EQ(failedIds.size(), 1);
    EXPECT_EQ(failedIds.at(0).toInt(), 3);
}

// --- 场景 3：备份失败直接 Failed（不进 Scanning/Migrating + 报告 + 不写回）---

TEST(UT_MigrationOrchestrator, BackupFailureGoesFailed)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));

    // 备份目录父级为普通文件，mkpath 失败 → DestDirCreateFailed。
    const QString blocker = dir.path() + QStringLiteral("/blocker");
    {
        QFile f(blocker);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
    }
    const QString badBackupDir = blocker + QStringLiteral("/backup");

    MigrationOrchestrator o(dbPath, statePath, badBackupDir, reportDir);
    MigrationState finalState = MigrationState::Pending;
    QString reportPath;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &p) { finalState = s; reportPath = p; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::Failed);
    EXPECT_FALSE(reportPath.isEmpty());

    // 不写回任何笔记。
    EXPECT_FALSE(isTiptap(dbPath, 1));
    EXPECT_EQ(o.progressSnapshot().success, 0);

    const QJsonObject report = loadReport(reportDir);
    EXPECT_EQ(report.value(QStringLiteral("finalState")).toString(), QStringLiteral("Failed"));
}

// --- 场景 4：扫描判定无需迁移（NotNeeded + 不出报告 + 不写回）---

TEST(UT_MigrationOrchestrator, NotNeededWhenAllTiptap)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"format\":\"tiptap\",\"content\":{}}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 2, QStringLiteral("{\"format\":\"tiptap\",\"content\":{}}")));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState finalState = MigrationState::Pending;
    QString reportPath;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &p) { finalState = s; reportPath = p; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::NotNeeded);
    // 不出报告。
    EXPECT_TRUE(reportPath.isEmpty());
    EXPECT_TRUE(firstReportPath(reportDir).isEmpty());
    // 不写回：笔记仍为 Tiptap（未被改写）。
    EXPECT_TRUE(isTiptap(dbPath, 1));
    EXPECT_TRUE(isTiptap(dbPath, 2));
    EXPECT_EQ(o.progressSnapshot().success, 0);

    MigrationStateMachine sm(statePath);
    ASSERT_TRUE(sm.load());
    EXPECT_EQ(sm.currentState(), MigrationState::NotNeeded);
}

// --- 场景 5：用户取消（Migrating 第 2 条后 requestCancel → PartialCompleted）---

TEST(UT_MigrationOrchestrator, UserCancelInMigrating)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 2, QStringLiteral("{\"htmlCode\":\"<p>b</p>\"}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 3, QStringLiteral("plain three")));
    ASSERT_TRUE(insertNoteInto(dbPath, 4, QStringLiteral("plain four")));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    // 第 2 条处理完后请求取消。
    QObject::connect(&o, &MigrationOrchestrator::progressChanged,
                     [&](const MigrationOrchestrator::ProgressSnapshot &snap) {
                         if (snap.processed == 2) {
                             o.requestCancel();
                         }
                     });
    MigrationState finalState = MigrationState::Pending;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &) { finalState = s; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::PartialCompleted);

    // isCancelled == true（从状态文件）。
    MigrationStateMachine sm(statePath);
    ASSERT_TRUE(sm.load());
    EXPECT_TRUE(sm.isCancelled());
    EXPECT_EQ(sm.currentState(), MigrationState::PartialCompleted);

    // 已处理条目写回，未处理条目保留原数据。
    EXPECT_TRUE(isTiptap(dbPath, 1));
    EXPECT_TRUE(isTiptap(dbPath, 2));
    EXPECT_FALSE(isTiptap(dbPath, 3));
    EXPECT_FALSE(isTiptap(dbPath, 4));

    // 报告 cancelled==true。
    const QJsonObject report = loadReport(reportDir);
    EXPECT_EQ(report.value(QStringLiteral("cancelled")).toBool(), true);
    EXPECT_EQ(report.value(QStringLiteral("finalState")).toString(), QStringLiteral("PartialCompleted"));
}

// --- 场景 6：中断后续传（跑到第 2 条后销毁编排器 → 新编排器从 nextIndex=2 续传）---

TEST(UT_MigrationOrchestrator, ResumeAfterInterrupt)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 2, QStringLiteral("{\"htmlCode\":\"<p>b</p>\"}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 3, QStringLiteral("plain three")));
    ASSERT_TRUE(insertNoteInto(dbPath, 4, QStringLiteral("plain four")));

    // 第一阶段：跑到第 2 条后模拟中断，状态留 Migrating、游标 nextIndex=2。
    {
        InterruptableOrchestrator first(dbPath, statePath, backupDir, reportDir);
        first.stopAfter = 2;
        first.run();
        // 销毁 first（保留状态文件）。
    }

    // 中断后状态为 Migrating、可续传。
    {
        MigrationStateMachine sm(statePath);
        ASSERT_TRUE(sm.load());
        EXPECT_EQ(sm.currentState(), MigrationState::Migrating);
        EXPECT_TRUE(sm.isResumable());
        EXPECT_EQ(sm.cursor().value(QStringLiteral("nextIndex")).toInt(), 2);
    }

    // 第二阶段：新编排器从 nextIndex=2 续传。
    QVector<MigrationOrchestrator::ProgressSnapshot> snaps;
    MigrationOrchestrator second(dbPath, statePath, backupDir, reportDir);
    QObject::connect(&second, &MigrationOrchestrator::progressChanged,
                     [&](const MigrationOrchestrator::ProgressSnapshot &s) { snaps.append(s); });
    MigrationState finalState = MigrationState::Pending;
    QObject::connect(&second, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &) { finalState = s; });
    second.run();

    EXPECT_EQ(finalState, MigrationState::Completed);

    // 续传从 nextIndex=2 起，不重复已成功条目：第二条 run 不会出现 processed==1。
    bool restartedFromZero = false;
    for (const auto &s : snaps) {
        if (s.processed == 1) {
            restartedFromZero = true;
        }
    }
    EXPECT_FALSE(restartedFromZero);

    // 全部 4 条最终都已被迁移（1,2 来自首段，3,4 来自续传）。
    EXPECT_TRUE(isTiptap(dbPath, 1));
    EXPECT_TRUE(isTiptap(dbPath, 2));
    EXPECT_TRUE(isTiptap(dbPath, 3));
    EXPECT_TRUE(isTiptap(dbPath, 4));

    MigrationStateMachine sm(statePath);
    ASSERT_TRUE(sm.load());
    EXPECT_EQ(sm.currentState(), MigrationState::Completed);
}

// --- 场景 7a：续传跳过备份（BackupDone 不调 backup）---

TEST(UT_MigrationOrchestrator, ResumeSkipsBackup)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));

    // 预置备份目录 + 一个已有备份文件，状态为 BackingUp/BackupDone。
    QDir().mkpath(backupDir);
    const QString preBackup = backupDir + QStringLiteral("/pre.db");
    {
        QFile f(preBackup);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("backup");
        f.close();
    }
    const QJsonObject cursor = makeCursor(preBackup, {}, 0, 0, 0, 0, 0,
                                          makeScanObject(0, 0, 0, 0, {}));
    ASSERT_TRUE(writeStateFile(statePath, QStringLiteral("BackingUp"),
                               QStringLiteral("BackupDone"), cursor));
    const int before = backupFileCount(backupDir);
    ASSERT_EQ(before, 1);

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState finalState = MigrationState::Pending;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &) { finalState = s; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::Completed);
    // 未新增备份文件（backup 被跳过，沿用游标 backupPath）。
    EXPECT_EQ(backupFileCount(backupDir), 1);
    EXPECT_TRUE(isTiptap(dbPath, 1));
}

// --- 场景 7b：续传跳过扫描（ScanDone 不调 scan）---

TEST(UT_MigrationOrchestrator, ResumeSkipsScan)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    // 库中实际有 4 条需迁移笔记。
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 2, QStringLiteral("{\"htmlCode\":\"<p>b</p>\"}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 3, QStringLiteral("plain three")));
    ASSERT_TRUE(insertNoteInto(dbPath, 4, QStringLiteral("plain four")));

    QDir().mkpath(backupDir);
    const QString preBackup = backupDir + QStringLiteral("/pre.db");
    {
        QFile f(preBackup);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("backup");
        f.close();
    }
    // 游标清单只含 [1,2]：若扫描被跳过，仅迁移 1,2；若扫描重跑，会迁移 1,2,3,4。
    const QJsonObject cursor = makeCursor(preBackup, {1, 2}, 0, 0, 0, 0, 2,
                                          makeScanObject(4, 2, 0, 0, {}));
    ASSERT_TRUE(writeStateFile(statePath, QStringLiteral("Scanning"),
                               QStringLiteral("ScanDone"), cursor));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState finalState = MigrationState::Pending;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &) { finalState = s; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::Completed);
    // 扫描被跳过：仅处理游标清单 [1,2]，3,4 未被迁移（仍是旧格式）。
    EXPECT_EQ(o.progressSnapshot().success, 2);
    EXPECT_TRUE(isTiptap(dbPath, 1));
    EXPECT_TRUE(isTiptap(dbPath, 2));
    EXPECT_FALSE(isTiptap(dbPath, 3));
    EXPECT_FALSE(isTiptap(dbPath, 4));
}

// --- 场景 8：异常项不写回（Invalid 进报告 abnormalNoteIds，不参与 Migrating）---

TEST(UT_MigrationOrchestrator, AbnormalNotWrittenBack)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));
    ASSERT_TRUE(insertNoteInto(dbPath, 2, QStringLiteral("{\"htmlCode\":\"<p>b</p>\"}")));
    // Invalid 格式：扫描归为异常（不进需迁移清单）。
    ASSERT_TRUE(insertNoteInto(dbPath, 3, QStringLiteral("{\"unknown\":\"obj\"}")));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState finalState = MigrationState::Pending;
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &) { finalState = s; });
    o.run();

    EXPECT_EQ(finalState, MigrationState::Completed);
    // 异常项不参与 Migrating：仅 2 条成功。
    EXPECT_EQ(o.progressSnapshot().success, 2);
    EXPECT_EQ(o.progressSnapshot().fail, 0);

    // 异常项原数据保留（未写回）。
    const NoteRow row = readNote(dbPath, 3);
    ASSERT_TRUE(row.found);
    EXPECT_EQ(row.metaData, QStringLiteral("{\"unknown\":\"obj\"}"));

    // 报告 abnormalNoteIds 含异常 note_id。
    const QJsonObject report = loadReport(reportDir);
    const QJsonArray abnormalIds = report.value(QStringLiteral("abnormalNoteIds")).toArray();
    ASSERT_EQ(abnormalIds.size(), 1);
    EXPECT_EQ(abnormalIds.at(0).toInt(), 3);
    EXPECT_EQ(report.value(QStringLiteral("counts")).toObject().value(QStringLiteral("abnormal")).toInt(), 1);
}

// --- 启动决策：Completed 不启动 / Pending 启动 ---

TEST(UT_MigrationOrchestrator, ShouldRunDecision)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));

    // 初始 Pending → shouldRun true。
    {
        MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
        // shouldRun 为私有，通过公开行为间接验证：Pending 运行后进入终态。
        MigrationState fs = MigrationState::Pending;
        QObject::connect(&o, &MigrationOrchestrator::finished,
                         [&](MigrationState s, const QString &) { fs = s; });
        o.run();
        EXPECT_NE(fs, MigrationState::Pending);
    }

    // 终态 Completed 后，新编排器不再推进（finished 不发）。
    {
        MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
        bool emitted = false;
        QObject::connect(&o, &MigrationOrchestrator::finished,
                         [&]() { emitted = true; });
        o.run();
        EXPECT_FALSE(emitted);
    }
}

// --- terminalInfo 伴生信号（TTP-021 前置改动）：紧邻 finished 发出，携带 backupPath ---

TEST(UT_MigrationOrchestrator, TerminalInfoCarriedWithFinished)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState terminalState = MigrationState::Pending;
    QString terminalBackup;
    QString terminalReport;
    MigrationState finishedState = MigrationState::Pending;
    QString finishedReport;
    QObject::connect(&o, &MigrationOrchestrator::terminalInfo,
                     [&](MigrationState s, const QString &backup, const QString &report) {
                         terminalState = s;
                         terminalBackup = backup;
                         terminalReport = report;
                     });
    QObject::connect(&o, &MigrationOrchestrator::finished,
                     [&](MigrationState s, const QString &p) { finishedState = s; finishedReport = p; });
    o.run();

    // terminalInfo 与 finished 携带一致的终态与报告路径。
    EXPECT_EQ(terminalState, MigrationState::Completed);
    EXPECT_EQ(finishedState, MigrationState::Completed);
    EXPECT_EQ(terminalReport, finishedReport);
    EXPECT_FALSE(terminalReport.isEmpty());
    // Completed 终态 backupPath 非空（备份阶段已产出）。
    EXPECT_FALSE(terminalBackup.isEmpty());
    EXPECT_TRUE(QFile::exists(terminalBackup));
}

// --- terminalInfo：NotNeeded 终态 backupPath/reportPath 均为空 ---

TEST(UT_MigrationOrchestrator, TerminalInfoNotNeededIsEmpty)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    // 全部已是 Tiptap 信封（已迁移）→ 扫描无需迁移 → NotNeeded。
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"format\":\"tiptap\",\"content\":{}}")));

    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    MigrationState terminalState = MigrationState::Pending;
    QString terminalBackup = QStringLiteral("sentinel");
    QString terminalReport = QStringLiteral("sentinel");
    QObject::connect(&o, &MigrationOrchestrator::terminalInfo,
                     [&](MigrationState s, const QString &backup, const QString &report) {
                         terminalState = s;
                         terminalBackup = backup;
                         terminalReport = report;
                     });
    o.run();

    EXPECT_EQ(terminalState, MigrationState::NotNeeded);
    EXPECT_TRUE(terminalBackup.isEmpty());
    EXPECT_TRUE(terminalReport.isEmpty());
}
