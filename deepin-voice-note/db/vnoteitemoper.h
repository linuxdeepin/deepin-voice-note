#ifndef VNOTEITEMOPER_H
#define VNOTEITEMOPER_H

#include "common/datatypedef.h"

class DbVisitor;
class VNoteFolderOper;

class VNoteItemOper
{
public:
    VNoteItemOper(VNoteItem* note = nullptr);

    VNOTE_ALL_NOTES_MAP* loadAllVNotes();

    bool modifyNoteText(QString text);

    VNoteItem* addNote(VNoteItem& note);
    VNoteItem* getNote(qint64 folderId, qint32 noteId);
    VNOTE_ITEMS_MAP *getFolderNotes(qint64 folderId);

    bool deleteNote();
    bool deleteNote(qint64 folderId, qint32 noteId);

protected:
    VNoteItem* m_note {nullptr};

    static const QStringList noteColumnsName;

    enum {
        note_id = 0,
        folder_id,
        note_type,
        note_title,
        meta_data,
        note_state,
        create_time,
        modify_time,
        delete_time,
    };

    friend class NoteQryDbVisitor;
    friend class AddNoteDbVisitor;
    friend class VNoteFolderOper;   //Need folder table feilds name
};

#endif // VNOTEITEMOPER_H
