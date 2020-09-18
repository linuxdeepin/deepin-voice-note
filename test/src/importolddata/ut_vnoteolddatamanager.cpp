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

ut_vnoteolddatamanager_test::ut_vnoteolddatamanager_test()
{

}

void ut_vnoteolddatamanager_test::SetUp()
{
    m_vnoteolddatamanager = new VNoteOldDataManager;
}

void ut_vnoteolddatamanager_test::TearDown()
{
    delete m_vnoteolddatamanager;
}

TEST_F(ut_vnoteolddatamanager_test, folders)
{
    m_vnoteolddatamanager->folders();
}

TEST_F(ut_vnoteolddatamanager_test, initOldDb)
{
    m_vnoteolddatamanager->initOldDb();
}

TEST_F(ut_vnoteolddatamanager_test, reqDatas)
{
    m_vnoteolddatamanager->reqDatas();
}

TEST_F(ut_vnoteolddatamanager_test, doUpgrade)
{
    m_vnoteolddatamanager->doUpgrade();
}

TEST_F(ut_vnoteolddatamanager_test, onFinishLoad)
{
    m_vnoteolddatamanager->onFinishLoad();
}

TEST_F(ut_vnoteolddatamanager_test, onFinishUpgrade)
{
    m_vnoteolddatamanager->onFinishUpgrade();
}

TEST_F(ut_vnoteolddatamanager_test, onProgress)
{
    m_vnoteolddatamanager->onProgress(1);
}
