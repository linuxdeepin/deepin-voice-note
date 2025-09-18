// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "olddataloadwokers.h"
#include "olddbvisistors.h"
#include "upgradedbutil.h"
#include "vnoteolddatamanager.h"

#include <DLog>

/**
 * @brief OldDataLoadTask::OldDataLoadTask
 * @param parent
 */
OldDataLoadTask::OldDataLoadTask(QObject *parent)
    : VNTask(parent)
{
    qInfo() << "OldDataLoadTask constructor called";
}

/**
 * @brief OldDataLoadTask::run
 */
void OldDataLoadTask::run()
{
    qInfo() << "Starting old data load task";
    VNOTE_FOLDERS_MAP *foldersMap = new VNOTE_FOLDERS_MAP();
    VNoteOldDataManager::instance()->m_qspNoteFoldersMap.reset(foldersMap);

    //DataManager should set autoRelease flag
    foldersMap->autoRelease = true;

    OldFolderQryDbVisitor folderVisitor(VNoteOldDataManager::instance()->m_oldDbManger->getVNoteDb(), nullptr, foldersMap);

    if (!VNoteDbManager::instance()->queryData(&folderVisitor)) {
        qCritical() << "Query old folder failed!";
    } else {
        qInfo() << "Successfully queried old folders, count:" << foldersMap->folders.count();
    }

    VNOTE_ALL_NOTES_MAP *notesMap = new VNOTE_ALL_NOTES_MAP();
    VNoteOldDataManager::instance()->m_qspAllNotes.reset(notesMap);

    //DataManager data should set autoRelease flag
    notesMap->autoRelease = true;

    OldNoteQryDbVisitor noteVisitor(VNoteOldDataManager::instance()->m_oldDbManger->getVNoteDb(), nullptr, notesMap);

    if (!VNoteDbManager::instance()->queryData(&noteVisitor)) {
        qCritical() << "Query old notes failed!";
    } else {
        qInfo() << "Successfully queried old notes, count:" << notesMap->notes.count();
    }

    qInfo() << "Old data load task completed";
    emit finishLoad();
}

/**
 * @brief OldDataUpgradeTask::OldDataUpgradeTask
 * @param parent
 */
OldDataUpgradeTask::OldDataUpgradeTask(QObject *parent)
    : VNTask(parent)
{
    qInfo() << "OldDataUpgradeTask constructor called";
}

/**
 * @brief OldDataUpgradeTask::run
 */
void OldDataUpgradeTask::run()
{
    qInfo() << "Starting old data upgrade task";
    int folderCount = VNoteOldDataManager::instance()->folders()->folders.count();
    qDebug() << "Total folders to upgrade:" << folderCount;
    int progress = 0;

    int index = 1;
    for (auto it : VNoteOldDataManager::instance()->folders()->folders) {
        int curProgressValue = ((index)*100) / folderCount;

        //Do folder upgrade here.
        UpgradeDbUtil::doFolderUpgrade(it);

        if (progress != curProgressValue) {
            progress = curProgressValue;
            emit progressValue(progress);
        }

        index++;
    }

    qInfo() << "Old data upgrade task completed";
    emit finishUpgrade();
}
