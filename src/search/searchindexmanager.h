// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEARCHINDEXMANAGER_H
#define SEARCHINDEXMANAGER_H

#include "searchdocument.h"

#include "datatypedef.h"

#include <QHash>
#include <QMultiHash>
#include <QReadWriteLock>
#include <QSet>
#include <QStringList>

struct VNoteItem;

class SearchIndexManager
{
public:
    static constexpr int NGramSize = 3;

    void clear();
    void refreshFromNotes(VNOTE_ALL_NOTES_MAP *noteAll);
    void updateNote(const VNoteItem *note);
    void removeNote(int noteId);
    QList<SearchResult> search(const QString &query) const;
    int size() const;

private:
    // 避免单个超长笔记把常驻 n-gram 索引放大到不可控规模。
    static constexpr int MaxGramIndexedTextLength = 1000000;

    struct SearchIndexEntry {
        SearchDocument document;
        quint64 fingerprint {0};
        QString normalizedTitle;
        QString normalizedCorpus;
        bool gramIndexed {false};
    };

    static quint64 fingerprintForNote(const VNoteItem *note);
    static QString corpusForDocument(const SearchDocument &document);
    static QSet<quint64> gramsForText(const QString &text);
    static int fieldWeight(SearchField field);
    static QString highlightedHtml(const QString &text, const QString &query);
    static QString snippetHtml(const QString &text, const QString &query);
    static SearchResult makeResult(const SearchIndexEntry &entry,
                                   const QString &normalizedQuery,
                                   const QString &rawQuery);

    void upsertEntryUnlocked(SearchIndexEntry &&entry);
    void removeEntryUnlocked(int noteId);

    mutable QReadWriteLock m_lock;
    QHash<int, SearchIndexEntry> m_entries;
    QMultiHash<quint64, int> m_ngramIndex;
};

#endif // SEARCHINDEXMANAGER_H
