// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONHTMLCONVERTER_H
#define MIGRATIONHTMLCONVERTER_H

#include <QJsonObject>
#include <QString>
#include <QVector>

struct MigrationHtmlConversionIssue {
    QString path;
    QString code;
    QString message;
};

struct MigrationHtmlConversionResult {
    QJsonObject envelope;
    QVector<MigrationHtmlConversionIssue> errors;
    QVector<MigrationHtmlConversionIssue> warnings;

    bool ok() const;
};

class MigrationHtmlConverter
{
public:
    static MigrationHtmlConversionResult convert(const QString &htmlCode);
};

#endif // MIGRATIONHTMLCONVERTER_H
