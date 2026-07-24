// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONHTMLCODES_H
#define MIGRATIONHTMLCODES_H

#include <QSet>
#include <QString>

// Single source of truth for HTML migration path warning codes, shared tag
// sets and the unsafe URL scheme predicate. Consumed by MigrationHtmlParser and
// MigrationHtmlConverter only (D8=A: noteDatas/validator codes stay local).

// --- Segment A: warning code vocabulary ---

inline const QString kCodeDowngradedHtmlBlock = QStringLiteral("downgraded-html-block");
inline const QString kCodeDowngradedHtmlListChild = QStringLiteral("downgraded-html-list-child");
inline const QString kCodeDowngradedInlineVoicebox = QStringLiteral("downgraded-inline-voicebox");
inline const QString kCodeDowngradedOrphanListItem = QStringLiteral("downgraded-orphan-list-item");
inline const QString kCodeDowngradedBase64Image = QStringLiteral("downgraded-base64-image");
inline const QString kCodeMissingHtmlImageSrc = QStringLiteral("missing-html-image-src");
inline const QString kCodeUnsafeHtmlImageSrc = QStringLiteral("unsafe-html-image-src");
inline const QString kCodeUnsupportedHtmlStyle = QStringLiteral("unsupported-html-style");
inline const QString kCodeInvalidHtmlStyleValue = QStringLiteral("invalid-html-style-value");
inline const QString kCodeDowngradedTextAlign = QStringLiteral("downgraded-text-align");

inline const QString kCodeMissingVoiceboxJsonKey = QStringLiteral("missing-voicebox-jsonkey");
inline const QString kCodeInvalidVoiceboxJsonKey = QStringLiteral("invalid-voicebox-jsonkey");
inline const QString kCodeInvalidVoiceboxStringField = QStringLiteral("invalid-voicebox-string-field");
inline const QString kCodeInvalidVoiceSize = QStringLiteral("invalid-voice-size");
inline const QString kCodeMissingVoiceSize = QStringLiteral("missing-voice-size");
inline const QString kCodeInvalidVoicePath = QStringLiteral("invalid-voice-path");

inline const QString kCodeDangerousHtmlNode = QStringLiteral("dangerous-html-node");
inline const QString kCodeDepthExceeded = QStringLiteral("depth-exceeded");
inline const QString kCodeParseFailed = QStringLiteral("parse-failed");

// New downgrade codes (TTP-013).
inline const QString kCodeDowngradedInlineElement = QStringLiteral("downgraded-inline-element");
inline const QString kCodeDangerousHtmlAttribute = QStringLiteral("dangerous-html-attribute");
inline const QString kCodeDowngradedHtmlLink = QStringLiteral("downgraded-html-link");

// --- Segment B: shared tag sets ---

inline const QSet<QString> &dangerousTags()
{
    static const QSet<QString> tags {
        QStringLiteral("script"),
        QStringLiteral("iframe"),
        QStringLiteral("object"),
        QStringLiteral("embed")
    };
    return tags;
}

inline const QSet<QString> &blockTags()
{
    static const QSet<QString> tags {
        QStringLiteral("address"),
        QStringLiteral("article"),
        QStringLiteral("aside"),
        QStringLiteral("blockquote"),
        QStringLiteral("dd"),
        QStringLiteral("div"),
        QStringLiteral("dl"),
        QStringLiteral("dt"),
        QStringLiteral("figcaption"),
        QStringLiteral("figure"),
        QStringLiteral("footer"),
        QStringLiteral("h1"),
        QStringLiteral("h2"),
        QStringLiteral("h3"),
        QStringLiteral("h4"),
        QStringLiteral("h5"),
        QStringLiteral("h6"),
        QStringLiteral("header"),
        QStringLiteral("hr"),
        QStringLiteral("li"),
        QStringLiteral("main"),
        QStringLiteral("ol"),
        QStringLiteral("p"),
        QStringLiteral("pre"),
        QStringLiteral("section"),
        QStringLiteral("table"),
        QStringLiteral("td"),
        QStringLiteral("th"),
        QStringLiteral("tr"),
        QStringLiteral("ul")
    };
    return tags;
}

// --- Segment C: unsafe URL scheme predicate (self-contained, G2) ---

inline bool isUnsafeUrlScheme(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.startsWith(QStringLiteral("//")) || trimmed.startsWith(QStringLiteral("\\\\"))) {
        return true;
    }

    const int colonIndex = trimmed.indexOf(QLatin1Char(':'));
    if (colonIndex <= 0 || !trimmed.at(0).isLetter()) {
        return false;
    }

    for (int index = 1; index < colonIndex; ++index) {
        const QChar character = trimmed.at(index);
        if (!(character.isLetterOrNumber()
              || character == QLatin1Char('+')
              || character == QLatin1Char('.')
              || character == QLatin1Char('-'))) {
            return false;
        }
    }

    const QString scheme = trimmed.left(colonIndex).toLower();
    return scheme == QStringLiteral("javascript")
        || scheme == QStringLiteral("vbscript")
        || scheme == QStringLiteral("data");
}

#endif // MIGRATIONHTMLCODES_H
