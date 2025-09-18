// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnoteforlder.h"
#include "common/vnotedatamanager.h"
#include "db/vnoteitemoper.h"
#include <QDebug>

// #include <DLog>

/**
 * @brief VNoteFolder::VNoteFolder
 */
VNoteFolder::VNoteFolder()
{
    qInfo() << "VNoteFolder constructor called";
}

/**
 * @brief VNoteFolder::~VNoteFolder
 */
VNoteFolder::~VNoteFolder()
{
    // qInfo() << "VNoteFolder destructor called";
}

/**
 * @brief VNoteFolder::isValid
 * @return true 可用
 */
bool VNoteFolder::isValid()
{
    // qInfo() << "Checking if folder is valid, id:" << id;
    return (id > INVALID_ID) ? true : false;
}

/**
 * @brief VNoteFolder::maxNoteIdRef
 * @return 最大记事项id
 */
qint32 &VNoteFolder::maxNoteIdRef()
{
    // qInfo() << "Getting max note ID reference";
    return maxNoteId;
}

/**
 * @brief VNoteFolder::getNotesCount
 * @return 记事项数目
 */
qint32 VNoteFolder::getNotesCount()
{
    // qInfo() << "Getting notes count";
    int nCount = 0;

    VNOTE_ITEMS_MAP *folderNotes = getNotes();

    if (Q_LIKELY(nullptr != folderNotes)) {
        qInfo() << "folderNotes is not nullptr";
        folderNotes->lock.lockForRead();
        nCount = folderNotes->folderNotes.count();
        folderNotes->lock.unlock();
    }

    qInfo() << "Notes count retrieval finished, count:" << nCount;
    return nCount;
}

/**
 * @brief VNoteFolder::getNotes
 * @return 记事项数据
 */
VNOTE_ITEMS_MAP *VNoteFolder::getNotes()
{
    qInfo() << "Getting notes";
    if (Q_UNLIKELY(nullptr == notes)) {
        qInfo() << "notes is nullptr";
        VNOTE_ITEMS_MAP *folderNotes = VNoteDataManager::instance()->getFolderNotes(id);
        notes = folderNotes;
    }

    qInfo() << "Notes retrieval finished";
    return notes;
}
