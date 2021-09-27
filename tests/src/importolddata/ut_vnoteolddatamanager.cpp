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

#include "ut_vnoteolddatamanager.h"
#include "vnoteolddatamanager.h"

UT_VNoteOldDataManager::UT_VNoteOldDataManager()
{
}

void UT_VNoteOldDataManager::SetUp()
{
    m_vnoteolddatamanager = new VNoteOldDataManager;
}

void UT_VNoteOldDataManager::TearDown()
{
    delete m_vnoteolddatamanager;
}

TEST_F(UT_VNoteOldDataManager, UT_VNoteOldDataManager_folders_001)
{
    EXPECT_EQ(nullptr, m_vnoteolddatamanager->folders());
}

TEST_F(UT_VNoteOldDataManager, UT_VNoteOldDataManager_initOldDb_001)
{
    m_vnoteolddatamanager->initOldDb();
    EXPECT_TRUE(nullptr != m_vnoteolddatamanager->m_oldDbManger);
}

TEST_F(UT_VNoteOldDataManager, UT_VNoteOldDataManager_reqDatas_001)
{
    //m_vnoteolddatamanager->reqDatas();
}

TEST_F(UT_VNoteOldDataManager, UT_VNoteOldDataManager_doUpgrade_001)
{
    //m_vnoteolddatamanager->doUpgrade();
}

TEST_F(UT_VNoteOldDataManager, UT_VNoteOldDataManager_onFinishLoad_001)
{
    m_vnoteolddatamanager->onFinishLoad();
}

TEST_F(UT_VNoteOldDataManager, UT_VNoteOldDataManager_onFinishUpgrade_001)
{
    m_vnoteolddatamanager->onFinishUpgrade();
}

TEST_F(UT_VNoteOldDataManager, UT_VNoteOldDataManager_onProgress_001)
{
    m_vnoteolddatamanager->onProgress(1);
}
