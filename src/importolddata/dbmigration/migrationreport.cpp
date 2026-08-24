// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationreport.h"

#include "migrationhtmlcodes.h"  // warning 码词汇表（只读引用，不改义）

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QStandardPaths>

#include <cerrno>       // errno
#include <cstdio>       // std::rename
#include <cstring>      // strerror

namespace {

const char *kTmpSuffix = ".tmp";
const int kSchemaVersion = 1;

// D17=A 降级码显式集合（前缀 downgraded- 之外的额外码）。
// 只读引用 migrationhtmlcodes.h 的常量，不改义；skipped-non-text-block 为
// 词汇表未列但 D17 口径纳入的降级码，以本地字面量补充。
const QSet<QString> &extraDowngradeCodes()
{
    static const QSet<QString> codes {
        kCodeDangerousHtmlNode,
        kCodeDangerousHtmlAttribute,
        kCodeMissingHtmlImageSrc,
        kCodeUnsafeHtmlImageSrc,
        kCodeFileImageOutsideImagesDir,
        QStringLiteral("skipped-non-text-block")
    };
    return codes;
}

// 终态判定：Completed / PartialCompleted / Failed（D15 单次迁移终态）。
bool isTerminalState(MigrationState state)
{
    return state == MigrationState::Completed
        || state == MigrationState::PartialCompleted
        || state == MigrationState::Failed;
}

// 终态名序列化（与 migrationStateToString 复用同一词汇）。
QString finalStateName(MigrationState state)
{
    return migrationStateToString(state);
}

}  // namespace

MigrationReport::MigrationReport()
{
}

MigrationReport::MigrationReport(const QString &reportDir)
    : m_reportDir(reportDir)
{
}

QString MigrationReport::defaultReportDir()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + QDir::separator() + QStringLiteral("migration/report");
}

QString MigrationReport::reportDir() const
{
    return m_reportDir;
}

QString MigrationReport::effectiveReportDir() const
{
    return m_reportDir.isEmpty() ? defaultReportDir() : m_reportDir;
}

QString MigrationReport::generateReportName() const
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return QStringLiteral("migration-report.%1.json").arg(timestamp);
}

QString MigrationReport::resolveUniquePath(const QString &dir, const QString &baseName) const
{
    // 落盘不覆盖：同名时追加序号，直到找到一个不存在的路径。
    QString path = dir + QDir::separator() + baseName;
    if (!QFileInfo::exists(path)) {
        return path;
    }

    const QString prefix = QStringLiteral("migration-report.");
    const QString suffix = QStringLiteral(".json");
    // 从基础名中提取时间戳部分，构造 <name>.<seq>.json
    const int prefixLen = prefix.size();
    const int suffixLen = suffix.size();
    QString stamp = baseName.mid(prefixLen, baseName.size() - prefixLen - suffixLen);

    int seq = 2;
    while (true) {
        const QString name = QStringLiteral("migration-report.%1.%2.json").arg(stamp).arg(seq);
        path = dir + QDir::separator() + name;
        if (!QFileInfo::exists(path)) {
            return path;
        }
        ++seq;
    }
}

bool MigrationReport::isDowngradeWarningCode(const QString &code)
{
    // D17=A 口径：前缀 downgraded- 命中，或属于显式降级码集合。
    return code.startsWith(QStringLiteral("downgraded-")) || extraDowngradeCodes().contains(code);
}

QJsonObject MigrationReport::buildReportObject(const MigrationReportInput &input)
{
    // --- 从 writeResults 计算 success / failed / failedNoteIds / downgraded ---
    int success = 0;
    int failed = 0;
    QJsonArray failedNoteIds;
    QJsonArray warnings;
    int downgraded = 0;

    for (const WriteResult &result : input.writeResults) {
        if (result.success) {
            ++success;
        } else {
            ++failed;
            failedNoteIds.append(result.noteId);
        }

        // 逐条遍历 warnings 标注 noteId；同时判定该 note 是否含降级码。
        bool noteDowngraded = false;
        for (const MigrationWarning &warning : result.warnings) {
            QJsonObject entry;
            entry.insert(QStringLiteral("noteId"), result.noteId);
            entry.insert(QStringLiteral("path"), warning.path);
            entry.insert(QStringLiteral("code"), warning.code);
            entry.insert(QStringLiteral("message"), warning.message);
            warnings.append(entry);

            if (isDowngradeWarningCode(warning.code)) {
                noteDowngraded = true;
            }
        }
        // D17=A：凡 note 存在任意降级码即计一次 downgraded
        // （降级但成功者同时计入 success 与 downgraded）。
        if (noteDowngraded) {
            ++downgraded;
        }
    }

    // 续传累计计数优先：success/failed 使用从游标恢复的全量计数，保证与
    // total/needMigrate 同为全量口径（writeResults 在续传段仅含本次明细）。
    // cumulativeSuccess/cumulativeFail < 0 表示未提供，回退到上方重算值。
    if (input.cumulativeSuccess >= 0) {
        success = input.cumulativeSuccess;
    }
    if (input.cumulativeFail >= 0) {
        failed = input.cumulativeFail;
    }

    // --- counts ---
    QJsonObject counts;
    counts.insert(QStringLiteral("total"), input.totalCount);
    counts.insert(QStringLiteral("needMigrate"), input.needMigrateCount);
    counts.insert(QStringLiteral("success"), success);
    counts.insert(QStringLiteral("failed"), failed);
    counts.insert(QStringLiteral("downgraded"), downgraded);
    counts.insert(QStringLiteral("skipped"), input.alreadyTiptapCount);
    counts.insert(QStringLiteral("abnormal"), input.abnormalCount);

    // --- abnormalNoteIds ---
    QJsonArray abnormalNoteIds;
    for (qint32 id : input.abnormalNoteIds) {
        abnormalNoteIds.append(id);
    }

    // --- 顶层 schemaVersion:1 ---
    QJsonObject report;
    report.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    report.insert(QStringLiteral("counts"), counts);
    report.insert(QStringLiteral("failedNoteIds"), failedNoteIds);
    report.insert(QStringLiteral("abnormalNoteIds"), abnormalNoteIds);
    report.insert(QStringLiteral("warnings"), warnings);
    report.insert(QStringLiteral("elapsedMs"), input.elapsedMs);
    report.insert(QStringLiteral("backupPath"), input.backupPath);
    report.insert(QStringLiteral("finalState"), finalStateName(input.finalState));
    report.insert(QStringLiteral("cancelled"), input.cancelled);
    report.insert(QStringLiteral("writeResultsScope"), input.writeResultsScope);
    return report;
}

bool MigrationReport::atomicWrite(const QString &path, const QByteArray &payload)
{
    // 原子写：写临时文件后 rename 覆盖目标（与 migrationstatepersistent.cpp 一致）。
    // Qt 的 QFile::rename 在目标已存在时返回 false 不覆盖，故用 std::rename。
    const QString tmpPath = path + QLatin1String(kTmpSuffix);
    {
        QFile tmp(tmpPath);
        if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning("MigrationReport: open tmp failed: %s (%s)",
                     qPrintable(tmpPath), qPrintable(tmp.errorString()));
            return false;
        }
        if (tmp.write(payload) != payload.size()) {
            qWarning("MigrationReport: write tmp failed: %s (%s)",
                     qPrintable(tmpPath), qPrintable(tmp.errorString()));
            tmp.close();
            QFile::remove(tmpPath);
            return false;
        }
        tmp.flush();
        tmp.close();
    }

    if (std::rename(QFile::encodeName(tmpPath).constData(),
                    QFile::encodeName(path).constData()) != 0) {
        qWarning("MigrationReport: rename failed: %s -> %s (%s)",
                 qPrintable(tmpPath), qPrintable(path),
                 qPrintable(QString::fromLocal8Bit(strerror(errno))));
        QFile::remove(tmpPath);
        return false;
    }
    return true;
}

ReportResult MigrationReport::generate(const MigrationReportInput &input)
{
    ReportResult result;

    // 1. 校验 finalState 为终态（D15：仅终态产出报告）。
    if (!isTerminalState(input.finalState)) {
        result.code = ReportErrorCode::InputInvalid;
        result.message = QStringLiteral("finalState is not terminal: %1")
                             .arg(finalStateName(input.finalState));
        qWarning("MigrationReport: %s", qPrintable(result.message));
        return result;
    }

    // 2. 聚合报告对象。
    const QJsonObject reportObject = buildReportObject(input);

    // 3. 序列化（QJsonDocument::Indented，便于人工排查）。
    const QByteArray payload = QJsonDocument(reportObject).toJson(QJsonDocument::Indented);

    // 4. 确保报告目录存在。
    const QString dir = effectiveReportDir();
    if (!QDir().exists(dir) && !QDir().mkpath(dir)) {
        result.code = ReportErrorCode::DirCreateFailed;
        result.message = QStringLiteral("Failed to create report directory: %1").arg(dir);
        qWarning("MigrationReport: %s", qPrintable(result.message));
        return result;
    }

    // 5. 时间戳命名 + 落盘不覆盖。
    const QString baseName = generateReportName();
    const QString path = resolveUniquePath(dir, baseName);

    // 6. 原子写（临时文件 + rename）。
    if (!atomicWrite(path, payload)) {
        result.code = ReportErrorCode::WriteFailed;
        result.message = QStringLiteral("Failed to write report: %1").arg(path);
        qWarning("MigrationReport: %s", qPrintable(result.message));
        return result;
    }

    result.success = true;
    result.reportPath = path;
    result.code = ReportErrorCode::None;
    result.message = QStringLiteral("Report generated: %1").arg(path);
    qInfo("MigrationReport: %s", qPrintable(result.message));
    return result;
}
