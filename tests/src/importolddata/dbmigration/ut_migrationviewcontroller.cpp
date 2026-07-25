// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// TTP-021: MigrationViewController 单元测试。
// 复用 ut_migrationorchestrator 的注入式编排器（构造带 dbPath/stateFilePath/backupDir/
// reportDir）+ 直接驱动控制器公开槽；测试构建开启 -fno-access-control，可注入私有
// m_orchestrator 与 m_migrationActive 以构造可测状态。

#include "importolddata/dbmigration/migrationorchestrator.h"
#include "importolddata/dbmigration/migrationstate.h"
#include "common/migrationviewcontroller.h"

#include "gtest/gtest.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

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
    const QString conn = uniqueConn("ut_vc_mk");
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

bool insertNoteInto(const QString &path, qint32 noteId, const QString &metaData)
{
    const QString conn = uniqueConn("ut_vc_ins");
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
                                  "VALUES (?, 0, 0, 't', ?, 0, '', '2026-01-01 00:00:00', '', 0, 0)"));
        q.addBindValue(noteId);
        q.addBindValue(metaData);
        ok = q.exec();
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

// 注入式编排器：可重写 simulateInterrupt 以模拟进程中断（aborted 路径）。
class InterruptableOrchestrator : public MigrationOrchestrator
{
public:
    InterruptableOrchestrator(const QString &dbPath, const QString &statePath,
                              const QString &backupDir, const QString &reportDir)
        : MigrationOrchestrator(dbPath, statePath, backupDir, reportDir)
    {
    }

    void setInterruptAfter(int count) { m_interruptAfter = count; }

protected:
    bool simulateInterrupt(int processedCount) override
    {
        return m_interruptAfter > 0 && processedCount >= m_interruptAfter;
    }

private:
    int m_interruptAfter = 0;
};

} // namespace

// --- 信号→属性映射 ---

TEST(UT_MigrationViewController, SignalToPropertyMapping)
{
    MigrationViewController ctrl;

    MigrationOrchestrator::ProgressSnapshot snap;
    snap.stage = MigrationState::Migrating;
    snap.processed = 3;
    snap.total = 5;
    snap.success = 2;
    snap.fail = 1;
    ctrl.onProgressChanged(snap);

    EXPECT_EQ(ctrl.processed(), 3);
    EXPECT_EQ(ctrl.total(), 5);
    EXPECT_EQ(ctrl.success(), 2);
    EXPECT_EQ(ctrl.fail(), 1);

    ctrl.onStageChanged(MigrationState::Migrating);
    EXPECT_EQ(ctrl.stage().toStdString(), "Migrating");

    ctrl.onStageChanged(MigrationState::BackingUp);
    EXPECT_EQ(ctrl.stage().toStdString(), "BackingUp");
}

// --- 取消转发：requestCancel 置 cancelling 并转发到编排器 ---

TEST(UT_MigrationViewController, CancelForwarding)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));

    MigrationViewController ctrl;
    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    ctrl.m_orchestrator = &o;  // -fno-access-control：注入编排器

    EXPECT_FALSE(ctrl.cancelling());
    ctrl.requestCancel();
    EXPECT_TRUE(ctrl.cancelling());
    // 转发到编排器：取消原子标志已置位。
    EXPECT_TRUE(o.m_cancelRequested.load());

    // 重复 requestCancel 不再转发（cancelling 已为 true）。
    ctrl.requestCancel();
}

// --- NotNeeded 放行：不展示终态视图，migrationActive 翻 false ---

TEST(UT_MigrationViewController, NotNeededRelease)
{
    MigrationViewController ctrl;
    ctrl.m_migrationActive = true;  // -fno-access-control

    ctrl.onTerminalInfo(MigrationState::NotNeeded, QString(), QString());

    EXPECT_FALSE(ctrl.migrationActive());
    EXPECT_TRUE(ctrl.terminalState().isEmpty());
    EXPECT_TRUE(ctrl.backupPath().isEmpty());
    EXPECT_TRUE(ctrl.reportPath().isEmpty());
    EXPECT_FALSE(ctrl.cancelling());
}

// --- 终态填充：Completed 终态 backupPath/reportPath 填充、migrationActive 翻 false、QPointer 清空 ---

TEST(UT_MigrationViewController, TerminalFillCompleted)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));

    MigrationViewController ctrl;
    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    // 连接编排器信号→控制器槽（同线程，直接调用路径）。
    QObject::connect(&o, &MigrationOrchestrator::progressChanged, &ctrl,
                     &MigrationViewController::onProgressChanged);
    QObject::connect(&o, &MigrationOrchestrator::stageChanged, &ctrl,
                     &MigrationViewController::onStageChanged);
    QObject::connect(&o, &MigrationOrchestrator::terminalInfo, &ctrl,
                     &MigrationViewController::onTerminalInfo);
    QObject::connect(&o, &MigrationOrchestrator::aborted, &ctrl,
                     &MigrationViewController::onAborted);
    ctrl.m_migrationActive = true;  // -fno-access-control
    ctrl.m_orchestrator = &o;       // -fno-access-control

    o.run();  // → Completed，emit terminalInfo

    EXPECT_EQ(ctrl.terminalState().toStdString(), "Completed");
    EXPECT_FALSE(ctrl.backupPath().isEmpty());
    EXPECT_FALSE(ctrl.reportPath().isEmpty());
    EXPECT_FALSE(ctrl.migrationActive());
    EXPECT_FALSE(ctrl.cancelling());
    // QPointer 已在 onTerminalInfo 内清空。
    EXPECT_EQ(ctrl.m_orchestrator.data(), nullptr);
    // Migrating 计数已映射。
    EXPECT_EQ(ctrl.success(), 1);
    EXPECT_EQ(ctrl.fail(), 0);
}

// --- Failed 终态：backupPath/reportPath 填充 ---

TEST(UT_MigrationViewController, TerminalFillFailed)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    // 无需迁移笔记但有异常项 → Failed（无需迁移且 abnormalCount>0）。
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"unknown\":\"obj\"}")));

    MigrationViewController ctrl;
    MigrationOrchestrator o(dbPath, statePath, backupDir, reportDir);
    QObject::connect(&o, &MigrationOrchestrator::terminalInfo, &ctrl,
                     &MigrationViewController::onTerminalInfo);
    ctrl.m_migrationActive = true;
    ctrl.m_orchestrator = &o;

    o.run();  // → Failed

    EXPECT_EQ(ctrl.terminalState().toStdString(), "Failed");
    EXPECT_FALSE(ctrl.migrationActive());
    EXPECT_FALSE(ctrl.backupPath().isEmpty());
    EXPECT_FALSE(ctrl.reportPath().isEmpty());
}

// --- aborted 放行：模拟中断 → migrationActive 翻 false、QPointer 清空 ---

TEST(UT_MigrationViewController, AbortedRelease)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString dbPath = dir.path() + QStringLiteral("/notes.db");
    const QString statePath = dir.path() + QStringLiteral("/state/migration-state.json");
    const QString backupDir = dir.path() + QStringLiteral("/backup");
    const QString reportDir = dir.path() + QStringLiteral("/report");
    ASSERT_TRUE(createDb(dbPath));
    ASSERT_TRUE(insertNoteInto(dbPath, 1, QStringLiteral("{\"noteDatas\":[{\"type\":1,\"text\":\"a\"}]}")));

    MigrationViewController ctrl;
    InterruptableOrchestrator o(dbPath, statePath, backupDir, reportDir);
    o.setInterruptAfter(1);  // 处理完第 1 条后中断 → aborted
    QObject::connect(&o, &MigrationOrchestrator::aborted, &ctrl,
                     &MigrationViewController::onAborted);
    QObject::connect(&o, &MigrationOrchestrator::terminalInfo, &ctrl,
                     &MigrationViewController::onTerminalInfo);
    ctrl.m_migrationActive = true;
    ctrl.m_orchestrator = &o;

    o.run();  // → simulateInterrupt → aborted

    EXPECT_FALSE(ctrl.migrationActive());
    EXPECT_TRUE(ctrl.terminalState().isEmpty());  // 非终态，无终态视图
    EXPECT_EQ(ctrl.m_orchestrator.data(), nullptr);
}

// --- enterApp 放行：终态后收起终态视图 ---

TEST(UT_MigrationViewController, EnterAppDismissesTerminal)
{
    MigrationViewController ctrl;
    ctrl.m_migrationActive = false;
    ctrl.m_terminalState = QStringLiteral("Completed");
    ctrl.m_backupPath = QStringLiteral("/tmp/backup.db");
    ctrl.m_reportPath = QStringLiteral("/tmp/report.json");

    ctrl.enterApp();

    EXPECT_FALSE(ctrl.migrationActive());
    EXPECT_TRUE(ctrl.terminalState().isEmpty());
    EXPECT_TRUE(ctrl.backupPath().isEmpty());
    EXPECT_TRUE(ctrl.reportPath().isEmpty());
}
