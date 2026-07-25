// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationwriter.h"

#include "legacyformatdetector.h"       // LegacyFormatDetector::detect()/formatName()
#include "migrationhtmlconverter.h"     // MigrationHtmlConverter::convert()
#include "migrationjsonbuilder.h"       // MigrationJsonBuilder::toCompactJson()/makeEnvelope()/...
#include "migrationjsonvalidator.h"     // MigrationJsonValidator::validateEnvelope()
#include "migrationnotedataconverter.h" // MigrationNoteDataConverter::convertBlocks()
#include "db/dbvisitor.h"               // DbVisitor::DBNote 列枚举/列名
#include "db/vnotedbmanager.h"          // VNoteDbManager::NOTES_TABLE_NAME/DBVERSION
#include "globaldef.h"                  // DEEPIN_VOICE_NOTE

#include <QAtomicInt>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace {
// 读取 SELECT 取列顺序（按 SELECT 列表的位置索引读取，避免依赖表全列序）。
constexpr int kColMetaData = 0;
constexpr int kColEncrypt = 1;
// SQLite busy 超时（毫秒），缓解与 VNoteDbManager 运行态连接的写锁竞争。
constexpr int kBusyTimeoutMs = 5000;
}  // namespace

MigrationWriter::MigrationWriter()
{
}

MigrationWriter::MigrationWriter(const QString &dbPath)
    : m_dbPath(dbPath)
{
}

QString MigrationWriter::dbPath() const
{
    return m_dbPath;
}

QString MigrationWriter::defaultDbPath()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + QDir::separator() + DEEPIN_VOICE_NOTE
           + QString(VNoteDbManager::DBVERSION) + QStringLiteral(".db");
}

QString MigrationWriter::effectiveDbPath() const
{
    return m_dbPath.isEmpty() ? defaultDbPath() : m_dbPath;
}

QString MigrationWriter::makeConnectionName() const
{
    // 进程级递增计数，保证多次写回/测试场景连接名唯一，用后 removeDatabase 释放。
    static QAtomicInt counter(0);
    return QStringLiteral("voice_note_migration_write_%1")
        .arg(counter.fetchAndAddRelaxed(1));
}

// 归一化 TTP-013 两类转换器 warnings（结构同构：path/code/message）为 MigrationWarning。
template <typename Issue>
QVector<MigrationWarning> collectWarnings(const QVector<Issue> &issues)
{
    QVector<MigrationWarning> warnings;
    warnings.reserve(issues.size());
    for (const Issue &issue : issues) {
        warnings.append(MigrationWarning{issue.path, issue.code, issue.message});
    }
    return warnings;
}

WriteResult MigrationWriter::writeOne(qint32 folderId, qint32 noteId)
{
    WriteResult result;
    result.noteId = noteId;

    const QString dbPath = effectiveDbPath();

    // 源库文件不存在直接失败，避免 SQLite 打开静默创建空库（写副作用防御）。
    if (!QFileInfo(dbPath).isFile()) {
        result.code = WriteErrorCode::ReadFailed;
        result.message = QStringLiteral("Database file not found: %1").arg(dbPath);
        qWarning("MigrationWriter: note_id=%lld folder_id=%lld %s",
                 static_cast<long long>(noteId),
                 static_cast<long long>(folderId),
                 qPrintable(result.message));
        return result;
    }

    const QString connName = makeConnectionName();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(dbPath);
        // 非只读连接，设 busy 超时缓解与运行态连接的写锁竞争。
        db.setConnectOptions(
            QStringLiteral("QSQLITE_BUSY_TIMEOUT=%1").arg(kBusyTimeoutMs));
        if (!db.open()) {
            result.code = WriteErrorCode::ReadFailed;
            result.message = QStringLiteral("Failed to open database: %1")
                                 .arg(db.lastError().text());
            qWarning("MigrationWriter: note_id=%lld folder_id=%lld %s",
                     static_cast<long long>(noteId),
                     static_cast<long long>(folderId),
                     qPrintable(result.message));
        } else {
            // 列名取自 DbVisitor::DBNote::noteColumnsName，与运行态查询同源
            // （encrypt 实际落在 expand_filed2 列）。
            const QStringList &cols = DbVisitor::DBNote::noteColumnsName;
            const QString selectSql = QStringLiteral("SELECT %1, %2 FROM %3 WHERE %4=?")
                                          .arg(cols.value(DbVisitor::DBNote::meta_data),
                                               cols.value(DbVisitor::DBNote::encrypt),
                                               VNoteDbManager::NOTES_TABLE_NAME,
                                               cols.value(DbVisitor::DBNote::note_id));

            QSqlQuery readQuery(db);
            readQuery.prepare(selectSql);
            readQuery.addBindValue(noteId);
            if (!readQuery.exec()) {
                result.code = WriteErrorCode::ReadFailed;
                result.message = QStringLiteral("Failed to read note: %1")
                                     .arg(readQuery.lastError().text());
                qWarning("MigrationWriter: note_id=%lld folder_id=%lld %s",
                         static_cast<long long>(noteId),
                         static_cast<long long>(folderId),
                         qPrintable(result.message));
            } else if (!readQuery.next()) {
                // 查无此行：原数据（无）保留，按 NoteNotFound 上报。
                result.code = WriteErrorCode::NoteNotFound;
                result.message = QStringLiteral("note_id not found");
                qInfo("MigrationWriter: note_id=%lld folder_id=%lld not found",
                      static_cast<long long>(noteId),
                      static_cast<long long>(folderId));
            } else {
                const int encryption = readQuery.value(kColEncrypt).toInt();
                const QVariant metaDataVar = readQuery.value(kColMetaData);
                // 加密还原：对齐 dbvisitor.cpp 读取语义，base64 还原后再探测/转换。
                const QString metaDataStr = (encryption != 0)
                    ? QString::fromUtf8(QByteArray::fromBase64(metaDataVar.toByteArray()))
                    : metaDataVar.toString();

                // 探测分派。
                const LegacyFormat format = LegacyFormatDetector::detect(metaDataStr);

                if (format == LegacyFormat::TiptapEnvelope) {
                    // 已是信封（用户已编辑或前次部分迁移已写），跳过写回，避免覆盖最新内容。
                    result.success = true;
                    result.code = WriteErrorCode::None;
                    result.originalDataPreserved = true;
                    result.message = QStringLiteral("already tiptap, skipped");
                    qInfo("MigrationWriter: note_id=%lld folder_id=%lld format=%s skipped",
                          static_cast<long long>(noteId),
                          static_cast<long long>(folderId),
                          qPrintable(LegacyFormatDetector::formatName(format)));
                } else if (format == LegacyFormat::Invalid) {
                    result.code = WriteErrorCode::UnsupportedFormat;
                    result.message = QStringLiteral("unrecognized meta_data format");
                    qWarning("MigrationWriter: note_id=%lld folder_id=%lld format=%s %s",
                             static_cast<long long>(noteId),
                             static_cast<long long>(folderId),
                             qPrintable(LegacyFormatDetector::formatName(format)),
                             qPrintable(result.message));
                } else {
                    // 转换分派：LegacyHtmlCode / LegacyNoteDatas / ProseMirrorDoc / PlainText。
                    QJsonObject envelope;
                    QVector<MigrationWarning> warnings;

                    switch (format) {
                    case LegacyFormat::LegacyHtmlCode: {
                        // htmlCode 可能是 JSON {"htmlCode":...} 包封，也可能是裸 HTML。
                        QString htmlCode = metaDataStr;
                        const QJsonDocument doc =
                            QJsonDocument::fromJson(metaDataStr.toUtf8());
                        if (doc.isObject()) {
                            const QJsonObject root = doc.object();
                            const QJsonValue code = root.value(QStringLiteral("htmlCode"));
                            if (code.isString()) {
                                htmlCode = code.toString();
                            }
                        }
                        const MigrationHtmlConversionResult converted =
                            MigrationHtmlConverter::convert(htmlCode);
                        warnings = collectWarnings(converted.warnings);
                        if (!converted.ok()) {
                            result.warnings = warnings;
                            result.code = WriteErrorCode::ConvertFailed;
                            result.message = QStringLiteral("html convert failed");
                            qWarning("MigrationWriter: note_id=%lld folder_id=%lld "
                                     "format=%s convert failed",
                                     static_cast<long long>(noteId),
                                     static_cast<long long>(folderId),
                                     qPrintable(LegacyFormatDetector::formatName(format)));
                            break;
                        }
                        envelope = converted.envelope;
                        break;
                    }
                    case LegacyFormat::LegacyNoteDatas: {
                        const MigrationNoteDataConversionResult converted =
                            MigrationNoteDataConverter::convertBlocks(metaDataStr);
                        warnings = collectWarnings(converted.warnings);
                        if (!converted.ok()) {
                            result.warnings = warnings;
                            result.code = WriteErrorCode::ConvertFailed;
                            result.message = QStringLiteral("noteDatas convert failed");
                            qWarning("MigrationWriter: note_id=%lld folder_id=%lld "
                                     "format=%s convert failed",
                                     static_cast<long long>(noteId),
                                     static_cast<long long>(folderId),
                                     qPrintable(LegacyFormatDetector::formatName(format)));
                            break;
                        }
                        envelope = converted.envelope;
                        break;
                    }
                    case LegacyFormat::ProseMirrorDoc: {
                        // R-PM1：无专用转换器，直接用 TTP-013 builder 包封已有 doc 对象。
                        const QJsonDocument doc =
                            QJsonDocument::fromJson(metaDataStr.toUtf8());
                        envelope = MigrationJsonBuilder::makeEnvelope(doc.object());
                        break;
                    }
                    case LegacyFormat::PlainText: {
                        // R-PM1：无专用转换器，包成单段落 doc（空串→空段落）。
                        QJsonArray paragraphContent;
                        if (!metaDataStr.isEmpty()) {
                            paragraphContent.append(
                                MigrationJsonBuilder::makeText(metaDataStr));
                        }
                        envelope = MigrationJsonBuilder::makeEnvelope(
                            MigrationJsonBuilder::makeDoc(
                                QJsonArray{MigrationJsonBuilder::makeParagraph(
                                    paragraphContent)}));
                        break;
                    }
                    default:
                        break;
                    }

                    // 转换失败已组装结果，直接跳过写回。
                    if (result.code == WriteErrorCode::ConvertFailed) {
                        // warnings 已填，originalDataPreserved 默认 true。
                    } else if (envelope.isEmpty()) {
                        result.warnings = warnings;
                        result.code = WriteErrorCode::ConvertFailed;
                        result.message = QStringLiteral("convert produced empty envelope");
                        qWarning("MigrationWriter: note_id=%lld folder_id=%lld empty envelope",
                                 static_cast<long long>(noteId),
                                 static_cast<long long>(folderId));
                    } else {
                        // 强制校验：写回前必须通过 C++ validator。
                        const MigrationJsonValidationResult validated =
                            MigrationJsonValidator::validateEnvelope(envelope);
                        if (!validated.ok()) {
                            result.warnings = warnings;
                            result.code = WriteErrorCode::ValidationFailed;
                            result.message = QStringLiteral(
                                "envelope validation failed: %1 error(s)")
                                                 .arg(validated.errors.size());
                            qWarning("MigrationWriter: note_id=%lld folder_id=%lld "
                                     "format=%s validation failed",
                                     static_cast<long long>(noteId),
                                     static_cast<long long>(folderId),
                                     qPrintable(LegacyFormatDetector::formatName(format)));
                        } else {
                            // 紧凑化（QJsonDocument::Compact）。
                            const QString compact =
                                MigrationJsonBuilder::toCompactJson(envelope);
                            // 加密再入库：对齐 dbvisitor.cpp 写入语义，不改 expand_filed2。
                            // 绑定 QString 走 TEXT 通道（与 UpdateNoteDbVisitor 一致），
                            // 避免 QByteArray 绑定落入 BLOB 导致 detect() 读回异常。
                            const QString stored = (encryption != 0)
                                ? QString::fromUtf8(compact.toLocal8Bit().toBase64())
                                : compact;

                            // 单条参数化 UPDATE（单语句原子），仅 SET meta_data，
                            // 不触 modify_time 列、不触 folder 表。
                            const QString updateSql =
                                QStringLiteral("UPDATE %1 SET %2=? WHERE %3=?")
                                    .arg(VNoteDbManager::NOTES_TABLE_NAME,
                                         cols.value(DbVisitor::DBNote::meta_data),
                                         cols.value(DbVisitor::DBNote::note_id));
                            QSqlQuery writeQuery(db);
                            writeQuery.prepare(updateSql);
                            writeQuery.addBindValue(stored);
                            writeQuery.addBindValue(noteId);
                            if (!writeQuery.exec()) {
                                result.warnings = warnings;
                                result.code = WriteErrorCode::WriteFailed;
                                result.message = QStringLiteral(
                                    "update failed: %1")
                                                     .arg(writeQuery.lastError().text());
                                qWarning("MigrationWriter: note_id=%lld folder_id=%lld "
                                         "format=%s write failed",
                                         static_cast<long long>(noteId),
                                         static_cast<long long>(folderId),
                                         qPrintable(LegacyFormatDetector::formatName(format)));
                            } else if (writeQuery.numRowsAffected() != 1) {
                                result.warnings = warnings;
                                result.code = WriteErrorCode::WriteFailed;
                                result.message = QStringLiteral(
                                    "update affected %1 row(s)")
                                                     .arg(writeQuery.numRowsAffected());
                                qWarning("MigrationWriter: note_id=%lld folder_id=%lld "
                                         "format=%s unexpected rows affected",
                                         static_cast<long long>(noteId),
                                         static_cast<long long>(folderId),
                                         qPrintable(LegacyFormatDetector::formatName(format)));
                            } else {
                                result.warnings = warnings;
                                result.success = true;
                                result.code = WriteErrorCode::None;
                                result.originalDataPreserved = false;
                                result.message = QStringLiteral("migrated");
                                qInfo("MigrationWriter: note_id=%lld folder_id=%lld "
                                      "format=%s migrated",
                                      static_cast<long long>(noteId),
                                      static_cast<long long>(folderId),
                                      qPrintable(LegacyFormatDetector::formatName(format)));
                            }
                        }
                    }
                }
            }
        }
        db.close();
    }
    // 连接已离开作用域，安全移除（避免 "connection still in use" 警告）。
    QSqlDatabase::removeDatabase(connName);
    return result;
}
