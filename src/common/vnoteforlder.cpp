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

/**
 * @brief VNoteFolder::VNoteFolder
 */
VNoteFolder::VNoteFolder()
{
}

/**
 * @brief VNoteFolder::~VNoteFolder
 */
VNoteFolder::~VNoteFolder()
{
}

/**
 * @brief VNoteFolder::isValid
 * @return true 可用
 */
bool VNoteFolder::isValid()
{
    return (id > INVALID_ID) ? true : false;
}

/**
 * @brief VNoteFolder::maxNoteIdRef
 * @return 最大记事项id
 */
qint32 &VNoteFolder::maxNoteIdRef()
{
    return maxNoteId;
}

/**
 * @brief VNoteFolder::getNotesCount
 * @return 记事项数目
 */
qint32 VNoteFolder::getNotesCount()
{
    int nCount = 0;

    VNOTE_ITEMS_MAP *folderNotes = getNotes();

    if (Q_LIKELY(nullptr != folderNotes)) {
        folderNotes->lock.lockForRead();
        nCount = folderNotes->folderNotes.count();
        folderNotes->lock.unlock();
    }

    return nCount;
}

/**
 * @brief VNoteFolder::getNotes
 * @return 记事项数据
 */
VNOTE_ITEMS_MAP *VNoteFolder::getNotes()
{
    if (Q_UNLIKELY(nullptr == notes)) {
        VNOTE_ITEMS_MAP *folderNotes = VNoteDataManager::instance()->getFolderNotes(id);
        notes = folderNotes;
    }

    return notes;
}
