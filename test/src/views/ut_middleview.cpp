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
#include "middleview.h"
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

//TEST_F(ut_middleview_test, addRowAtHead)
//{
//    MiddleView middleview;
//    VNoteItem *vnoteitem = new VNoteItem;
//    vnoteitem->noteId = 2;
//    vnoteitem->folderId = 2;
//    vnoteitem->noteTitle = "test";
//    middleview.addRowAtHead(vnoteitem);
//    middleview.appendRow(vnoteitem);
//    middleview.onNoteChanged();
//    middleview.rowCount();
//    middleview.setCurrentIndex(1);
//    middleview.editNote();
//    middleview.saveAsText();
//    middleview.saveRecords();
//    middleview.getCurrVNotedata();
//    middleview.deleteCurrentRow();
//}

TEST_F(ut_middleview_test, mouseEvent)
{
    MiddleView middleview;
    QPointF localPos;
    QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    middleview.mousePressEvent(event);
    middleview.mouseReleaseEvent(event);
    middleview.mouseDoubleClickEvent(event);
    middleview.mouseMoveEvent(event);
}

TEST_F(ut_middleview_test, eventFilter)
{
    MiddleView middleview;
    QKeyEvent *event = new QKeyEvent(QEvent::FocusIn, 0x01000016, Qt::NoModifier, "test");
    middleview.eventFilter(&middleview, event);
    QKeyEvent *event1 = new QKeyEvent(QEvent::Destroy, 0x01000001, Qt::NoModifier, "test");
    middleview.eventFilter(&middleview, event1);
}

TEST_F(ut_middleview_test, keyPressEvent)
{
    MiddleView middleview;
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, 0x01000016, Qt::NoModifier, "test");
    middleview.keyPressEvent(event);
    QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, 0x01000001, Qt::NoModifier, "test");
    middleview.keyPressEvent(event1);
}

TEST_F(ut_middleview_test, setVisibleEmptySearch)
{
    MiddleView middleview;
    middleview.setVisibleEmptySearch(true);
    middleview.setOnlyCurItemMenuEnable(true);
}

TEST_F(ut_middleview_test, selectAfterRemoved)
{
    MiddleView middleview;
    if(middleview.count()){
        middleview.m_nextSelection = 0;
    }
    middleview.selectAfterRemoved();
    middleview.m_nextSelection = -1;
    middleview.selectAfterRemoved();
}

TEST_F(ut_middleview_test, setNextSelection)
{
    MiddleView middleview;
    if(middleview.count()){
        middleview.setCurrentIndex(0);
    }
    middleview.setNextSelection();
}

TEST_F(ut_middleview_test, setDragSuccess)
{
    MiddleView middleview;
    bool value = true;
    middleview.setDragSuccess(value);
}

TEST_F(ut_middleview_test,changeRightView)
{
    MiddleView middleview;
    bool value = true;
    middleview.changeRightView(value);
}

TEST_F(ut_middleview_test,handleDragEvent)
{
    MiddleView middleview;
    middleview.m_onlyCurItemMenuEnable = false;
    bool value = true;
    middleview.handleDragEvent(value);
}

TEST_F(ut_middleview_test,deleteModelIndexs)
{
    MiddleView middleview;
    QModelIndexList indexList;
    if(middleview.count()){
        indexList.append(middleview.m_pDataModel->index(0,0));
    }
    middleview.deleteModelIndexs(indexList);
}

TEST_F(ut_middleview_test,keyReleaseEvent)
{
    MiddleView middleview;
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier, "test");
    middleview.keyReleaseEvent(event);
    QKeyEvent *event2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "test");
    middleview.keyReleaseEvent(event);
}

TEST_F(ut_middleview_test,getSelectedCount)
{
    MiddleView middleview;
    middleview.getSelectedCount();
}

//TEST_F(ut_middleview_test,onMenuShow){
//    QPoint point(128,128);
//    MiddleView middleview;
//    middleview.selectionModel()->select(middleview.m_pSortViewFilter->index(0, 0), QItemSelectionModel::Select);
//    middleview.onMenuShow(point);
//    middleview.selectionModel()->select(middleview.m_pSortViewFilter->index(1, 0), QItemSelectionModel::Select);
//    middleview.onMenuShow(point);
//}

//TEST_F(ut_middleview_test,haveVoice)
//{
//    MiddleView middleview;
//    QModelIndex index = middleview.indexAt(QCursor::pos());
//    middleview.selectionModel()->select(middleview.m_pSortViewFilter->index(index.row(), 0), QItemSelectionModel::Select);
//    middleview.haveVoice();
//}

TEST_F(ut_middleview_test,isMultipleSelected)
{
    MiddleView middleview;
    middleview.isMultipleSelected();
}

TEST_F(ut_middleview_test,setMouseState)
{
    MiddleView middleview;

    MiddleView::MouseState mouseState {MiddleView::MouseState::normal};
    middleview.setMouseState(mouseState);
}

TEST_F(ut_middleview_test,getModifierState)
{
    MiddleView middleview;
    middleview.getModifierState();
}

TEST_F(ut_middleview_test,setModifierState)
{
    MiddleView middleview;
    MiddleView::ModifierState modifierState {MiddleView::ModifierState::noModifier};
    middleview.setModifierState(modifierState);
}

TEST_F(ut_middleview_test,onNoteChanged)
{
    MiddleView middleview;
    middleview.onNoteChanged();
}

TEST_F(ut_middleview_test,getCurrVNotedataList)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.getCurrVNotedataList();
}

TEST_F(ut_middleview_test,getCurrVNotedata)
{
    MiddleView middleview;
    middleview.m_index = middleview.currentIndex();
    middleview.getCurrVNotedata();
}

TEST_F(ut_middleview_test,deleteCurrentRow)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.deleteCurrentRow();
}
