// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatypedef.h"
#include "vnoteforlder.h"
#include "vnoteitem.h"
#include "vnoteitemoper.h"
#include "VNoteMainManager.h"

VNoteMainManager::VNoteMainManager()
{

}

VNoteMainManager::init()
{
    initData();
    initConnections();
}

void VNoteMainManager::initData()
{
    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    VNoteDataManager::instance()->reqNoteItems();
}

void VNoteMainManager::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onAllDatasReady,
        this, &VNoteMainManager::onVNoteFoldersLoaded);
}

void VNoteMainManager::initQMLRegister()
{
    qmlRegisterType<VNoteMainManager>("VNote", 1, 0, "VNoteMainWindow");
    qmlRegisterSingletonType<JsContent>("VNote", 1, 0, "Webobj", jsContent_provider);
}

void VNoteMainManager::onVNoteFoldersLoaded()
{
// #ifdef IMPORT_OLD_VERSION_DATA
//     if (m_fNeedUpgradeOldDb) {
//         //Start upgrade if all components are ready.
//         m_upgradeView->startUpgrade();
//         return;
//     }
// #endif

    //If have folders show note view,else show
    //default home page
    if (loadNotepads() > 0) {
    //     switchWidget(WndNoteShow);
    // } else {
    //     switchWidget(WndHomePage);
    }

//     PerformanceMonitor::initializeAppFinish();

//     //注册文件清理工作
//     FileCleanupWorker *pFileCleanupWorker =
//         new FileCleanupWorker(VNoteDataManager::instance()->getAllNotesInFolder(), this);
//     pFileCleanupWorker->setAutoDelete(true);
//     pFileCleanupWorker->setObjectName("FileCleanupWorker");
//     QThreadPool::globalInstance()->start(pFileCleanupWorker);
}

/**
 * @brief VNoteMainManager::loadNotepads
 * @return 记事本数量
 */
int VNoteMainManager::loadNotepads()
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    // QVariant value = setting::instance()->getOption(VNOTE_FOLDER_SORT);
    // QStringList tmpsortFolders = value.toString().split(",");

    int folderCount = 0;

    if (folders) {
        folders->lock.lockForRead();

        QList<QVariantMap> foldersDataList;
        foreach (VNoteFolder *folder, folders->folders) {
            QVariantMap data;
            data.insert("name", folder->name);
            data.insert("notesCount", folder->notesCount);
            data.insert("icon", folder->UI.icon);
            data.insert("grayIcon", folder->UI.grayIcon);

            foldersDataList.append(data);
        }

        folders->lock.unlock();
        emit finishedFolderLoad(foldersDataList);
    }

    return folderCount;
}

void VNoteMainManager::vNoteFloderChanged(const int &index)
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        folders->lock.lockForRead();

        VNoteFolder *folder = folders->folders.value(index+1);
        if (!loadNotes(folder)) {
        }

        folders->lock.unlock();
    }
}

void VNoteMainManager::vNoteChanged(const int &index)
{
    VNoteItem *data = m_noteItems.at(index);
    // if (data == nullptr || stateOperation->isSearching()) {
    //     m_recordBar->setVisible(false);
    // } else {
    //     m_recordBar->setVisible(true);
    // }
    //多选笔记时不更新详情页内容
    // if (!m_middleView->isMultipleSelected()) {
    //     m_richTextEdit->initData(data, m_searchKey, m_rightViewHasFouse);
    // }
    // //没有数据，插入图片按钮禁用
    // m_imgInsert->setDisabled(nullptr == data);
    // m_rightViewHasFouse = false;
}

/**
 * @brief VNoteMainManager::loadNotes
 * @param folder
 * @return 记事项数量
 */
int VNoteMainManager::loadNotes(VNoteFolder *folder)
{
    m_noteItems.clear();
    int notesCount = -1;
    if (folder) {
        notesCount = 0;
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(folder->id);
        QList<QVariantMap> notesDataList;
        if (notes) {
            notes->lock.lockForRead();
            for (auto it : notes->folderNotes) {
                // m_middleView->appendRow(it);
                QVariantMap data;
                data.insert("name", it->noteTitle);
                data.insert("time", it->modifyTime);
                notesDataList.append(data);
                m_noteItems.append(it);
                notesCount++;
            }
            notes->lock.unlock();

            //Sort the view & set focus on first item
            // m_middleView->onNoteChanged();
            // m_middleView->setCurrentIndex(0);
        }
        emit updateNotes(notesDataList);
    }
    return notesCount;
}
