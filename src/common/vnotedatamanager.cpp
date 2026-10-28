// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnotedatamanager.h"
#include "ddlog.h"
#include "db/vnotedbmanager.h"
#include "task/loadfolderworker.h"
#include "task/loadnoteitemsworker.h"
#include "task/loadiconsworker.h"
#include "vnoteforlder.h"
#include "vnoteitem.h"

#include <QThreadPool>
#include <QDebug>

// DCORE_USE_NAMESPACE

VNoteDataManager *VNoteDataManager::_instance = nullptr;

/**
 * @brief VNoteDataManager::VNoteDataManager
 * @param parent
 */
VNoteDataManager::VNoteDataManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(voiceNoteLog) << "Initializing VNoteDataManager";
}

/**
 * @brief VNoteDataManager::instance
 * @return 单例对象
 */
VNoteDataManager *VNoteDataManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteDataManager();

        //Init dabase when create data manager
        //Bcs data manager depends on db
        VNoteDbManager::instance();
    }

    return _instance;
}

/**
 * @brief VNoteDataManager::getNoteFolders
 * @return 记事本数据
 */
VNOTE_FOLDERS_MAP *VNoteDataManager::getNoteFolders()
{
    return m_qspNoteFoldersMap.get();
}

/**
 * @brief VNoteDataManager::addFolder
 * @param folder
 * @return 处理后完整的数据
 */
VNoteFolder *VNoteDataManager::addFolder(VNoteFolder *folder)
{
    qCDebug(voiceNoteLog) << "Adding folder:" << (folder ? folder->id : -1);
    VNoteFolder *retFlder = nullptr;

    if (nullptr != folder) {
        m_qspNoteFoldersMap->lock.lockForWrite();

        VNOTE_FOLDERS_DATA_MAP::iterator it = m_qspNoteFoldersMap->folders.find(folder->id);

        if (it == m_qspNoteFoldersMap->folders.end()) {
            m_qspNoteFoldersMap->folders.insert(folder->id, folder);
            qCDebug(voiceNoteLog) << "New folder added with ID:" << folder->id;
        } else {
            qCDebug(voiceNoteLog) << "Replacing existing folder with ID:" << folder->id;
            QScopedPointer<VNoteFolder> release(*it);

            m_qspNoteFoldersMap->folders.remove(folder->id);
            m_qspNoteFoldersMap->folders.insert(folder->id, folder);
        }

        m_qspNoteFoldersMap->lock.unlock();

        retFlder = folder;
    } else {
        qCWarning(voiceNoteLog) << "Attempted to add null folder";
    }

    return retFlder;
}

/**
 * @brief VNoteDataManager::getFolder
 * @param folderId
 * @return  记事本数据
 */
VNoteFolder *VNoteDataManager::getFolder(qint64 folderId)
{
    qCDebug(voiceNoteLog) << "Getting folder with ID:" << folderId;
    VNoteFolder *retFlder = nullptr;

    m_qspNoteFoldersMap->lock.lockForRead();

    VNOTE_FOLDERS_DATA_MAP::iterator it = m_qspNoteFoldersMap->folders.find(folderId);

    if (it != m_qspNoteFoldersMap->folders.end()) {
        retFlder = *it;
        qCDebug(voiceNoteLog) << "Folder found with ID:" << folderId;
    } else {
        qCDebug(voiceNoteLog) << "Folder not found with ID:" << folderId;
    }

    m_qspNoteFoldersMap->lock.unlock();

    return retFlder;
}

/**
 * @brief VNoteDataManager::delFolder
 * @param folderId
 * @return 移除的记事本数据
 */
VNoteFolder *VNoteDataManager::delFolder(qint64 folderId)
{
    qCDebug(voiceNoteLog) << "Deleting folder with ID:" << folderId;
    VNoteFolder *retFlder = nullptr;

    m_qspNoteFoldersMap->lock.lockForWrite();

    //Find the folder first. if can't find, may some goes to wrong
    VNOTE_FOLDERS_DATA_MAP::iterator itFolder = m_qspNoteFoldersMap->folders.find(folderId);

    if (itFolder != m_qspNoteFoldersMap->folders.end()) {
        //Get notes lock ,release all notes in the folder
        m_qspAllNotesMap->lock.lockForWrite();

        VNOTE_ALL_NOTES_DATA_MAP::iterator itNote = m_qspAllNotesMap->notes.find(folderId);

        if (itNote != m_qspAllNotesMap->notes.end()) {
            VNOTE_ITEMS_MAP *itemsMap = itNote.value();
            m_qspAllNotesMap->notes.erase(itNote);
            QScopedPointer<VNOTE_ITEMS_MAP> foldersMap(itemsMap);

            //Remove voice files in the folder
            qCDebug(voiceNoteLog) << "Removing voice files for folder:" << folderId;
            for (auto it : foldersMap->folderNotes) {
                it->delNoteData();
            }
        }

        //All note released. unlock
        m_qspAllNotesMap->lock.unlock();

        retFlder = *itFolder;
        m_qspNoteFoldersMap->folders.erase(itFolder);
        qCDebug(voiceNoteLog) << "Folder and associated notes deleted successfully";
    } else {
        qCWarning(voiceNoteLog) << "Folder not found for deletion:" << folderId;
    }

    m_qspNoteFoldersMap->lock.unlock();

    return retFlder;
}

/**
 * @brief VNoteDataManager::folderCount
 * @return 记事本数量
 */
qint32 VNoteDataManager::folderCount()
{
    m_qspNoteFoldersMap->lock.lockForRead();
    int count = m_qspNoteFoldersMap->folders.size();
    m_qspNoteFoldersMap->lock.unlock();
    qCDebug(voiceNoteLog) << "Current folder count:" << count;
    return count;
}

/**
 * @brief VNoteDataManager::addNote
 * @param note 记事项
 * @return 处理后完整的记事项数据
 */
VNoteItem *VNoteDataManager::addNote(VNoteItem *note)
{
    VNoteItem *retNote = nullptr;

    if (nullptr != note) {
        m_qspAllNotesMap->lock.lockForWrite();

        //Find the folder first. if can't find, may some goes to wrong
        VNOTE_ALL_NOTES_DATA_MAP::iterator it = m_qspAllNotesMap->notes.find(note->folderId);

        if (it != m_qspAllNotesMap->notes.end()) {
            VNOTE_ITEMS_MAP *notesInFolder = *it;

            Q_ASSERT(nullptr != notesInFolder);

            //TODO:
            //    Need lock the folder first,for muti-thread operation.
            //But new maybe not necessary
            notesInFolder->lock.lockForWrite();

            //Find if alreay exist same id note
            VNOTE_ITEMS_DATA_MAP::iterator noteIter = notesInFolder->folderNotes.find(note->noteId);

            if (noteIter == notesInFolder->folderNotes.end()) {
                notesInFolder->folderNotes.insert(note->noteId, note);
                qCDebug(voiceNoteLog) << "New note added with ID:" << note->noteId;
            } else {
                qCDebug(voiceNoteLog) << "Replacing existing note with ID:" << note->noteId;
                //Release old and insert new
                QScopedPointer<VNoteItem> release(*noteIter);

                notesInFolder->folderNotes.remove(note->noteId);
                notesInFolder->folderNotes.insert(note->noteId, note);
            }

            notesInFolder->lock.unlock();
        } else {
            qCInfo(voiceNoteLog) << "Creating new folder for note, folder ID:" << note->folderId;

            VNOTE_ITEMS_MAP *folderNotes = new VNOTE_ITEMS_MAP();

            //Set release flag true in data manager
            folderNotes->autoRelease = true;

            folderNotes->folderNotes.insert(note->noteId, note);
            m_qspAllNotesMap->notes.insert(note->folderId, folderNotes);
        }

        m_qspAllNotesMap->lock.unlock();

        retNote = note;
    } else {
        qCWarning(voiceNoteLog) << "Attempted to add null note";
    }

    return retNote;
}

/**
 * @brief VNoteDataManager::getNote
 * @param folderId
 * @param noteId
 * @return 记事项数据
 */
VNoteItem *VNoteDataManager::getNote(qint64 folderId, qint32 noteId)
{
    qCDebug(voiceNoteLog) << "Getting note from folder:" << folderId << "note ID:" << noteId;
    VNoteItem *retNote = nullptr;

    m_qspAllNotesMap->lock.lockForRead();

    //Find the folder first. if can't find, may some goes to wrong
    VNOTE_ALL_NOTES_DATA_MAP::iterator it = m_qspAllNotesMap->notes.find(folderId);

    if (it != m_qspAllNotesMap->notes.end()) {
        VNOTE_ITEMS_MAP *notesInFolder = *it;

        Q_ASSERT(nullptr != notesInFolder);

        //TODO:
        //    Need lock the folder first,for muti-thread operation.
        //But new maybe not necessary
        notesInFolder->lock.lockForRead();

        //Find the note
        VNOTE_ITEMS_DATA_MAP::iterator noteIter = notesInFolder->folderNotes.find(noteId);

        if (noteIter != notesInFolder->folderNotes.end()) {
            retNote = *noteIter;
            qCDebug(voiceNoteLog) << "Note found";
        } else {
            qCDebug(voiceNoteLog) << "Note not found";
        }

        notesInFolder->lock.unlock();
    } else {
        qCCritical(voiceNoteLog) << "Folder not found for note retrieval:" << folderId;
    }

    m_qspAllNotesMap->lock.unlock();

    return retNote;
}

/**
 * @brief VNoteDataManager::delNote
 * @param folderId
 * @param noteId
 * @return 移除的记事项数据
 */
VNoteItem *VNoteDataManager::delNote(qint64 folderId, qint32 noteId)
{
    qCDebug(voiceNoteLog) << "Deleting note from folder:" << folderId << "note ID:" << noteId;
    VNoteItem *retNote = nullptr;

    m_qspAllNotesMap->lock.lockForWrite();

    //Find the folder first. if can't find, may some goes to wrong
    VNOTE_ALL_NOTES_DATA_MAP::iterator it = m_qspAllNotesMap->notes.find(folderId);

    if (it != m_qspAllNotesMap->notes.end()) {
        VNOTE_ITEMS_MAP *notesInFolder = *it;

        Q_ASSERT(nullptr != notesInFolder);

        //TODO:
        //    Need lock the folder first,for muti-thread operation.
        //But new maybe not necessary
        notesInFolder->lock.lockForWrite();

        //Find the note
        VNOTE_ITEMS_DATA_MAP::iterator noteIter = notesInFolder->folderNotes.find(noteId);

        if (noteIter != notesInFolder->folderNotes.end()) {
            retNote = *noteIter;
            notesInFolder->folderNotes.erase(noteIter);
            //Remove voice file of voice note
            qCDebug(voiceNoteLog) << "Note deleted, removing associated data";
            retNote->delNoteData();
        } else {
            qCDebug(voiceNoteLog) << "Note not found for deletion";
        }

        notesInFolder->lock.unlock();
    } else {
        qCCritical(voiceNoteLog) << "Folder not found for note deletion:" << folderId;
    }

    m_qspAllNotesMap->lock.unlock();

    return retNote;
}

/**
 * @brief VNoteDataManager::folderNotesCount
 * @param folderId
 * @return 记事项数量
 */
qint32 VNoteDataManager::folderNotesCount(qint64 folderId)
{
    qCDebug(voiceNoteLog) << "Getting note count for folder:" << folderId;
    qint32 notesCount = 0;

    VNOTE_ITEMS_MAP *folderNotes = getFolderNotes(folderId);

    if (nullptr != folderNotes) {
        folderNotes->lock.lockForRead();
        notesCount = folderNotes->folderNotes.count();
        folderNotes->lock.unlock();
        qCDebug(voiceNoteLog) << "Folder contains" << notesCount << "notes";
    } else {
        qCDebug(voiceNoteLog) << "Folder not found for note count";
    }

    return notesCount;
}

/**
 * @brief VNoteDataManager::getFolderNotes
 * @param folderId
 * @return 记事本所有的记事项数据
 */
VNOTE_ITEMS_MAP *VNoteDataManager::getFolderNotes(qint64 folderId)
{
    qCDebug(voiceNoteLog) << "Getting notes for folder:" << folderId;
    VNOTE_ITEMS_MAP *folderNotes = nullptr;

    m_qspAllNotesMap->lock.lockForRead();

    //Find the folder first. if can't find, may some goes to wrong
    VNOTE_ALL_NOTES_DATA_MAP::iterator it = m_qspAllNotesMap->notes.find(folderId);

    if (it != m_qspAllNotesMap->notes.end()) {
        folderNotes = *it;
        qCDebug(voiceNoteLog) << "Found existing folder notes";
    } else {
        qCDebug(voiceNoteLog) << "Creating new folder notes map";
        folderNotes = new VNOTE_ITEMS_MAP();
        folderNotes->autoRelease = true;
        m_qspAllNotesMap->notes.insert(folderId, folderNotes);
    }

    m_qspAllNotesMap->lock.unlock();

    return folderNotes;
}

/**
 * @brief VNoteDataManager::getDefaultIcon
 * @param index
 * @param type
 * @return 图标
 */
QPixmap VNoteDataManager::getDefaultIcon(qint32 index, IconsType type)
{
    m_iconLock.lockForRead();
    if (index < 1 || index > m_defaultIcons[type].size()) {
        index = 0;
    }

    QPixmap icon = m_defaultIcons[type].at(index);

    m_iconLock.unlock();

    return icon;
}

/**
 * @brief VNoteDataManager::isAllDatasReady
 * @return true 所有数据加载完成
 */
bool VNoteDataManager::isAllDatasReady() const
{
    qCDebug(voiceNoteLog) << "Checking data ready state:" << m_fDataState;
    bool ready = ((m_fDataState & DataState::FolderDataReady)
                 && (m_fDataState & DataState::NotesDataReady));
    qCDebug(voiceNoteLog) << "Data ready state:" << ready;
    return ready;
}

/**
 * @brief VNoteDataManager::reqNoteDefIcons
 */
void VNoteDataManager::reqNoteDefIcons()
{
    // 重新启用图标加载，使用修复后的 MIPS64 兼容版本
    LoadIconsWorker *iconLoadWorker = new LoadIconsWorker();
    iconLoadWorker->setAutoDelete(true);
    QThreadPool::globalInstance()->start(iconLoadWorker, QThread::TimeCriticalPriority);
}

/**
 * @brief VNoteDataManager::reqNoteFolders
 */
void VNoteDataManager::reqNoteFolders()
{
    qCDebug(voiceNoteLog) << "Requesting note folders";
    if (m_pForldesLoadThread == nullptr) {
        m_pForldesLoadThread = new LoadFolderWorker();
        m_pForldesLoadThread->setAutoDelete(true);

        connect(m_pForldesLoadThread, &LoadFolderWorker::onFoldersLoaded,
                this, &VNoteDataManager::onFoldersLoaded);

        QThreadPool::globalInstance()->start(m_pForldesLoadThread);
        qCDebug(voiceNoteLog) << "Folder load worker started";
    } else {
        qCDebug(voiceNoteLog) << "Folder load already in progress";
    }
}

/**
 * @brief VNoteDataManager::reqNoteItems
 */
void VNoteDataManager::reqNoteItems()
{
    qCDebug(voiceNoteLog) << "Requesting note items";
    if (m_pNotesLoadThread == nullptr) {
        m_pNotesLoadThread = new LoadNoteItemsWorker();
        m_pNotesLoadThread->setAutoDelete(true);

        connect(m_pNotesLoadThread, &LoadNoteItemsWorker::onAllNotesLoaded,
                this, &VNoteDataManager::onAllNotesLoaded);

        QThreadPool::globalInstance()->start(m_pNotesLoadThread);
        qCDebug(voiceNoteLog) << "Note items load worker started";
    } else {
        qCDebug(voiceNoteLog) << "Note items load already in progress";
    }
}

void VNoteDataManager::reloadAllData()
{
    qCInfo(voiceNoteLog) << "Reloading all note data";
    m_fDataState = DataState::DataNotLoaded;
    reqNoteFolders();
    reqNoteItems();
}

/**
 * @brief VNoteDataManager::getAllNotesInFolder
 * @return 所有记事本的记事项数据
 */
VNOTE_ALL_NOTES_MAP *VNoteDataManager::getAllNotesInFolder()
{
    return m_qspAllNotesMap.get();
}

/**
 * @brief VNoteDataManager::onFoldersLoaded
 * @param foldesMap 记事本数据
 */
void VNoteDataManager::onFoldersLoaded(VNOTE_FOLDERS_MAP *foldesMap)
{
    //Release old data
    qCDebug(voiceNoteLog) << "Folders loaded, processing data";
    if (m_qspNoteFoldersMap != nullptr) {
        qCInfo(voiceNoteLog) << "Releasing old folders map, size:" << m_qspNoteFoldersMap->folders.size();

        m_qspNoteFoldersMap->lock.lockForWrite();

        for (auto it : m_qspNoteFoldersMap->folders) {
            delete it;
        }

        m_qspNoteFoldersMap->folders.clear();
        m_qspNoteFoldersMap->lock.unlock();
    }

    m_qspNoteFoldersMap.reset(foldesMap);

    qCInfo(voiceNoteLog) << "New folders map loaded, size:" << m_qspNoteFoldersMap->folders.size();

    //Object is already deleted
    m_pForldesLoadThread = nullptr;

    //Set folder data ready flag
    m_fDataState |= DataState::FolderDataReady;

    emit noteFoldersLoaded();

    // Send data ready signal if data ready
    if (isAllDatasReady()) {
        qCDebug(voiceNoteLog) << "All data ready, emitting signal";
        emit onAllDatasReady();
    }
}

/**
 * @brief VNoteDataManager::onAllNotesLoaded
 * @param notesMap 笔记数据
 */
void VNoteDataManager::onAllNotesLoaded(VNOTE_ALL_NOTES_MAP *notesMap)
{
    //Release old data
    qCDebug(voiceNoteLog) << "All notes loaded, processing data";
    if (m_qspAllNotesMap != nullptr) {
        qCInfo(voiceNoteLog) << "Releasing old notes map, folder count:" << m_qspAllNotesMap->notes.size();

        m_qspAllNotesMap->lock.lockForWrite();

        for (auto folderNotes : m_qspAllNotesMap->notes) {
            for (auto note : folderNotes->folderNotes) {
                delete note;
            }

            folderNotes->folderNotes.clear();
            delete reinterpret_cast<VNOTE_ITEMS_MAP *>(folderNotes);
        }

        m_qspAllNotesMap->notes.clear();
        m_qspAllNotesMap->lock.unlock();
    }

    m_qspAllNotesMap.reset(notesMap);

    qCInfo(voiceNoteLog) << "New notes map loaded, folder count:" << m_qspAllNotesMap->notes.size();

    //Object is already deleted
    m_pNotesLoadThread = nullptr;

    //Set folder data ready flag
    m_fDataState |= DataState::NotesDataReady;

    emit onNoteItemsLoaded();

    //Send data ready signal if data ready
    if (isAllDatasReady()) {
        qCDebug(voiceNoteLog) << "All data ready, emitting signal";
        emit onAllDatasReady();
    }
}
