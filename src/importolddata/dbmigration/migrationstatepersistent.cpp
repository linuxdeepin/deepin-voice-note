// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migrationstatepersistent.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstdio>      // std::rename
#include <errno.h>
#include <string.h>

namespace {

const char *kTmpSuffix = ".tmp";

QJsonObject historyEntryToJson(const HistoryEntry &entry)
{
    QJsonObject obj;
    obj["from"] = migrationStateToString(entry.from);
    obj["to"] = migrationStateToString(entry.to);
    obj["substage"] = migrationSubstageToString(entry.substage);
    obj["reason"] = entry.reason;
    obj["timestamp"] = entry.timestamp;
    obj["cursor"] = entry.cursor;
    return obj;
}

HistoryEntry historyEntryFromJson(const QJsonObject &obj)
{
    HistoryEntry entry;
    entry.from = migrationStateFromString(obj.value("from").toString());
    entry.to = migrationStateFromString(obj.value("to").toString());
    entry.substage = migrationSubstageFromString(obj.value("substage").toString());
    entry.reason = obj.value("reason").toString();
    entry.timestamp = obj.value("timestamp").toString();
    entry.cursor = obj.value("cursor").toObject();
    return entry;
}

} // namespace

MigrationStatePersistent::MigrationStatePersistent(const QString &filePath)
    : m_filePath(filePath)
{
}

bool MigrationStatePersistent::save(MigrationState state, MigrationSubstage substage,
                                    const QJsonObject &cursor, bool cancelled,
                                    const QString &updatedAt,
                                    const QVector<HistoryEntry> &history)
{
    QJsonObject root;
    root["state"] = migrationStateToString(state);
    root["substage"] = migrationSubstageToString(substage);
    root["cursor"] = cursor;
    root["cancelled"] = cancelled;
    root["updatedAt"] = updatedAt;

    QJsonArray historyArray;
    for (const HistoryEntry &entry : history) {
        historyArray.append(historyEntryToJson(entry));
    }
    root["history"] = historyArray;

    const QJsonDocument doc(root);

    // 确保目录存在
    const QFileInfo info(m_filePath);
    const QDir dir = info.absoluteDir();
    if (!dir.exists() && !dir.mkpath(dir.absolutePath())) {
        qWarning("MigrationStatePersistent: mkpath failed for %s",
                 qPrintable(dir.absolutePath()));
        return false;
    }

    // 原子写：写临时文件后 rename 覆盖目标（缓解 R2 游标与状态不一致）
    const QString tmpPath = m_filePath + QLatin1String(kTmpSuffix);
    {
        QFile tmp(tmpPath);
        if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning("MigrationStatePersistent: open tmp failed: %s",
                     qPrintable(tmp.errorString()));
            return false;
        }
        const QByteArray payload = doc.toJson(QJsonDocument::Compact);
        if (tmp.write(payload) != payload.size()) {
            qWarning("MigrationStatePersistent: write tmp failed: %s",
                     qPrintable(tmp.errorString()));
            tmp.close();
            return false;
        }
        tmp.flush();
        tmp.close();
    }

    // 原子覆盖目标：用 POSIX rename(2)（std::rename）替换已存在的目标文件。
    // 注意：Qt 的 QFile::rename 在目标已存在时返回 false 不覆盖，与 POSIX rename(2)
    // 的原子替换语义不同，故此处必须用 std::rename 以保证原子写（缓解 R2）。
    if (std::rename(QFile::encodeName(tmpPath).constData(),
                    QFile::encodeName(m_filePath).constData()) != 0) {
        qWarning("MigrationStatePersistent: rename failed: %s -> %s (%s)",
                 qPrintable(tmpPath), qPrintable(m_filePath),
                 qPrintable(QString::fromLocal8Bit(strerror(errno))));
        return false;
    }
    return true;
}

bool MigrationStatePersistent::load(MigrationState &state, MigrationSubstage &substage,
                                    QJsonObject &cursor, bool &cancelled,
                                    QString &updatedAt, QVector<HistoryEntry> &history)
{
    // 回退初始态的 lambda
    auto resetInitial = [&]() {
        state = MigrationState::Pending;
        substage = MigrationSubstage::None;
        cursor = QJsonObject();
        cancelled = false;
        updatedAt = QString();
        history.clear();
    };

    QFile file(m_filePath);
    if (!file.exists()) {
        // 文件缺失视为初始态（合法）
        resetInitial();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("MigrationStatePersistent: open failed: %s",
                 qPrintable(file.errorString()));
        resetInitial();
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning("MigrationStatePersistent: parse failed: %s",
                 qPrintable(parseError.errorString()));
        resetInitial();
        return false;
    }
    const QJsonObject root = doc.object();

    const QString stateName = root.value("state").toString();
    const MigrationState parsedState = migrationStateFromString(stateName);
    // 回检：未知状态字符串经转换后会与原名不一致，视为损坏回退初始态
    if (stateName.isEmpty() || migrationStateToString(parsedState) != stateName) {
        qWarning("MigrationStatePersistent: unknown state %s",
                 qPrintable(stateName));
        resetInitial();
        return false;
    }

    state = parsedState;
    const QString substageName = root.value("substage").toString();
    const MigrationSubstage parsedSubstage = migrationSubstageFromString(substageName);
    if (!substageName.isEmpty() && migrationSubstageToString(parsedSubstage) != substageName) {
        qWarning("MigrationStatePersistent: unknown substage %s",
                 qPrintable(substageName));
        resetInitial();
        return false;
    }
    substage = parsedSubstage;
    cursor = root.value("cursor").toObject();
    cancelled = root.value("cancelled").toBool(false);
    updatedAt = root.value("updatedAt").toString();

    history.clear();
    const QJsonArray historyArray = root.value("history").toArray();
    for (const QJsonValue &value : historyArray) {
        if (value.isObject()) {
            history.append(historyEntryFromJson(value.toObject()));
        }
    }
    return true;
}

QString MigrationStatePersistent::filePath() const
{
    return m_filePath;
}
