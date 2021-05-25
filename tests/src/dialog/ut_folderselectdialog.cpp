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
#include "leftview.h"
#include "leftviewdelegate.h"
#include "leftviewsortfilter.h"
#include "common/vnoteforlder.h"

#include <QStandardItemModel>

ut_folderselectdialog_test::ut_folderselectdialog_test()
{
}

TEST_F(ut_folderselectdialog_test, setNoteContext)
{
    QStandardItemModel data;
    FolderSelectDialog folderselectdialog(&data);
    folderselectdialog.setNoteContextInfo("test", 1);
    folderselectdialog.getSelectIndex();
}

TEST_F(ut_folderselectdialog_test, clearSelection)
{
    QStandardItemModel data;
    FolderSelectDialog folderselectdialog(&data);
    folderselectdialog.clearSelection();
}

TEST_F(ut_folderselectdialog_test, hideEvent)
{
    QStandardItemModel data;
    FolderSelectDialog folderselectdialog(&data);
    QHideEvent *event = new QHideEvent();
    folderselectdialog.hideEvent(event);
    delete event;
}

TEST_F(ut_folderselectdialog_test, refreshTextColor)
{
    QStandardItemModel dataModel;
    FolderSelectDialog folderselectdialog(&dataModel);
    folderselectdialog.refreshTextColor(true);
    folderselectdialog.refreshTextColor(false);
}

TEST_F(ut_folderselectdialog_test, setFolderBlack)
{
    VNoteFolder *folder1 = new VNoteFolder;
    QStandardItemModel dataModel;
    FolderSelectDialog folderselectdialog(&dataModel);
    QList<VNoteFolder *> blackList;
    blackList.push_back(folder1);
    folderselectdialog.setFolderBlack(blackList);
    delete folder1;
}
