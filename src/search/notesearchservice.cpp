// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notesearchservice.h"

NoteSearchService::NoteSearchService(QObject *parent)
    : QObject(parent)
{
}

NoteSearchService *NoteSearchService::instance()
{
    static NoteSearchService service;
    return &service;
}

QList<SearchResult> NoteSearchService::search(VNOTE_ALL_NOTES_MAP *noteAll, const QString &query)
{
    refreshIndex(noteAll);
    return m_index.search(query);
}

void NoteSearchService::refreshIndex(VNOTE_ALL_NOTES_MAP *noteAll)
{
    m_index.refreshFromNotes(noteAll);
}

void NoteSearchService::updateNote(const VNoteItem *note)
{
    m_index.updateNote(note);
}

void NoteSearchService::removeNote(int noteId)
{
    m_index.removeNote(noteId);
}

void NoteSearchService::clearIndex()
{
    m_index.clear();
}

int NoteSearchService::indexedNoteCount() const
{
    return m_index.size();
}
