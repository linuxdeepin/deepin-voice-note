// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONWRITER_H
#define MIGRATIONWRITER_H

#include <QString>
#include <QVector>

// Forward declarations — 仅头文件需要的类型，实现见 .cpp。
class QSqlDatabase;
class QJsonObject;

enum class LegacyFormat;

// TTP-018: 单条事务写回错误码。
// 对齐集成数据契约「单条写回回报」+ GAP-2：失败时给出可区分原因码 + 描述。
enum class WriteErrorCode {
    None,               // 成功
    NoteNotFound,       // note_id 查无此行
    ReadFailed,         // 读取 meta_data/encrypt 的 DB 错误或库文件不可用
    UnsupportedFormat,  // detect() 返回 Invalid（不可识别，无法转换）
    ConvertFailed,      // 转换器返回 errors（result.ok()==false）
    ValidationFailed,   // MigrationJsonValidator::validateEnvelope() 不通过
    WriteFailed,        // UPDATE 执行失败或未按预期影响行
    Other
};

// 统一告警条目（归一化 TTP-013 两类转换器 warnings，GAP-2：path/code/message）。
struct MigrationWarning {
    QString path;
    QString code;
    QString message;
};

// 单条写回回报（GAP-2：成功与失败均附带 warnings[]）。
struct WriteResult {
    qint32 noteId = 0;
    bool success = false;
    WriteErrorCode code = WriteErrorCode::None;
    QString message;                   // 可区分原因的简述（不含正文/信封）
    bool originalDataPreserved = true; // 失败时是否原数据原样保留
    QVector<MigrationWarning> warnings;
};

// TTP-018: 单条事务写回模块。
// 在迁移链路 Migrating 态下，对单条笔记执行「读 meta_data → 探测分派 → 转换 →
// 信封校验 → 紧凑化 → 加密再入库 → 单条参数化 UPDATE → 结构化回报」闭环。
//
// 写回仅替换 vnote_items_tbl.meta_data 为 Tiptap 紧凑信封，写回前必须经
// MigrationJsonValidator::validateEnvelope() 通过；失败保留原数据、单条失败不连累
// 其他条；加密笔记照旧 base64 编码入库且不改加密标志（expand_filed2）。
//
// 边界：不扫描、不调度、不管状态、不生成报告、不复用 UpdateNoteDbVisitor、
// 不改 src/db/ 与 tiptapmigration/（只读复用 TTP-013 零件）。自带 RW QSqlDatabase
// 连接（仿 MigrationScanner），单条参数化 UPDATE（单语句原子），不启用事务开关。
class MigrationWriter
{
public:
    // 默认构造：使用 defaultDbPath() 定位业务库（与 scanner/backup 同源）。
    MigrationWriter();
    // 注入 db 路径（测试用，便于指向临时 SQLite 库）。
    explicit MigrationWriter(const QString &dbPath);

    // 对单条笔记执行完整闭环，返回结构化回报。同步、不抛异常。
    WriteResult writeOne(qint32 folderId, qint32 noteId);

    // 默认业务库路径定位（只读复用 VNoteDbManager::DBVERSION / DEEPIN_VOICE_NOTE 逻辑）。
    static QString defaultDbPath();

    QString dbPath() const;

private:
    // 读取单行 meta_data + encrypt 的中间结果。
    // found=false 表示查无此行（NoteNotFound）或读取失败（ReadFailed），
    // 此时 result->code/message 已由本方法填入。
    struct NoteRow {
        bool found = false;
        int encryption = 0;     // expand_filed2 原值（用于读解密与写再加密）
        QString metaData;       // 已按 encryption 解密还原后的 meta_data 字符串
    };

    QString effectiveDbPath() const;
    // 进程级唯一 RW 连接名，避免与 VNoteDbManager 运行态连接争用。
    QString makeConnectionName() const;

    // 读取单行笔记。失败时设置 result 的 ReadFailed/NoteNotFound 并日志，found=false。
    NoteRow readNoteRow(QSqlDatabase &db, qint32 noteId, qint32 folderId,
                        WriteResult *result) const;

    // 按探测格式分派转换，产出信封并归一化 warnings。
    // 转换失败时设置 result 的 ConvertFailed 并日志，返回空 envelope。
    QJsonObject convertByFormat(LegacyFormat format, const QString &metaDataStr,
                                qint32 folderId, qint32 noteId,
                                QVector<MigrationWarning> *warnings,
                                WriteResult *result) const;

    // 信封校验 → 紧凑化 → 加密再入库 → 单条参数化 UPDATE。
    // 成功设置 result.success；失败设置 ValidationFailed/WriteFailed 并日志。
    void validateAndPersist(QSqlDatabase &db, const QJsonObject &envelope,
                            int encryption, qint32 noteId, qint32 folderId,
                            LegacyFormat format,
                            const QVector<MigrationWarning> &warnings,
                            WriteResult *result) const;

    QString m_dbPath;
};

#endif  // MIGRATIONWRITER_H
