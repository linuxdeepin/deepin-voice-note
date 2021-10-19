/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_upgradeview.h"
#include "upgradeview.h"
#include "importolddata/vnoteolddatamanager.h"
#include "common/vnoteforlder.h"

#include <stub.h>
#include <QThreadPool>

static void stub_void()
{
}

UT_UpgradeView::UT_UpgradeView()
{
}

TEST_F(UT_UpgradeView, UT_UpgradeView_onUpgradeFinish_001)
{
    UpgradeView upgradeview;
    upgradeview.onUpgradeFinish();
    EXPECT_EQ(100, upgradeview.m_waterProgress->value());
}

TEST_F(UT_UpgradeView, UT_UpgradeView_setProgress_001)
{
    UpgradeView upgradeview;
    upgradeview.setProgress(1);
    EXPECT_EQ(1, upgradeview.m_waterProgress->value());
}

TEST_F(UT_UpgradeView, UT_UpgradeView_startUpgrade_001)
{
    Stub stub;
    stub.set((void (QThreadPool::*)(QRunnable *, int))ADDR(QThreadPool, start), stub_void);
    UpgradeView upgradeview;
    upgradeview.startUpgrade();
}

TEST_F(UT_UpgradeView, UT_UpgradeView_onDataReady_001)
{
    VNOTE_FOLDERS_MAP *m_qspNoteFoldersMap = new VNOTE_FOLDERS_MAP;
    VNoteOldDataManager::instance()->m_qspNoteFoldersMap.reset(m_qspNoteFoldersMap);
    UpgradeView upgradeview;
    upgradeview.onDataReady();
}

TEST_F(UT_UpgradeView, UT_UpgradeView_onDataReady_002)
{
    Stub stub;
    stub.set((void (QThreadPool::*)(QRunnable *, int))ADDR(QThreadPool, start), stub_void);
    VNOTE_FOLDERS_MAP *qspNoteFoldersMap = new VNOTE_FOLDERS_MAP;
    VNoteFolder *folder = new VNoteFolder();
    qspNoteFoldersMap->folders.insert(1, folder);
    VNoteOldDataManager::instance()->m_qspNoteFoldersMap.reset(qspNoteFoldersMap);
    UpgradeView upgradeview;
    upgradeview.onDataReady();
    delete folder;
}
