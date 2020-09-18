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

ut_upgradedbutil_test::ut_upgradedbutil_test()
{

}

void ut_upgradedbutil_test::SetUp()
{
    m_upgradedbutil = new UpgradeDbUtil;
}

void ut_upgradedbutil_test::TearDown()
{
    delete m_upgradedbutil;
}

TEST_F(ut_upgradedbutil_test, saveUpgradeState)
{
    m_upgradedbutil->saveUpgradeState(m_upgradedbutil->Loading);
}

TEST_F(ut_upgradedbutil_test, readUpgradeState)
{
    m_upgradedbutil->readUpgradeState();
}

TEST_F(ut_upgradedbutil_test, needUpdateOldDb)
{
    m_upgradedbutil->needUpdateOldDb(m_upgradedbutil->Loading);
}

TEST_F(ut_upgradedbutil_test, checkUpdateState)
{
    m_upgradedbutil->checkUpdateState(m_upgradedbutil->Processing);
}

TEST_F(ut_upgradedbutil_test, backUpOldDb)
{
    m_upgradedbutil->backUpOldDb();
}

TEST_F(ut_upgradedbutil_test, clearVoices)
{
    m_upgradedbutil->clearVoices();
}

TEST_F(ut_upgradedbutil_test, doFolderUpgrade)
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

TEST_F(ut_upgradedbutil_test, doFolderNoteUpgrade)
{
    m_upgradedbutil->doFolderNoteUpgrade(1, 1);
}
