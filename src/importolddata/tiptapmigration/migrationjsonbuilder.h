// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONJSONBUILDER_H
#define MIGRATIONJSONBUILDER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class MigrationJsonBuilder
{
public:
    static QJsonObject makeEnvelope();
    static QJsonObject makeEnvelope(const QJsonObject &content);
    static QJsonObject makeDoc(const QJsonArray &content = QJsonArray());
    static QJsonObject makeParagraph(const QJsonArray &content = QJsonArray());
    static QJsonObject makeText(const QString &text, const QJsonArray &marks = QJsonArray());
    static QJsonObject makeHardBreak();
    static QJsonObject makeHeading(int level, const QJsonArray &content = QJsonArray());
    static QJsonObject makeBlockquote(const QJsonArray &content = QJsonArray());
    static QJsonObject makeMark(const QString &type, const QJsonObject &attrs = QJsonObject());
    static QJsonObject makeImage(const QString &src,
                                 const QString &relPath = QString(),
                                 const QString &alt = QString(),
                                 const QString &title = QString());
    static QJsonObject makeVoiceBlock(const QString &voiceId,
                                      const QString &voicePath,
                                      qint64 voiceSize,
                                      const QString &createTime = QString(),
                                      const QString &title = QString(),
                                      const QString &text = QString(),
                                      bool translateUnfold = true);
    static QJsonObject makeBulletList(const QJsonArray &content = QJsonArray());
    static QJsonObject makeOrderedList(const QJsonArray &content = QJsonArray());
    static QJsonObject makeListItem(const QJsonArray &content = QJsonArray());
    static QJsonObject makeTaskList(const QJsonArray &content = QJsonArray());
    static QJsonObject makeTaskItem(bool checked, const QJsonArray &content = QJsonArray());
    static QString toCompactJson(const QJsonObject &object);

private:
    static QJsonValue nullableString(const QString &value);
    static QJsonArray normalizedDocContent(const QJsonArray &content);
    static QJsonArray normalizedListItemContent(const QJsonArray &content);
    static QJsonObject makeContainerNode(const QString &type, const QJsonArray &content);
};

#endif // MIGRATIONJSONBUILDER_H
