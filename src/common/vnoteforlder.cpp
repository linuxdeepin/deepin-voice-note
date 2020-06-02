/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
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
#include "vnoteforlder.h"
#include "common/vnotedatamanager.h"

#include <DLog>

VNoteFolder::VNoteFolder()
{
}

VNoteFolder::~VNoteFolder()
{
}

bool VNoteFolder::isValid()
{
    return (id > INVALID_ID) ? true : false;
}

qint32 &VNoteFolder::maxNoteIdRef()
{
    return maxNoteId;
}

qint32 VNoteFolder::getNotesCount()
{
    int nCount =  0;

    VNOTE_ITEMS_MAP * folderNotes = getNotes();

    if (Q_LIKELY(nullptr != folderNotes)) {
        folderNotes->lock.lockForRead();
        nCount = folderNotes->folderNotes.count();
        folderNotes->lock.unlock();
    }

    return nCount;
}

VNOTE_ITEMS_MAP * VNoteFolder::getNotes()
{
    if (Q_UNLIKELY(nullptr == notes)) {
        VNOTE_ITEMS_MAP * folderNotes =VNoteDataManager::instance()->getFolderNotes(id);
        notes = folderNotes;
    }

    return notes;
}

QDebug & operator <<(QDebug &out, VNoteFolder &folder)
{
    out << "\n{ "
        << "id=" << folder.id << ","
        << "name=" << folder.name << ","
        << "defaultIcon=" << folder.defaultIcon << ","
        << "iconPath=" << folder.iconPath << ","
        << "notesCount=" << folder.notesCount << ","
        << "folder_state=" << folder.folder_state << ","
        << "maxNoteId=" << folder.maxNoteId << ","
        << "createTime=" << folder.createTime << ","
        << "modifyTime=" << folder.modifyTime << ","
        << "deleteTime=" << folder.deleteTime
        << " }\n";

    return out;
}
