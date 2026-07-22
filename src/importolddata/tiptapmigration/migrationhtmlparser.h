// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONHTMLPARSER_H
#define MIGRATIONHTMLPARSER_H

#include <QMap>
#include <QString>
#include <QVector>

enum class MigrationHtmlNodeType {
    Document,
    Element,
    Text
};

struct MigrationHtmlAttribute {
    QString name;
    QString value;
};

struct MigrationHtmlNode {
    MigrationHtmlNodeType type = MigrationHtmlNodeType::Document;
    QString tagName;
    QString text;
    QMap<QString, QString> attributes;
    QVector<MigrationHtmlNode> children;
};

// Dangerous nodes are kept in the parsed tree for converter-level downgrade
// decisions, while fallback plain text deliberately skips their content.
struct MigrationHtmlParseWarning {
    QString path;
    QString code;
    QString message;
};

struct MigrationHtmlParseResult {
    MigrationHtmlNode root;
    QString plainText;
    QVector<MigrationHtmlParseWarning> warnings;

    bool ok() const;
};

class MigrationHtmlParser
{
public:
    static MigrationHtmlParseResult parse(const QString &html);
    static QString attribute(const MigrationHtmlNode &node, const QString &name);
    static bool hasClass(const MigrationHtmlNode &node, const QString &className);
};

#endif // MIGRATIONHTMLPARSER_H
