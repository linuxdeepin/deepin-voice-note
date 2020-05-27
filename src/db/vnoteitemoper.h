#ifndef VNOTEITEMOPER_H
#define VNOTEITEMOPER_H

#include "common/datatypedef.h"

class VNoteItemOper
{
public:
    VNoteItemOper(VNoteItem* note = nullptr);

    VNOTE_ALL_NOTES_MAP* loadAllVNotes();

    bool modifyNoteTitle(QString title);
    bool updateNote();

    VNoteItem* addNote(VNoteItem& note);
    VNoteItem* getNote(qint64 folderId, qint32 noteId);
    VNOTE_ITEMS_MAP *getFolderNotes(qint64 folderId);
    QString getDefaultNoteName(qint64 folderId);
    QString getDefaultVoiceName() const;

    bool deleteNote();

protected:
    VNoteItem* m_note {nullptr};
};

#endif // VNOTEITEMOPER_H
