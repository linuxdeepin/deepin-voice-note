// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatypedef.h"
#include "vnoteforlder.h"
#include "VNoteMainWindow.h"

VNoteMainWindow::VNoteMainWindow()
{
    initData();
    initConnections();
}

void VNoteMainWindow::initData()
{
    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    VNoteDataManager::instance()->reqNoteItems();
}

void VNoteMainWindow::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onAllDatasReady,
        this, &VNoteMainWindow::onVNoteFoldersLoaded);
}

void VNoteMainWindow::onVNoteFoldersLoaded()
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
 * @brief VNoteMainWindow::loadNotepads
 * @return 记事本数量
 */
int VNoteMainWindow::loadNotepads()
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    // QVariant value = setting::instance()->getOption(VNOTE_FOLDER_SORT);
    // QStringList tmpsortFolders = value.toString().split(",");

    int folderCount = 0;

    if (folders) {
        folders->lock.lockForRead();

        folderCount = folders->folders.size();

        QStringList folderNames;
        for (auto it : folders->folders) {
    //         int tmpIndexCount = tmpsortFolders.indexOf(QString::number(it->id));
    //         if (-1 != tmpIndexCount) {
    //             it->sortNumber = folderCount - tmpIndexCount;
    //         }
    //         m_leftView->appendFolder(it);
            qInfo() << "loadNotepads folder name:" << it->name;
            folderNames.append(it->name);
        }

        folders->lock.unlock();
        emit finishedFolderLoad(folderNames);
    //     m_leftView->sort();
    }

    return folderCount;
}