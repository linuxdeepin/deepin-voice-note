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

#include "ut_middleview.h"

#define protected public
#define private public
#include "middleview.h"
#undef protected
#undef private

#include "middleviewdelegate.h"
#include "vnoteitem.h"

ut_middleview_test::ut_middleview_test()
{

}

TEST_F(ut_middleview_test, setSearchKey)
{
    MiddleView middleview;
    middleview.setSearchKey("test");
    middleview.m_pItemDelegate->handleChangeTheme();
    middleview.clearAll();
    middleview.closeMenu();
}

TEST_F(ut_middleview_test, setCurrentId)
{
    MiddleView middleview;
    middleview.setCurrentId(1);
    middleview.getCurrentId();
    VNoteItem vnoteitem;
    vnoteitem.noteId = 2;
    vnoteitem.folderId = 2;
}

TEST_F(ut_middleview_test, addRowAtHead)
{
    MiddleView middleview;
    VNoteItem *vnoteitem = new VNoteItem;
    vnoteitem->noteId = 2;
    vnoteitem->folderId = 2;
    vnoteitem->noteTitle = "test";
    middleview.addRowAtHead(vnoteitem);
    middleview.appendRow(vnoteitem);
    middleview.onNoteChanged();
    middleview.rowCount();
    middleview.setCurrentIndex(1);
    middleview.editNote();
    middleview.saveAsText();
    middleview.saveRecords();
    middleview.getCurrVNotedata();
    middleview.deleteCurrentRow();
}

TEST_F(ut_middleview_test, mouseEvent)
{
    MiddleView middleview;
    QPointF localPos;
    QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    middleview.mousePressEvent(event);
    middleview.mouseReleaseEvent(event);
    middleview.mouseDoubleClickEvent(event);
    middleview.mouseMoveEvent(event);
}

TEST_F(ut_middleview_test, eventFilter)
{
    MiddleView middleview;
    QKeyEvent* event = new QKeyEvent(QEvent::FocusIn, 0x01000016, Qt::NoModifier, "test");
    middleview.eventFilter(&middleview, event);
    QKeyEvent* event1 = new QKeyEvent(QEvent::Destroy, 0x01000001, Qt::NoModifier, "test");
    middleview.eventFilter(&middleview, event1);
}

TEST_F(ut_middleview_test, keyPressEvent)
{
    MiddleView middleview;
    QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, 0x01000016, Qt::NoModifier, "test");
    middleview.keyPressEvent(event);
    QKeyEvent* event1 = new QKeyEvent(QEvent::KeyPress, 0x01000001, Qt::NoModifier, "test");
    middleview.keyPressEvent(event1);
}

TEST_F(ut_middleview_test, setVisibleEmptySearch)
{
    MiddleView middleview;
    middleview.setVisibleEmptySearch(true);
    middleview.setOnlyCurItemMenuEnable(true);
}
