// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "legacyformatdetector.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

LegacyFormat LegacyFormatDetector::detect(const QString &payload)
{
    const QString trimmed = payload.trimmed();
    if (trimmed.isEmpty()) {
        return LegacyFormat::PlainText;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError) {
        if (!document.isObject()) {
            return LegacyFormat::Invalid;
        }

        const QJsonObject root = document.object();
        if (root.value(QStringLiteral("format")).toString() == QStringLiteral("tiptap")) {
            return LegacyFormat::TiptapEnvelope;
        }

        if (root.value(QStringLiteral("type")).toString() == QStringLiteral("doc")) {
            return LegacyFormat::ProseMirrorDoc;
        }

        if (root.value(QStringLiteral("noteDatas")).isArray()) {
            return LegacyFormat::LegacyNoteDatas;
        }

        if (root.value(QStringLiteral("htmlCode")).isString()) {
            return LegacyFormat::LegacyHtmlCode;
        }

        return LegacyFormat::Invalid;
    }

    if (looksLikeJson(trimmed)) {
        return LegacyFormat::Invalid;
    }

    if (looksLikeHtml(trimmed)) {
        return LegacyFormat::LegacyHtmlCode;
    }

    return LegacyFormat::PlainText;
}

QString LegacyFormatDetector::formatName(LegacyFormat format)
{
    switch (format) {
    case LegacyFormat::TiptapEnvelope:
        return QStringLiteral("TiptapEnvelope");
    case LegacyFormat::ProseMirrorDoc:
        return QStringLiteral("ProseMirrorDoc");
    case LegacyFormat::LegacyHtmlCode:
        return QStringLiteral("LegacyHtmlCode");
    case LegacyFormat::LegacyNoteDatas:
        return QStringLiteral("LegacyNoteDatas");
    case LegacyFormat::PlainText:
        return QStringLiteral("PlainText");
    case LegacyFormat::Invalid:
        return QStringLiteral("Invalid");
    }

    return QStringLiteral("Invalid");
}

bool LegacyFormatDetector::looksLikeJson(const QString &payload)
{
    if (payload.isEmpty()) {
        return false;
    }

    const QChar first = payload.front();
    return first == QLatin1Char('{') || first == QLatin1Char('[');
}

bool LegacyFormatDetector::looksLikeHtml(const QString &payload)
{
    static const QRegularExpression htmlTagPattern(
        QStringLiteral(R"(<\s*/?\s*(p|div|br|span|h[1-6]|ul|ol|li|blockquote|img|table|strong|b|em|i|u|s)\b)"),
        QRegularExpression::CaseInsensitiveOption);

    return payload.contains(QStringLiteral("voiceBox"))
        || payload.contains(QStringLiteral("jsonKey"))
        || htmlTagPattern.match(payload).hasMatch();
}
