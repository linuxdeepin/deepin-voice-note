// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/dbmigration/migrationreport.h"
#include "importolddata/dbmigration/migrationstate.h"
#include "importolddata/dbmigration/migrationwriter.h"

#include "gtest/gtest.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QTemporaryDir>

#include <sys/stat.h>  // chmod
#include <unistd.h>    // getuid

namespace {

// 构造一条写回回报。
WriteResult makeResult(qint32 noteId, bool success, WriteErrorCode code,
                       const QVector<MigrationWarning> &warnings = {})
{
    WriteResult r;
    r.noteId = noteId;
    r.success = success;
    r.code = code;
    r.message = success ? QStringLiteral("ok") : QStringLiteral("fail");
    r.originalDataPreserved = true;
    r.warnings = warnings;
    return r;
}

MigrationWarning makeWarning(const QString &code, const QString &path = QStringLiteral("p"),
                             const QString &message = QStringLiteral("m"))
{
    MigrationWarning w;
    w.path = path;
    w.code = code;
    w.message = message;
    return w;
}

// 构造一份典型输入：3 成功（1 降级）/ 1 失败 / 1 异常。
MigrationReportInput makeSampleInput()
{
    MigrationReportInput input;
    input.totalCount = 5;
    input.needMigrateCount = 4;   // 3 success + 1 failed
    input.alreadyTiptapCount = 1; // skipped
    input.abnormalCount = 1;
    input.abnormalNoteIds = { 99 };
    input.backupPath = QStringLiteral("/tmp/backup/test.db.20260725-100000");
    input.elapsedMs = 12345;
    input.finalState = MigrationState::Completed;
    input.cancelled = false;

    QVector<WriteResult> results;
    results.append(makeResult(1, true, WriteErrorCode::None));
    // note 2: 成功但含降级码 → 同时计入 success 与 downgraded
    results.append(makeResult(2, true, WriteErrorCode::None,
                              { makeWarning(QStringLiteral("downgraded-html-block")) }));
    results.append(makeResult(3, true, WriteErrorCode::None,
                              { makeWarning(QStringLiteral("unsupported-html-style")) }));
    // note 4: 失败
    results.append(makeResult(4, false, WriteErrorCode::ConvertFailed,
                              { makeWarning(QStringLiteral("dangerous-html-node")) }));
    input.writeResults = results;
    return input;
}

}  // namespace

// --- 默认路径定位 ---

TEST(UT_MigrationReport, DefaultReportDirSameRootAsBackup)
{
    const QString dir = MigrationReport::defaultReportDir();
    EXPECT_FALSE(dir.isEmpty());
    EXPECT_TRUE(dir.endsWith(QStringLiteral("migration/report")));
}

// --- isDowngradeWarningCode 表驱动（D17=A 口径） ---

TEST(UT_MigrationReport, IsDowngradeWarningCodeTableDriven)
{
    // 前缀 downgraded- 命中
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("downgraded-html-block")));
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("downgraded-inline-element")));
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("downgraded-base64-image")));
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("downgraded-html-link")));

    // 显式集合
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("dangerous-html-node")));
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("dangerous-html-attribute")));
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("missing-html-image-src")));
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("unsafe-html-image-src")));
    EXPECT_TRUE(MigrationReport::isDowngradeWarningCode(QStringLiteral("skipped-non-text-block")));

    // 非降级码
    EXPECT_FALSE(MigrationReport::isDowngradeWarningCode(QStringLiteral("unsupported-html-style")));
    EXPECT_FALSE(MigrationReport::isDowngradeWarningCode(QStringLiteral("invalid-html-style-value")));
    EXPECT_FALSE(MigrationReport::isDowngradeWarningCode(QStringLiteral("depth-exceeded")));
    EXPECT_FALSE(MigrationReport::isDowngradeWarningCode(QStringLiteral("parse-failed")));
    EXPECT_FALSE(MigrationReport::isDowngradeWarningCode(QStringLiteral("missing-voicebox-jsonkey")));
    EXPECT_FALSE(MigrationReport::isDowngradeWarningCode(QString()));
}

// --- buildReportObject 计数正确性 ---

TEST(UT_MigrationReport, BuildReportObjectCounts)
{
    const MigrationReportInput input = makeSampleInput();
    const QJsonObject report = MigrationReport::buildReportObject(input);

    EXPECT_EQ(report.value(QStringLiteral("schemaVersion")).toInt(), 1);

    const QJsonObject counts = report.value(QStringLiteral("counts")).toObject();
    EXPECT_EQ(counts.value(QStringLiteral("total")).toInt(), 5);
    EXPECT_EQ(counts.value(QStringLiteral("needMigrate")).toInt(), 4);
    EXPECT_EQ(counts.value(QStringLiteral("success")).toInt(), 3);
    EXPECT_EQ(counts.value(QStringLiteral("failed")).toInt(), 1);
    EXPECT_EQ(counts.value(QStringLiteral("downgraded")).toInt(), 2); // note2 + note4
    EXPECT_EQ(counts.value(QStringLiteral("skipped")).toInt(), 1);
    EXPECT_EQ(counts.value(QStringLiteral("abnormal")).toInt(), 1);

    // 不变量：needMigrate == success + failed
    EXPECT_EQ(counts.value(QStringLiteral("needMigrate")).toInt(),
              counts.value(QStringLiteral("success")).toInt()
                  + counts.value(QStringLiteral("failed")).toInt());

    // 降级口径：降级但成功者（note2）同时计入 success 与 downgraded
    EXPECT_GE(counts.value(QStringLiteral("success")).toInt(), 1);
    EXPECT_GE(counts.value(QStringLiteral("downgraded")).toInt(), 1);
}

// --- buildReportObject failedNoteIds / abnormalNoteIds / cancelled ---

TEST(UT_MigrationReport, BuildReportObjectIdsAndCancelled)
{
    MigrationReportInput input = makeSampleInput();
    input.finalState = MigrationState::PartialCompleted;
    input.cancelled = true;

    const QJsonObject report = MigrationReport::buildReportObject(input);

    const QJsonArray failedIds = report.value(QStringLiteral("failedNoteIds")).toArray();
    ASSERT_EQ(failedIds.size(), 1);
    EXPECT_EQ(failedIds.at(0).toInt(), 4);

    const QJsonArray abnormalIds = report.value(QStringLiteral("abnormalNoteIds")).toArray();
    ASSERT_EQ(abnormalIds.size(), 1);
    EXPECT_EQ(abnormalIds.at(0).toInt(), 99);

    EXPECT_EQ(report.value(QStringLiteral("cancelled")).toBool(), true);
    EXPECT_EQ(report.value(QStringLiteral("finalState")).toString(),
              QStringLiteral("PartialCompleted"));
    EXPECT_EQ(report.value(QStringLiteral("elapsedMs")).toInt(), 12345);
    EXPECT_EQ(report.value(QStringLiteral("backupPath")).toString(), input.backupPath);
}

// --- buildReportObject warnings 聚合带 noteId + 隐私断言 ---

TEST(UT_MigrationReport, BuildReportObjectWarningsAndPrivacy)
{
    const MigrationReportInput input = makeSampleInput();
    const QJsonObject report = MigrationReport::buildReportObject(input);

    const QJsonArray warnings = report.value(QStringLiteral("warnings")).toArray();
    // note2(1) + note3(1) + note4(1) = 3 条
    ASSERT_EQ(warnings.size(), 3);

    // 每条 warning 带 noteId + path + code + message
    for (const QJsonValue &value : warnings) {
        const QJsonObject entry = value.toObject();
        EXPECT_TRUE(entry.contains(QStringLiteral("noteId")));
        EXPECT_TRUE(entry.contains(QStringLiteral("path")));
        EXPECT_TRUE(entry.contains(QStringLiteral("code")));
        EXPECT_TRUE(entry.contains(QStringLiteral("message")));
    }

    // note2 的 warning 标注了正确 noteId
    const QJsonObject first = warnings.at(0).toObject();
    EXPECT_EQ(first.value(QStringLiteral("noteId")).toInt(), 2);
    EXPECT_EQ(first.value(QStringLiteral("code")).toString(), QStringLiteral("downgraded-html-block"));

    // 隐私断言：报告整体不含正文 / meta_data / envelope / htmlCode 键
    const QByteArray serialized = QJsonDocument(report).toJson(QJsonDocument::Compact);
    const QString text = QString::fromUtf8(serialized);
    EXPECT_FALSE(text.contains(QStringLiteral("meta_data"), Qt::CaseInsensitive));
    EXPECT_FALSE(text.contains(QStringLiteral("envelope"), Qt::CaseInsensitive));
    EXPECT_FALSE(text.contains(QStringLiteral("htmlCode"), Qt::CaseInsensitive));
    EXPECT_FALSE(text.contains(QStringLiteral("content"), Qt::CaseInsensitive));
    EXPECT_FALSE(text.contains(QStringLiteral("metaData"), Qt::CaseInsensitive));
}

// --- generate 落盘成功（QTemporaryDir 注入、可解析 JSON、命名匹配） ---

TEST(UT_MigrationReport, GenerateWritesParseableJson)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    MigrationReport report(dir.path());
    const ReportResult result = report.generate(makeSampleInput());

    EXPECT_TRUE(result.success) << qPrintable(result.message);
    EXPECT_EQ(result.code, ReportErrorCode::None);
    EXPECT_TRUE(QFileInfo::exists(result.reportPath));

    // 命名匹配 migration-report.<yyyyMMdd-HHmmss>.json
    const QString name = QFileInfo(result.reportPath).fileName();
    const QRegularExpression re(QStringLiteral("^migration-report\\.\\d{8}-\\d{6}(\\.\\d+)?\\.json$"));
    EXPECT_TRUE(re.match(name).hasMatch()) << qPrintable(name);

    // 可解析 JSON，关键字段在位
    QFile f(result.reportPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    ASSERT_TRUE(doc.isObject());
    const QJsonObject obj = doc.object();
    EXPECT_EQ(obj.value(QStringLiteral("schemaVersion")).toInt(), 1);
    EXPECT_EQ(obj.value(QStringLiteral("finalState")).toString(), QStringLiteral("Completed"));
    EXPECT_FALSE(obj.value(QStringLiteral("cancelled")).toBool());
}

// --- generate 连续两次不覆盖 ---

TEST(UT_MigrationReport, GenerateDoesNotOverwrite)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    MigrationReport report(dir.path());
    const MigrationReportInput input = makeSampleInput();

    const ReportResult first = report.generate(input);
    ASSERT_TRUE(first.success) << qPrintable(first.message);

    // 同秒内连续第二次：不应覆盖第一份
    const ReportResult second = report.generate(input);
    ASSERT_TRUE(second.success) << qPrintable(second.message);

    EXPECT_NE(first.reportPath, second.reportPath);
    EXPECT_TRUE(QFileInfo::exists(first.reportPath));
    EXPECT_TRUE(QFileInfo::exists(second.reportPath));

    // 两份均可独立解析
    for (const QString &path : { first.reportPath, second.reportPath }) {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::ReadOnly));
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        EXPECT_TRUE(doc.isObject());
    }
}

// --- 失败：非终态 InputInvalid ---

TEST(UT_MigrationReport, GenerateRejectsNonTerminalState)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    MigrationReportInput input = makeSampleInput();
    input.finalState = MigrationState::Migrating;  // 非终态

    MigrationReport report(dir.path());
    const ReportResult result = report.generate(input);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, ReportErrorCode::InputInvalid);
    // 不产任何文件
    EXPECT_FALSE(QFileInfo::exists(result.reportPath));
}

TEST(UT_MigrationReport, GenerateRejectsPendingState)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    MigrationReportInput input = makeSampleInput();
    input.finalState = MigrationState::Pending;

    MigrationReport report(dir.path());
    const ReportResult result = report.generate(input);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, ReportErrorCode::InputInvalid);
}

// --- 失败：目录不可创建 DirCreateFailed（父级是普通文件） ---

TEST(UT_MigrationReport, GenerateDirCreateFailed)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    // 报告目录路径的父级是一个普通文件，mkpath 无法在其下创建目录
    const QString blocker = dir.path() + QStringLiteral("/blocker");
    {
        QFile f(blocker);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
    }
    const QString badDir = blocker + QStringLiteral("/report");

    MigrationReport report(badDir);
    const ReportResult result = report.generate(makeSampleInput());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, ReportErrorCode::DirCreateFailed);
    // 不产半文件
    EXPECT_FALSE(QFileInfo::exists(badDir + QStringLiteral("/migration-report.json")));
}

// --- 失败：目录只读 WriteFailed（不抛异常、不产半文件） ---

TEST(UT_MigrationReport, GenerateWriteFailedReadOnlyDir)
{
    // root 绕过文件权限，无法触发只读失败
    if (getuid() == 0) {
        GTEST_SKIP() << "chmod 0500 ineffective as root; run as non-root user";
    }

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    const QString reportDir = dir.path() + QStringLiteral("/ro");
    ASSERT_TRUE(QDir().mkpath(reportDir));
    // 只读目录：可在其下查找但不可创建文件
    chmod(QFile::encodeName(reportDir).constData(), 0500);

    MigrationReport report(reportDir);
    const ReportResult result = report.generate(makeSampleInput());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, ReportErrorCode::WriteFailed);

    // 不产半文件：无 .json 也无 .tmp 残留
    QDir d(reportDir);
    const QStringList entries = d.entryList(QStringList() << QStringLiteral("*.json") << QStringLiteral("*.tmp"));
    EXPECT_TRUE(entries.isEmpty()) << qPrintable(entries.join(QStringLiteral(", ")));

    // 恢复权限以便 QTemporaryDir 清理
    chmod(QFile::encodeName(reportDir).constData(), 0700);
}
