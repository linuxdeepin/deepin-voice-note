// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationscanner.h"

#include "legacyformatdetector.h"  // LegacyFormatDetector::detect()
#include "db/dbvisitor.h"                           // DbVisitor::DBNote 列枚举/列名
#include "db/vnotedbmanager.h"                      // VNoteDbManager::NOTES_TABLE_NAME/DBVERSION
#include "globaldef.h"                              // DEEPIN_VOICE_NOTE

#include <QAtomicInt>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>


namespace {
// 扫描 SELECT 取列顺序（按 SELECT 列表的位置索引读取，避免依赖表全列序）。
constexpr int kColNoteId = 0;
constexpr int kColMetaData = 1;
constexpr int kColEncrypt = 2;
}  // namespace

MigrationScanner::MigrationScanner()
{
}

MigrationScanner::MigrationScanner(const QString &dbPath)
    : m_dbPath(dbPath)
{
}

void MigrationScanner::cancel()
{
    m_cancelRequested.store(true);
}

QString MigrationScanner::dbPath() const
{
    return m_dbPath;
}

QString MigrationScanner::defaultDbPath()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + QDir::separator() + DEEPIN_VOICE_NOTE
           + QString(VNoteDbManager::DBVERSION) + QStringLiteral(".db");
}

QString MigrationScanner::effectiveDbPath() const
{
    return m_dbPath.isEmpty() ? defaultDbPath() : m_dbPath;
}

QString MigrationScanner::makeConnectionName() const
{
    // 进程级递增计数，保证多次扫描/测试场景连接名唯一，用后 removeDatabase 释放。
    static QAtomicInt counter(0);
    return QStringLiteral("voice_note_migration_scan_%1")
        .arg(counter.fetchAndAddRelaxed(1));
}

void MigrationScanner::onRowProcessed(int processedCount)
{
    Q_UNUSED(processedCount)
}

ScanResult MigrationScanner::scan()
{
    ScanResult result;
    // D8：每次重新扫描独立，清除上轮残留取消标志。
    m_cancelRequested.store(false);

    const QString dbPath = effectiveDbPath();

    // 源库文件不存在直接失败，避免 SQLite 只读打开静默创建空库（写副作用防御）。
    if (!QFileInfo(dbPath).isFile()) {
        result.code = ScanErrorCode::DbOpenFailed;
        result.message = QStringLiteral("Database file not found: %1").arg(dbPath);
        qWarning("MigrationScanner: %s", qPrintable(result.message));
        return result;
    }

    // 独立只读连接，命名唯一，避免与 VNoteDbManager 运行态连接争用。
    const QString connName = makeConnectionName();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(dbPath);
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (!db.open()) {
            result.code = ScanErrorCode::DbOpenFailed;
            result.message = QStringLiteral("Failed to open database readonly: %1")
                                 .arg(db.lastError().text());
            qWarning("MigrationScanner: %s", qPrintable(result.message));
        } else {
            // 列名取自 DbVisitor::DBNote::noteColumnsName，与运行态查询同源，
            // 避免硬编码列名偏差（encrypt 实际落在 expand_filed2 列）。
            const QStringList &cols = DbVisitor::DBNote::noteColumnsName;
            const QString sql = QStringLiteral("SELECT %1, %2, %3 FROM %4 ORDER BY %1")
                                    .arg(cols.value(DbVisitor::DBNote::note_id),
                                         cols.value(DbVisitor::DBNote::meta_data),
                                         cols.value(DbVisitor::DBNote::encrypt),
                                         VNoteDbManager::NOTES_TABLE_NAME);

            QSqlQuery query(db);
            if (!query.exec(sql)) {
                result.code = ScanErrorCode::QueryFailed;
                result.message = QStringLiteral("Failed to execute scan query: %1")
                                     .arg(query.lastError().text());
                qWarning("MigrationScanner: %s", qPrintable(result.message));
            } else {
                while (query.next()) {
                    // 取消检查点：每行进入前判标志位，快速退出（D3 由 TTP-020 兜底）。
                    if (m_cancelRequested.load()) {
                        result.code = ScanErrorCode::Aborted;
                        result.message = QStringLiteral(
                                             "Scan aborted by cancel request after %1 row(s)")
                                             .arg(result.totalCount);
                        qInfo("MigrationScanner: %s", qPrintable(result.message));
                        break;
                    }

                    const qint32 noteId = query.value(kColNoteId).toInt();
                    const int encryption = query.value(kColEncrypt).toInt();
                    const QVariant metaDataVar = query.value(kColMetaData);

                    // 加密还原：复用 NoteQryDbVisitor 现有语义，base64 还原后再探测，
                    // 否则加密笔记会被误判为 PlainText/Invalid（R-scan-1）。
                    const QString metaDataStr = (encryption != 0)
                        ? QString::fromUtf8(QByteArray::fromBase64(metaDataVar.toByteArray()))
                        : metaDataVar.toString();

                    ++result.totalCount;
                    // 仅用 LegacyFormatDetector 归类（D5/D6），不调信封校验（属 TTP-018）。
                    const LegacyFormat format = LegacyFormatDetector::detect(metaDataStr);
                    switch (format) {
                    case LegacyFormat::TiptapEnvelope:
                        ++result.alreadyTiptapCount;
                        break;
                    case LegacyFormat::Invalid:
                        ++result.abnormalCount;
                        result.abnormals.append(
                            ScanAbnormal{noteId,
                                         QStringLiteral("%1 (unrecognized meta_data)")
                                             .arg(LegacyFormatDetector::formatName(format))});
                        break;
                    default:
                        // ProseMirrorDoc / LegacyHtmlCode / LegacyNoteDatas / PlainText → 需迁移
                        ++result.needMigrateCount;
                        result.needMigrateNoteIds.append(noteId);
                        break;
                    }

                    onRowProcessed(result.totalCount);
                }

                if (result.code != ScanErrorCode::Aborted) {
                    result.success = true;
                    result.code = ScanErrorCode::None;
                    result.message = QStringLiteral(
                                         "Scan completed: total=%1 needMigrate=%2 "
                                         "alreadyTiptap=%3 abnormal=%4")
                                         .arg(result.totalCount)
                                         .arg(result.needMigrateCount)
                                         .arg(result.alreadyTiptapCount)
                                         .arg(result.abnormalCount);
                    qInfo("MigrationScanner: %s", qPrintable(result.message));
                }
            }
        }
        db.close();
    }
    // 连接已离开作用域，安全移除（避免 "connection still in use" 警告）。
    QSqlDatabase::removeDatabase(connName);
    return result;
}
