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

#include "ut_leftview.h"

#define protected public
#define private public
#include "leftview.h"
#undef protected
#undef private

#include "leftviewdelegate.h"
#include "vnoteforlder.h"

ut_leftview_test::ut_leftview_test()
{

}

TEST_F(ut_leftview_test, getNotepadRoot)
{
    LeftView leftview;
    leftview.m_pItemDelegate->handleChangeTheme();
    leftview.getNotepadRoot();
}

TEST_F(ut_leftview_test, getNotepadRootIndex)
{
    LeftView leftview;
    leftview.getNotepadRootIndex();
}

TEST_F(ut_leftview_test, mouseEvent)
{
    LeftView leftview;
    QPointF localPos;
    QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    leftview.mousePressEvent(event);
    leftview.mouseReleaseEvent(event);
    leftview.mouseDoubleClickEvent(event);
    leftview.mouseMoveEvent(event);
}

TEST_F(ut_leftview_test, keyPressEvent)
{
    LeftView leftview;
    QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, 0x01000016, Qt::NoModifier, "test");
    leftview.keyPressEvent(event);
    QKeyEvent* event1 = new QKeyEvent(QEvent::KeyPress, 0x01000001, Qt::NoModifier, "test");
    leftview.keyPressEvent(event1);
}


TEST_F(ut_leftview_test, restoreNotepadItem)
{
    LeftView leftview;
    leftview.restoreNotepadItem();
}

TEST_F(ut_leftview_test, addFolder)
{
    LeftView leftview;
    VNoteFolder folder;
    folder.notesCount = 1;
    folder.defaultIcon = 1;
    folder.name = "test";
    folder.iconPath = "test1";
    folder.createTime = QDateTime::currentDateTime();
    leftview.addFolder(&folder);
    leftview.editFolder();
    leftview.folderCount();
}

TEST_F(ut_leftview_test, appendFolder)
{
    LeftView leftview;
    VNoteFolder folder;
    folder.notesCount = 1;
    folder.defaultIcon = 1;
    folder.name = "test";
    folder.iconPath = "test1";
    folder.createTime = QDateTime::currentDateTime();
    leftview.appendFolder(&folder);
    leftview.removeFolder();
}

TEST_F(ut_leftview_test, setOnlyCurItemMenuEnable)
{
    LeftView leftview;
    leftview.setOnlyCurItemMenuEnable(false);
}

TEST_F(ut_leftview_test, closeMenu)
{
    LeftView leftview;
    leftview.closeMenu();
}

TEST_F(ut_leftview_test, closeEditor)
{
    LeftView leftview;
    QWidget* editor = new QWidget;
    leftview.closeEditor(editor, QAbstractItemDelegate::NoHint);
}
