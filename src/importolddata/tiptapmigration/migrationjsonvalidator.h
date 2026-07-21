// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONJSONVALIDATOR_H
#define MIGRATIONJSONVALIDATOR_H

#include <QJsonObject>
#include <QString>
#include <QVector>

struct MigrationJsonValidationError {
    QString path;
    QString code;
    QString message;
};

struct MigrationJsonValidationResult {
    QVector<MigrationJsonValidationError> errors;

    bool ok() const;
};

class MigrationJsonValidator
{
public:
    static MigrationJsonValidationResult validateEnvelope(const QJsonObject &envelope);
};

#endif // MIGRATIONJSONVALIDATOR_H
