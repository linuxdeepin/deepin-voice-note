// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONNOTEDATACONVERTER_H
#define MIGRATIONNOTEDATACONVERTER_H

#include <QJsonObject>
#include <QString>
#include <QVector>

struct MigrationNoteDataConversionIssue {
    QString path;
    QString code;
    QString message;
};

struct MigrationNoteDataConversionResult {
    QJsonObject envelope;
    QVector<MigrationNoteDataConversionIssue> errors;
    QVector<MigrationNoteDataConversionIssue> warnings;

    bool ok() const;
};

class MigrationNoteDataConverter
{
public:
    static MigrationNoteDataConversionResult convertTextBlocks(const QString &metadataJson);
    static MigrationNoteDataConversionResult convertTextBlocks(const QJsonObject &metadata);
};

#endif // MIGRATIONNOTEDATACONVERTER_H
