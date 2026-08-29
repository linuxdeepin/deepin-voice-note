// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEARCHINDEXMANAGER_H
#define SEARCHINDEXMANAGER_H

#include "searchdocument.h"

#include "datatypedef.h"

#include <QHash>
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
    struct SearchIndexEntry {
        SearchDocument document;
        QString fingerprint;
        QString normalizedTitle;
        QString normalizedCorpus;
        QSet<QString> grams;
    };

    static QString fingerprintForNote(const VNoteItem *note);
    static QString corpusForDocument(const SearchDocument &document);
    static QSet<QString> gramsForText(const QString &text);
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
    QHash<QString, QSet<int>> m_ngramIndex;
};

#endif // SEARCHINDEXMANAGER_H
