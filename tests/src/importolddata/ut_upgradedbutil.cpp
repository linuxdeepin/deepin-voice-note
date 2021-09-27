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

#include "ut_upgradedbutil.h"
#include "upgradedbutil.h"
#include "vnoteforlder.h"

UT_UpgradeDbUtil::UT_UpgradeDbUtil()
{
}

void UT_UpgradeDbUtil::SetUp()
{
    m_upgradedbutil = new UpgradeDbUtil;
}

void UT_UpgradeDbUtil::TearDown()
{
    delete m_upgradedbutil;
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_saveUpgradeState_001)
{
    m_upgradedbutil->saveUpgradeState(m_upgradedbutil->Loading);
    EXPECT_EQ(UpgradeDbUtil::Loading, m_upgradedbutil->readUpgradeState());
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_readUpgradeState_001)
{
    m_upgradedbutil->saveUpgradeState(m_upgradedbutil->Loading);
    EXPECT_EQ(UpgradeDbUtil::Loading, m_upgradedbutil->readUpgradeState());
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_needUpdateOldDb_001)
{
    EXPECT_FALSE(m_upgradedbutil->needUpdateOldDb(m_upgradedbutil->Loading));
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_checkUpdateState_001)
{
    m_upgradedbutil->checkUpdateState(m_upgradedbutil->Processing);
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_backUpOldDb_001)
{
    m_upgradedbutil->backUpOldDb();
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_clearVoices_001)
{
    m_upgradedbutil->clearVoices();
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_doFolderUpgrade_001)
{
    VNoteFolder folder;
    folder.notesCount = 1;
    folder.defaultIcon = 1;
    folder.name = "test";
    folder.iconPath = "test1";
    folder.createTime = QDateTime::currentDateTime();
    folder.modifyTime = QDateTime::currentDateTime();
    folder.deleteTime = QDateTime::currentDateTime();
    m_upgradedbutil->doFolderUpgrade(&folder);
}

TEST_F(UT_UpgradeDbUtil, UT_UpgradeDbUtil_doFolderNoteUpgrade_001)
{
    m_upgradedbutil->doFolderNoteUpgrade(1, 1);
}
