// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONSCANNER_H
#define MIGRATIONSCANNER_H

#include <QString>
#include <QVector>

#include <atomic>

class QSqlQuery;

// TTP-017: 只读扫描错误码。
// 对齐集成数据契约「扫描结果」：失败时给出可区分原因码 + 描述。
enum class ScanErrorCode {
    None,          // 无错误（成功）
    DbOpenFailed,  // 数据库文件不存在或只读打开失败
    QueryFailed,   // 查询执行或逐行读取失败（含库结构损坏）
    Aborted        // 协作取消（cancel() 触发，TTP-020 兜底 PartialCompleted）
};

// 异常清单条目：note_id + 原因串。
// reason 复用 LegacyFormatDetector::formatName() 词汇 + 简述，便于 TTP-019 汇总。
struct ScanAbnormal {
    qint32 noteId = 0;
    QString reason;
};

// TTP-017: 只读扫描结果（对齐集成数据契约「扫描结果」行）。
// success=true 时四类计数 + 清单完整可用；失败时 code/message 给出原因，
// 计数/清单反映已处理部分（取消语义下为部分结果，由 TTP-020 兜底）。
struct ScanResult {
    bool success = false;
    int totalCount = 0;
    int needMigrateCount = 0;
    int alreadyTiptapCount = 0;
    int abnormalCount = 0;
    QVector<qint32> needMigrateNoteIds;   // 仅 note_id（GAP-1：不持 meta_data）
    QVector<ScanAbnormal> abnormals;      // note_id + 原因
    ScanErrorCode code = ScanErrorCode::None;
    QString message;
};

// TTP-017: 只读扫描和预检模块。
// 在迁移链路进入 Scanning 态时，对 vnote_items_tbl 全量笔记做一次只读体检：
// 逐条复用 LegacyFormatDetector::detect() 归类，统计总数 / 需迁移数 / 已是 Tiptap 数 /
// 异常数，产出「需迁移笔记清单（仅 note_id）」与「异常清单（note_id + 原因）」，
// 以结构化 ScanResult 回报。
//
// 只读边界：不写 DB（全程只读 SELECT，独立只读连接）、不开写事务、不持长锁、
// 不改迁移状态、不调状态机、不执行转换、不调信封校验、不生成报告、不调度、不落盘清单
// （D8：每次重新扫描，只读无副作用）。状态走向（Scanning→Migrating / NotNeeded / Failed）
// 由 TTP-020 依回报驱动，本模块不决策。
class MigrationScanner
{
public:
    // 默认构造：使用 defaultDbPath() 定位业务库（与 TTP-016 同源）。
    MigrationScanner();
    // 注入 db 路径（测试用，便于指向临时 SQLite 库）。
    explicit MigrationScanner(const QString &dbPath);

    // 执行只读全量扫描，返回结构化 ScanResult。
    ScanResult scan();

    // 协作取消：设置标志位，扫描循环在下一行检查点快速退出并返回 Aborted。
    // 可由 TTP-020 从另一线程调用；仅快速退出，已处理行计数进入部分 ScanResult。
    void cancel();

    // 默认业务库路径定位（只读复用 VNoteDbManager::DBVERSION / DEEPIN_VOICE_NOTE 逻辑）。
    static QString defaultDbPath();

    QString dbPath() const;

protected:
    // 每处理完一行后回调（默认空实现）。子类可重写以驱动协作取消或进度上报，
    // 扫描主循环在回调后的下一行检查取消标志位。
    virtual void onRowProcessed(int processedCount);

    // P3: 判定 query.next() 循环退出后是否因读取错误（非游标正常耗尽）而失败。
    // 默认实现检查 QSqlQuery::lastError()；测试子类可重写以注入中途失败。
    virtual bool checkIterationFailure(const QSqlQuery &query) const;

private:
    QString effectiveDbPath() const;
    // 进程级唯一只读连接名，避免与 VNoteDbManager 运行态连接争用。
    QString makeConnectionName() const;

    QString m_dbPath;
    std::atomic<bool> m_cancelRequested { false };
};


#endif // MIGRATIONSCANNER_H
