/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
