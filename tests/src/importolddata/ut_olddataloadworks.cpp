/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
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
#include "ut_olddataloadworks.h"
#include "importolddata/olddataloadwokers.h"
#include "common/vnoteforlder.h"
#include "importolddata/vnoteolddatamanager.h"

UT_OldDataLoadTask::UT_OldDataLoadTask()
{
}

TEST_F(UT_OldDataLoadTask, UT_OldDataLoadTask_OldDataLoadTask_001)
{
    OldDataLoadTask task;
    if (nullptr == VNoteOldDataManager::instance()->m_oldDbManger) {
        VNoteOldDataManager::instance()->initOldDb();
        EXPECT_FALSE(nullptr == VNoteOldDataManager::instance()->m_oldDbManger);
    }
    task.run();
}

TEST_F(UT_OldDataLoadTask, UT_OldDataLoadTask_OldDataUpgradeTask_001)
{
    VNOTE_FOLDERS_MAP *foldersMap = new VNOTE_FOLDERS_MAP();
    VNoteFolder *folder = new VNoteFolder;
    folder->notesCount = 0;
    folder->defaultIcon = 1;
    folder->name = "test";
    folder->iconPath = "test1";
    folder->id = 5;
    folder->createTime = QDateTime::currentDateTime();
    folder->modifyTime = QDateTime::currentDateTime();
    folder->deleteTime = QDateTime::currentDateTime();
    foldersMap->folders.insert(5, folder);
    foldersMap->autoRelease = true;
    VNoteOldDataManager::instance()->m_qspNoteFoldersMap.reset(foldersMap);
    OldDataUpgradeTask task;
    task.run();
}
