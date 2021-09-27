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

#include "ut_vnotefolderoper.h"
#include "vnotefolderoper.h"
#include "vnotedatamanager.h"

UT_VNoteFolderOper::UT_VNoteFolderOper()
{
}

void UT_VNoteFolderOper::SetUp()
{
    VNoteFolder *folder = nullptr;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders && folders->folders.size()) {
        folder = folders->folders[0];
    }
    m_vnotefolderoper = new VNoteFolderOper(folder);
}

void UT_VNoteFolderOper::TearDown()
{
    delete m_vnotefolderoper;
}

TEST_F(UT_VNoteFolderOper, UT_VNoteFolderOper_deleteVNoteFolder_001)
{
    EXPECT_FALSE(m_vnotefolderoper->deleteVNoteFolder(4));
}

TEST_F(UT_VNoteFolderOper, UT_VNoteFolderOper_renameVNoteFolder_001)
{
    QString foldername = "test";
    EXPECT_FALSE(m_vnotefolderoper->renameVNoteFolder(foldername));
}

TEST_F(UT_VNoteFolderOper, UT_VNoteFolderOper_loadVNoteFolders_001)
{
    VNOTE_FOLDERS_MAP *folders = m_vnotefolderoper->loadVNoteFolders();
    EXPECT_FALSE(folders == nullptr);
    delete folders;
}

TEST_F(UT_VNoteFolderOper, UT_VNoteFolderOper_getNotesCount_001)
{
    EXPECT_TRUE(m_vnotefolderoper->getNotesCount() >= 0);
}

TEST_F(UT_VNoteFolderOper, UT_VNoteFolderOper_getDefaultIcon_001)
{
    EXPECT_TRUE(m_vnotefolderoper->getDefaultIcon() >= 0);
}

TEST_F(UT_VNoteFolderOper, UT_VNoteFolderOper_getFoldersCount_001)
{
    EXPECT_TRUE(m_vnotefolderoper->getFoldersCount() > 0);
}

TEST_F(UT_VNoteFolderOper, UT_VNoteFolderOper_getDefaultFolderName_001)
{
    EXPECT_FALSE(m_vnotefolderoper->getDefaultFolderName().isEmpty());
}
