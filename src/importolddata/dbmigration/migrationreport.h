// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONREPORT_H
#define MIGRATIONREPORT_H

#include "migrationstate.h"   // MigrationState 终态枚举
#include "migrationwriter.h"  // WriteResult / MigrationWarning（GAP-2 锁定结构）

#include <QJsonObject>
#include <QString>
#include <QVector>

// TTP-019: 迁移报告落盘错误码。
// 对齐集成数据契约「报告结果」：失败时给出可区分原因码 + 描述，不抛异常。
enum class ReportErrorCode {
    None,             // 无错误（成功）
    InputInvalid,     // finalState 非终态（Completed/PartialCompleted/Failed）
    DirCreateFailed,  // 报告目录创建失败
    WriteFailed       // 临时文件写 / rename 失败
};

// TTP-019: 报告落盘结果。
// success=true 时 reportPath 为生成的报告文件路径；失败时 code/message 给出原因。
struct ReportResult {
    bool success = false;
    QString reportPath;
    ReportErrorCode code = ReportErrorCode::None;
    QString message;
};

// TTP-019: 报告输入。
// 聚合 TTP-017 扫描计数 + abnormalNoteIds、TTP-018 逐条写回回报（含 warnings）、
// TTP-016 备份路径、TTP-020 耗时、TTP-015 终态 + 取消标记。
// 不持 meta_data / envelope / htmlCode（隐私边界，D15 单次迁移）。
struct MigrationReportInput {
    // TTP-017 扫描计数
    int totalCount = 0;
    int needMigrateCount = 0;
    int alreadyTiptapCount = 0;
    int abnormalCount = 0;
    QVector<qint32> abnormalNoteIds;  // 异常 note_id 清单（仅 id）

    // TTP-018 逐条写回回报（含 warnings）
    QVector<WriteResult> writeResults;

    // TTP-016 备份路径
    QString backupPath;

    // TTP-020 迁移耗时（毫秒）
    qint64 elapsedMs = 0;

    // TTP-015 终态 + 取消标记（D3 取消映射 PartialCompleted）
    MigrationState finalState = MigrationState::Pending;
    bool cancelled = false;
};

// TTP-019: 迁移报告模块。
// 在迁移链路进入终态（Completed / PartialCompleted / Failed）时，由 TTP-020 编排层
// 调用 generate()，聚合终态及迁移过程数据，产出一份可落盘、不含隐私正文的 JSON
// 迁移报告（含总数 / 需迁移数 / 成功 / 跳过 / 降级 / 失败计数 + 失败 note id 清单 +
// warning 列表 + 耗时 + 备份路径 + cancelled 标记），落盘不覆盖，供用户反馈和问题排查。
//
// 边界：只提供「终态报告」生成能力；不扫描 / 写回 / 备份 / 改状态 / 调度 / 接 UI；
// 不读笔记正文 / meta_data / envelope / htmlCode；不跨次累计、不产中间报告（D15）。
// 落盘失败仅记日志返回 ReportErrorCode，不抛异常、不影响迁移结果。
class MigrationReport
{
public:
    // 默认构造：使用 defaultReportDir() 定位报告目录。
    MigrationReport();
    // 注入报告目录（测试用，便于指向 QTemporaryDir）。
    explicit MigrationReport(const QString &reportDir);

    // 生成并落盘报告：校验终态 → 聚合 → 序列化 → 确保目录 → 时间戳命名 →
    // 临时文件 + rename 原子写。落盘失败仅记日志返回 ReportErrorCode，不抛异常。
    ReportResult generate(const MigrationReportInput &input);

    // 默认报告目录：<AppDataLocation>/migration/report（与 TTP-016 backup 同根）。
    static QString defaultReportDir();

    // 聚合报告 JSON 对象（schemaVersion:1）。纯函数，单测直接校验计数 / 降级口径。
    static QJsonObject buildReportObject(const MigrationReportInput &input);
    // 判定 warning 码是否为降级码（D17=A：前缀 downgraded- + 显式集合）。
    // 只读引用 migrationhtmlcodes.h 词汇表，不自创 / 不改义。
    static bool isDowngradeWarningCode(const QString &code);

    QString reportDir() const;

private:
    // 解析当前生效的报告目录（注入优先，否则 defaultReportDir()）。
    QString effectiveReportDir() const;
    // 生成时间戳报告文件名：migration-report.<yyyyMMdd-HHmmss>.json
    QString generateReportName() const;
    // 在目录下解析一个不与已有文件冲突的最终路径（落盘不覆盖）。
    QString resolveUniquePath(const QString &dir, const QString &baseName) const;
    // 原子写：写临时文件后 rename 覆盖目标（与 migrationstatepersistent.cpp 一致）。
    // 失败时清理临时文件并返回 false。
    bool atomicWrite(const QString &path, const QByteArray &payload);

    QString m_reportDir;
};

#endif  // MIGRATIONREPORT_H
