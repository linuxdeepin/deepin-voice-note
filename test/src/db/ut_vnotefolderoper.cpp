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

ut_vnotefolderoper_test::ut_vnotefolderoper_test()
{

}

void ut_vnotefolderoper_test::SetUp()
{
    m_vnotefolderoper = new VNoteFolderOper;
}

void ut_vnotefolderoper_test::TearDown()
{
    delete m_vnotefolderoper;
}

TEST_F(ut_vnotefolderoper_test, deleteVNoteFolder)
{
//    m_vnotefolderoper->deleteVNoteFolder(1);
}

TEST_F(ut_vnotefolderoper_test, renameVNoteFolder)
{
    QString foldername = "test";
    m_vnotefolderoper->renameVNoteFolder(foldername);
}

TEST_F(ut_vnotefolderoper_test, loadVNoteFolders)
{
    m_vnotefolderoper->loadVNoteFolders();
}

TEST_F(ut_vnotefolderoper_test, getNotesCount)
{
    m_vnotefolderoper->getNotesCount();
}

TEST_F(ut_vnotefolderoper_test, getDefaultIcon)
{
    m_vnotefolderoper->getDefaultIcon();
}
