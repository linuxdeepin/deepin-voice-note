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

#include "ut_folderselectdialog.h"

#include "folderselectdialog.h"

#define protected public
#define private public
#include "leftview.h"
#include "leftviewdelegate.h"
#include "leftviewsortfilter.h"
#undef protected
#undef private

ut_folderselectdialog_test::ut_folderselectdialog_test()
{

}

TEST_F(ut_folderselectdialog_test, setNoteContext)
{
    LeftView leftview;
    FolderSelectDialog folderselectdialog(leftview.m_pDataModel);
    folderselectdialog.setNoteContext("test");
    folderselectdialog.getSelectIndex();
}
