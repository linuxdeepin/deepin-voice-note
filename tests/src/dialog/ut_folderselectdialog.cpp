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
    EXPECT_EQ(Qt::AlignCenter, folderselectdialog.m_noteInfo->alignment()) << "alignment";
    EXPECT_EQ("test", folderselectdialog.m_notesName) << "m_notesName";
    EXPECT_EQ(1, folderselectdialog.m_notesNumber) << "m_notesNumber";
    EXPECT_EQ(-1, folderselectdialog.getSelectIndex().row());
}

TEST_F(ut_folderselectdialog_test, clearSelection)
{
    QStandardItemModel data;
    FolderSelectDialog folderselectdialog(&data);
    folderselectdialog.clearSelection();
    EXPECT_FALSE(folderselectdialog.m_confirmBtn->isEnabled());
}

TEST_F(ut_folderselectdialog_test, hideEvent)
{
    QStandardItemModel data;
    FolderSelectDialog folderselectdialog(&data);
    QHideEvent *event = new QHideEvent();
    folderselectdialog.hideEvent(event);
    EXPECT_FALSE(folderselectdialog.m_closeButton->testAttribute(Qt::WA_UnderMouse)) << "attribute";
    EXPECT_FALSE(folderselectdialog.m_view->hasFocus()) << "hasfocus";
    delete event;
}

TEST_F(ut_folderselectdialog_test, refreshTextColor)
{
    QStandardItemModel dataModel;
    FolderSelectDialog folderselectdialog(&dataModel);
    QColor colorDark = QColor(255, 255, 255);
    folderselectdialog.refreshTextColor(true);
    EXPECT_EQ(colorDark, folderselectdialog.m_labMove->palette().windowText().color())
        << "dark is true, m_view color";
    colorDark.setAlphaF(0.7);
    EXPECT_EQ(colorDark, folderselectdialog.m_noteInfo->palette().windowText().color())
        << "dark is true, m_noteInfo color";
    QColor color = QColor(0, 0, 0, 1);
    color.setAlphaF(0.9);
    folderselectdialog.refreshTextColor(false);
    EXPECT_EQ(color, folderselectdialog.m_labMove->palette().windowText().color())
        << "dark is false, m_view color";
    color.setAlphaF(0.7);
    EXPECT_EQ(color, folderselectdialog.m_noteInfo->palette().windowText().color())
        << "dark is false, m_noteInfo color";
}

TEST_F(ut_folderselectdialog_test, setFolderBlack)
{
    VNoteFolder *folder1 = new VNoteFolder;
    QStandardItemModel dataModel;
    FolderSelectDialog folderselectdialog(&dataModel);
    QList<VNoteFolder *> blackList;
    blackList.push_back(folder1);
    folderselectdialog.setFolderBlack(blackList);
    EXPECT_EQ(blackList, folderselectdialog.m_model->m_blackFolders);
    delete folder1;
}
