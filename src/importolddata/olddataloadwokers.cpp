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
}

/**
 * @brief OldDataLoadTask::run
 */
void OldDataLoadTask::run()
{
    VNOTE_FOLDERS_MAP *foldersMap = new VNOTE_FOLDERS_MAP();
    VNoteOldDataManager::instance()->m_qspNoteFoldersMap.reset(foldersMap);

    //DataManager should set autoRelease flag
    foldersMap->autoRelease = true;

    OldFolderQryDbVisitor folderVisitor(VNoteOldDataManager::instance()->m_oldDbManger->getVNoteDb(), nullptr, foldersMap);

    if (!VNoteDbManager::instance()->queryData(&folderVisitor)) {
        qCritical() << "Query old folder failed!";
    }

    VNOTE_ALL_NOTES_MAP *notesMap = new VNOTE_ALL_NOTES_MAP();
    VNoteOldDataManager::instance()->m_qspAllNotes.reset(notesMap);

    //DataManager data should set autoRelease flag
    notesMap->autoRelease = true;

    OldNoteQryDbVisitor noteVisitor(VNoteOldDataManager::instance()->m_oldDbManger->getVNoteDb(), nullptr, notesMap);

    if (!VNoteDbManager::instance()->queryData(&noteVisitor)) {
        qCritical() << "Query old notes failed!";
    }

    emit finishLoad();
}

/**
 * @brief OldDataUpgradeTask::OldDataUpgradeTask
 * @param parent
 */
OldDataUpgradeTask::OldDataUpgradeTask(QObject *parent)
    : VNTask(parent)
{
}

/**
 * @brief OldDataUpgradeTask::run
 */
void OldDataUpgradeTask::run()
{
    int folderCount = VNoteOldDataManager::instance()->folders()->folders.count();
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

    emit finishUpgrade();
}
