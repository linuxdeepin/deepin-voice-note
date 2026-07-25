// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/dbmigration/migrationstate.h"

#include "gtest/gtest.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QTemporaryDir>

namespace {

// 构造一台指向独立临时文件的状态机
MigrationStateMachine makeMachine(const QTemporaryDir &dir)
{
    return MigrationStateMachine(dir.path() + QStringLiteral("/migration-state.json"));
}

QJsonObject makeCursor(int lastNoteId, int processedCount)
{
    QJsonObject cursor;
    cursor["lastNoteId"] = lastNoteId;
    cursor["processedCount"] = processedCount;
    return cursor;
}

} // namespace

// --- 枚举字符串转换 ---

TEST(UT_MigrationState, StateStringRoundTrip)
{
    const QVector<MigrationState> all = {
        MigrationState::NotNeeded, MigrationState::Pending, MigrationState::BackingUp,
        MigrationState::Scanning, MigrationState::Migrating, MigrationState::Completed,
        MigrationState::PartialCompleted, MigrationState::Failed};
    for (MigrationState s : all) {
        EXPECT_EQ(migrationStateFromString(migrationStateToString(s)), s);
    }
    // 未知字符串回退为 Pending（调用方负责回检）
    EXPECT_EQ(migrationStateFromString(QStringLiteral("Unknown")), MigrationState::Pending);
}

TEST(UT_MigrationState, SubstageStringRoundTrip)
{
    const QVector<MigrationSubstage> all = {
        MigrationSubstage::None, MigrationSubstage::BackupDone,
        MigrationSubstage::ScanDone, MigrationSubstage::MigratingCursor};
    for (MigrationSubstage s : all) {
        EXPECT_EQ(migrationSubstageFromString(migrationSubstageToString(s)), s);
    }
}

// --- 合法转换表全覆盖 ---

TEST(UT_MigrationState, LegalTransitions)
{
    EXPECT_TRUE(canTransition(MigrationState::Pending, MigrationState::BackingUp));

    EXPECT_TRUE(canTransition(MigrationState::BackingUp, MigrationState::Scanning));
    EXPECT_TRUE(canTransition(MigrationState::BackingUp, MigrationState::Failed));

    EXPECT_TRUE(canTransition(MigrationState::Scanning, MigrationState::Migrating));
    EXPECT_TRUE(canTransition(MigrationState::Scanning, MigrationState::NotNeeded));
    EXPECT_TRUE(canTransition(MigrationState::Scanning, MigrationState::Failed));

    EXPECT_TRUE(canTransition(MigrationState::Migrating, MigrationState::Completed));
    EXPECT_TRUE(canTransition(MigrationState::Migrating, MigrationState::PartialCompleted));
    EXPECT_TRUE(canTransition(MigrationState::Migrating, MigrationState::Failed));

    EXPECT_TRUE(canTransition(MigrationState::PartialCompleted, MigrationState::Pending));
    EXPECT_TRUE(canTransition(MigrationState::Failed, MigrationState::Pending));
    EXPECT_TRUE(canTransition(MigrationState::Completed, MigrationState::Pending));
    EXPECT_TRUE(canTransition(MigrationState::NotNeeded, MigrationState::Pending));
}

// --- 非法转换拒绝 ---

TEST(UT_MigrationState, IllegalTransitionsRejected)
{
    EXPECT_FALSE(canTransition(MigrationState::Completed, MigrationState::Migrating));
    EXPECT_FALSE(canTransition(MigrationState::NotNeeded, MigrationState::Failed));
    EXPECT_FALSE(canTransition(MigrationState::Pending, MigrationState::Completed));
    EXPECT_FALSE(canTransition(MigrationState::Failed, MigrationState::Migrating));
    EXPECT_FALSE(canTransition(MigrationState::BackingUp, MigrationState::Completed));
    EXPECT_FALSE(canTransition(MigrationState::Scanning, MigrationState::BackingUp));
    EXPECT_FALSE(canTransition(MigrationState::Migrating, MigrationState::BackingUp));
    EXPECT_FALSE(canTransition(MigrationState::Pending, MigrationState::PartialCompleted));
    // 不含 RollbackRequired：任何态都不允许转到一个不存在的回滚路径
    EXPECT_FALSE(canTransition(MigrationState::Migrating, MigrationState::Pending));
}

// --- requestTransition 合法流 + history 追加 ---

TEST(UT_MigrationState, RequestTransitionAppendsHistoryAndSaves)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);

    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None,
                                     QStringLiteral("start backup")));
    EXPECT_EQ(sm.currentState(), MigrationState::BackingUp);
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone,
                                     QStringLiteral("backup done")));
    EXPECT_EQ(sm.currentState(), MigrationState::Scanning);
    EXPECT_EQ(sm.substage(), MigrationSubstage::BackupDone);
    ASSERT_TRUE(sm.requestTransition(MigrationState::Migrating, MigrationSubstage::ScanDone,
                                     QStringLiteral("scan done")));
    EXPECT_EQ(sm.currentState(), MigrationState::Migrating);
    EXPECT_EQ(sm.substage(), MigrationSubstage::ScanDone);

    const auto hist = sm.history();
    ASSERT_EQ(hist.size(), 3);
    EXPECT_EQ(hist.at(0).to, MigrationState::BackingUp);
    EXPECT_EQ(hist.at(1).from, MigrationState::BackingUp);
    EXPECT_EQ(hist.at(1).substage, MigrationSubstage::BackupDone);
    EXPECT_EQ(hist.at(2).to, MigrationState::Migrating);
    EXPECT_EQ(hist.at(2).substage, MigrationSubstage::ScanDone);
    EXPECT_FALSE(sm.updatedAt().isEmpty());
    // 落盘文件已生成
    EXPECT_TRUE(QFileInfo::exists(dir.path() + QStringLiteral("/migration-state.json")));
}

TEST(UT_MigrationState, RequestIllegalTransitionReturnsFalse)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);

    EXPECT_FALSE(sm.requestTransition(MigrationState::Completed, MigrationSubstage::None,
                                      QStringLiteral("skip")));
    EXPECT_EQ(sm.currentState(), MigrationState::Pending); // 状态未变
    EXPECT_TRUE(sm.history().isEmpty()); // 未追加日志
}

// --- 持久化 round-trip ---

TEST(UT_MigrationState, PersistenceRoundTrip)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");

    MigrationStateMachine sm(path);
    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Migrating, MigrationSubstage::ScanDone, "m"));
    sm.setCursor(makeCursor(42, 7));

    // 新实例从同一文件 load
    MigrationStateMachine loaded(path);
    ASSERT_TRUE(loaded.load());
    EXPECT_EQ(loaded.currentState(), MigrationState::Migrating);
    EXPECT_EQ(loaded.substage(), MigrationSubstage::ScanDone);
    EXPECT_EQ(loaded.cursor().value("lastNoteId").toInt(), 42);
    EXPECT_EQ(loaded.cursor().value("processedCount").toInt(), 7);
    ASSERT_EQ(loaded.history().size(), sm.history().size());
    EXPECT_EQ(loaded.history().last().to, MigrationState::Migrating);
    EXPECT_TRUE(loaded.isResumable());
}

// --- 中断续传：ScanDone 跳过重扫 ---

TEST(UT_MigrationState, ResumeSkipsScanWhenScanDone)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");

    // 构造「扫描已完成、写回未开始/进行中」的续传态：Migrating + ScanDone + 游标
    MigrationStateMachine sm(path);
    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Migrating, MigrationSubstage::ScanDone, "m"));
    sm.setCursor(makeCursor(10, 3));

    // 模拟进程中断后重启：新实例 load
    MigrationStateMachine resumed(path);
    ASSERT_TRUE(resumed.load());
    EXPECT_TRUE(resumed.isResumable());
    EXPECT_EQ(resumed.substage(), MigrationSubstage::ScanDone); // 扫描已完成 -> 续传不重扫
    EXPECT_EQ(resumed.cursor().value("processedCount").toInt(), 3);
}

// 另：扫描未完成（仅 BackupDone）时续传需重扫
TEST(UT_MigrationState, ResumeNeedsRescanWhenScanNotDone)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");

    MigrationStateMachine sm(path);
    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));

    MigrationStateMachine resumed(path);
    ASSERT_TRUE(resumed.load());
    EXPECT_TRUE(resumed.isResumable());
    EXPECT_EQ(resumed.substage(), MigrationSubstage::BackupDone); // 扫描未完成 -> 需重扫
    EXPECT_NE(resumed.substage(), MigrationSubstage::ScanDone);
}

// --- 损坏文件回退初始态 ---

TEST(UT_MigrationState, CorruptFileFallsBackToInitial)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");

    // 写入非法 JSON
    {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("{ not valid json !!! ");
        f.close();
    }

    MigrationStateMachine sm(path);
    EXPECT_FALSE(sm.load()); // 解析失败返回 false
    EXPECT_EQ(sm.currentState(), MigrationState::Pending); // 回退初始态
    EXPECT_FALSE(sm.isCancelled());
    EXPECT_TRUE(sm.history().isEmpty());
}

// --- 未知状态字符串回退初始态 ---

TEST(UT_MigrationState, UnknownStateStringFallsBackToInitial)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");

    {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write(R"({"state":"RollbackRequired","substage":"None","cursor":{},"cancelled":false,"updatedAt":"","history":[]})");
        f.close();
    }

    MigrationStateMachine sm(path);
    EXPECT_FALSE(sm.load());
    EXPECT_EQ(sm.currentState(), MigrationState::Pending);
}

// --- 文件缺失视为初始态 ---

TEST(UT_MigrationState, MissingFileIsInitial)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);
    EXPECT_TRUE(sm.load()); // 文件缺失合法初始
    EXPECT_EQ(sm.currentState(), MigrationState::Pending);
    EXPECT_FALSE(sm.isResumable());
}

// --- 取消映射 PartialCompleted ---

TEST(UT_MigrationState, MarkCancelledMapsToPartialCompleted)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);

    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Migrating, MigrationSubstage::ScanDone, "m"));

    sm.markCancelled();
    EXPECT_EQ(sm.currentState(), MigrationState::PartialCompleted);
    EXPECT_TRUE(sm.isCancelled());
    EXPECT_TRUE(sm.isResumable()); // PartialCompleted 可续传
    // 取消产生一条历史记录
    EXPECT_FALSE(sm.history().isEmpty());
    EXPECT_EQ(sm.history().last().to, MigrationState::PartialCompleted);
}

// 取消标记持久化
TEST(UT_MigrationState, CancelledFlagPersists)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");

    MigrationStateMachine sm(path);
    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Migrating, MigrationSubstage::ScanDone, "m"));
    sm.markCancelled();

    MigrationStateMachine loaded(path);
    ASSERT_TRUE(loaded.load());
    EXPECT_TRUE(loaded.isCancelled());
    EXPECT_EQ(loaded.currentState(), MigrationState::PartialCompleted);
}

// --- 原子写：成功后无 .tmp 残留 ---

TEST(UT_MigrationState, AtomicWriteLeavesNoTmpResidue)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);

    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    EXPECT_TRUE(QFileInfo::exists(dir.path() + QStringLiteral("/migration-state.json")));
    EXPECT_FALSE(QFileInfo::exists(dir.path() + QStringLiteral("/migration-state.json.tmp")));
}

// --- 原子写中断：仅 .tmp 存在、目标缺失时 load 回退初始态 ---

TEST(UT_MigrationState, AtomicWriteInterruptLoadFallsBack)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");
    const QString tmpPath = path + QStringLiteral(".tmp");

    // 模拟写中途：只留 .tmp，目标文件不存在
    {
        QFile tmp(tmpPath);
        ASSERT_TRUE(tmp.open(QIODevice::WriteOnly));
        tmp.write(R"({"state":"Migrating","substage":"ScanDone"})");
        tmp.close();
    }
    ASSERT_FALSE(QFileInfo::exists(path));

    MigrationStateMachine sm(path);
    EXPECT_TRUE(sm.load()); // 目标缺失 -> 初始态
    EXPECT_EQ(sm.currentState(), MigrationState::Pending);
}

// --- setCursor 持久化 ---

TEST(UT_MigrationState, SetCursorPersists)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/migration-state.json");

    MigrationStateMachine sm(path);
    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Migrating, MigrationSubstage::ScanDone, "m"));
    sm.setCursor(makeCursor(99, 5));

    MigrationStateMachine loaded(path);
    ASSERT_TRUE(loaded.load());
    EXPECT_EQ(loaded.cursor().value("lastNoteId").toInt(), 99);
    EXPECT_EQ(loaded.cursor().value("processedCount").toInt(), 5);
}

// --- 续传重试：PartialCompleted -> Pending 重置 ---

TEST(UT_MigrationState, PartialCompletedCanResetForRetry)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);

    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Migrating, MigrationSubstage::ScanDone, "m"));
    sm.markCancelled();
    ASSERT_EQ(sm.currentState(), MigrationState::PartialCompleted);

    EXPECT_TRUE(sm.requestTransition(MigrationState::Pending, MigrationSubstage::None,
                                     QStringLiteral("retry")));
    EXPECT_EQ(sm.currentState(), MigrationState::Pending);
}

// --- Failed -> Pending 重置（回滚完成后重试） ---

TEST(UT_MigrationState, FailedCanResetForRetry)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);

    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Failed, MigrationSubstage::None,
                                     QStringLiteral("backup failed")));
    EXPECT_EQ(sm.currentState(), MigrationState::Failed);
    EXPECT_FALSE(sm.isResumable());

    EXPECT_TRUE(sm.requestTransition(MigrationState::Pending, MigrationSubstage::None,
                                     QStringLiteral("reset after rollback")));
    EXPECT_EQ(sm.currentState(), MigrationState::Pending);
}

// --- NotNeeded 终态 ---

TEST(UT_MigrationState, ScanToNotNeededWhenNothingToMigrate)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    MigrationStateMachine sm = makeMachine(dir);

    ASSERT_TRUE(sm.requestTransition(MigrationState::BackingUp, MigrationSubstage::None, "b"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::Scanning, MigrationSubstage::BackupDone, "s"));
    ASSERT_TRUE(sm.requestTransition(MigrationState::NotNeeded, MigrationSubstage::ScanDone,
                                     QStringLiteral("nothing to migrate")));
    EXPECT_EQ(sm.currentState(), MigrationState::NotNeeded);
    EXPECT_FALSE(sm.isResumable());
}

// --- 默认文件路径非空 ---

TEST(UT_MigrationState, DefaultFilePathIsSet)
{
    const QString path = MigrationStateMachine::defaultFilePath();
    EXPECT_FALSE(path.isEmpty());
    EXPECT_TRUE(path.endsWith(QStringLiteral("migration/migration-state.json")));
}
