// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEARCHDOCUMENT_H
#define SEARCHDOCUMENT_H

#include <QDateTime>
#include <QString>
#include <QVector>

enum class SearchField {
    Title,
    Body,
    VoiceTitle,
    VoiceTranscript,
    ImageAlt,
};

struct SearchSegment {
    SearchField field {SearchField::Body};
    QString text;
    QString locatorType;
    QString locator;
};

struct SearchDocument {
    int noteId {-1};
    int folderId {-1};
    bool isTop {false};
    QDateTime modifyTime;
    QString titlePlain;
    QVector<SearchSegment> segments;
};

struct SearchMatch {
    SearchField field {SearchField::Body};
    QString text;
    QString locatorType;
    QString locator;
    int position {-1};
};

struct SearchResult {
    int noteId {-1};
    int folderId {-1};
    bool isTop {false};
    QDateTime modifyTime;
    QString titlePlain;
    QString highlightedTitle;
    QString snippet;
    SearchField bestField {SearchField::Body};
    int score {0};
    QVector<SearchMatch> matches;
};

QString searchFieldName(SearchField field);

#endif // SEARCHDOCUMENT_H
