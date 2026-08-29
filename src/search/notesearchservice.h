// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NOTESEARCHSERVICE_H
#define NOTESEARCHSERVICE_H

#include "searchdocument.h"
#include "searchindexmanager.h"

#include <QObject>

struct VNoteItem;

class NoteSearchService : public QObject
{
    Q_OBJECT
public:
    static NoteSearchService *instance();

    QList<SearchResult> search(VNOTE_ALL_NOTES_MAP *noteAll, const QString &query);
    void refreshIndex(VNOTE_ALL_NOTES_MAP *noteAll);
    void updateNote(const VNoteItem *note);
    void removeNote(int noteId);
    void clearIndex();
    int indexedNoteCount() const;

private:
    explicit NoteSearchService(QObject *parent = nullptr);

    SearchIndexManager m_index;
};

#endif // NOTESEARCHSERVICE_H
