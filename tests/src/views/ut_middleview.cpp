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
#include "middleviewsortfilter.h"
#include "vnotedatamanager.h"
#include "vnoteitemoper.h"
#include "vnoteforlder.h"

#include <standarditemcommon.h>
#include <vnoteitemoper.h>

ut_middleview_test::ut_middleview_test()
{
}

TEST_F(ut_middleview_test, setSearchKey)
{
    MiddleView middleview;
    VNoteFolder *folder = VNoteDataManager::instance()->getNoteFolders()->folders[0];
    VNoteItemOper noteOper;
    VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(folder->id);
    if (notes) {
        notes->lock.lockForRead();
        int i = 0;
        for (auto it : notes->folderNotes) {
            it->isTop = i % 2 == 0 ? true : false;
            i++;
            middleview.appendRow(it);
        }
        notes->lock.unlock();
    }
    middleview.grab();
    middleview.setSearchKey("note");
    middleview.grab();
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
    //mousePressEvent
    middleview.m_onlyCurItemMenuEnable = false;
    QMouseEvent *mousePressEvent = new QMouseEvent(QEvent::MouseButtonPress, localPos, localPos, localPos, Qt::RightButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    middleview.mousePressEvent(mousePressEvent);
    middleview.m_onlyCurItemMenuEnable = true;
    middleview.mousePressEvent(mousePressEvent);

    //mouseReleaseEvent
    QMouseEvent *releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    middleview.m_onlyCurItemMenuEnable = false;
    QPoint point = QPoint(middleview.visualRect(middleview.currentIndex()).topLeft().x() + 10, middleview.visualRect(middleview.currentIndex()).topLeft().y() - 10);
    middleview.mouseReleaseEvent(releaseEvent);

    //doubleClickEvent
    middleview.m_onlyCurItemMenuEnable = false;
    QMouseEvent *doubleClickEvent = new QMouseEvent(QEvent::MouseButtonDblClick, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    middleview.mouseDoubleClickEvent(doubleClickEvent);

    //mouseMoveEvent
    middleview.m_onlyCurItemMenuEnable = false;
    QMouseEvent *mouseMoveEvent = new QMouseEvent(QEvent::MouseMove, localPos, localPos, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    middleview.setTouchState(middleview.TouchMoving);
    middleview.mouseMoveEvent(mouseMoveEvent);
}

TEST_F(ut_middleview_test, setTouchState)
{
    MiddleView middleview;
    middleview.setTouchState(middleview.TouchNormal);
}

TEST_F(ut_middleview_test, doTouchMoveEvent)
{
    MiddleView middleview;
    QPointF localPos;
    QMouseEvent *mouseMoveEvent = new QMouseEvent(QEvent::MouseMove, localPos, localPos, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    middleview.m_touchPressPoint = QPoint(localPos.x(), localPos.y() - 11);
    QDateTime time = QDateTime::currentDateTime();
    middleview.m_touchPressStartMs = time.toMSecsSinceEpoch() - 260;
    middleview.setTouchState(middleview.TouchMoving);
    middleview.doTouchMoveEvent(mouseMoveEvent);
}

TEST_F(ut_middleview_test, setOnlyCurItemMenuEnable)
{
    MiddleView middleview;
    bool enable = false;
    middleview.setOnlyCurItemMenuEnable(enable);
}

TEST_F(ut_middleview_test, eventFilter)
{
    MiddleView middleview;
    QFocusEvent focusInEvent(QEvent::FocusIn, Qt::TabFocusReason);
    middleview.eventFilter(&middleview, &focusInEvent);
    QFocusEvent focusOutEvent(QEvent::FocusOut);
    middleview.eventFilter(&middleview, &focusOutEvent);
    QKeyEvent *event1 = new QKeyEvent(QEvent::Destroy, 0x01000001, Qt::NoModifier, "test");
    middleview.eventFilter(nullptr, event1);
    middleview.eventFilter(nullptr, &focusInEvent);
}

TEST_F(ut_middleview_test, addRowAtHead)
{
    MiddleView middleview;
    VNoteItem *vnoteitem = new VNoteItem;
    vnoteitem->noteId = 2;
    vnoteitem->folderId = 2;
    vnoteitem->noteTitle = "test";
    middleview.addRowAtHead(vnoteitem);
}

TEST_F(ut_middleview_test, appendRow)
{
    MiddleView middleview;
    VNoteItem *noteData = new VNoteItem;
    VNoteItem *noteData1 = new VNoteItem;
    middleview.appendRow(noteData);
    middleview.appendRow(noteData1);
    middleview.sortView();
    noteData1->isTop = 1;
    middleview.sortView();
    noteData1->isTop = 0;
    middleview.m_pSortViewFilter->sortView(MiddleViewSortFilter::title);
    middleview.m_pSortViewFilter->sortView(MiddleViewSortFilter::createTime);
}

TEST_F(ut_middleview_test, rowCount)
{
    MiddleView middleview;
    middleview.rowCount();
}

TEST_F(ut_middleview_test, editNote)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.editNote();
}

TEST_F(ut_middleview_test, saveAsText)
{
    MiddleView middleview;
    middleview.selectionModel()->select(middleview.m_pDataModel->index(0, 0), QItemSelectionModel::Select);
    middleview.saveAsText();
}

TEST_F(ut_middleview_test, saveRecords)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.saveRecords();
}

TEST_F(ut_middleview_test, handleShiftAndPress)
{
    MiddleView middleview;
    middleview.setModifierState(middleview.shiftAndUpOrDownModifier);
    middleview.m_currentRow = 0;
    QModelIndex index = middleview.m_pDataModel->index(1, 0);
    middleview.handleShiftAndPress(index);
}

TEST_F(ut_middleview_test, keyPressEvent)
{
    MiddleView middleview;
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::ShiftModifier, "test");
    middleview.keyPressEvent(event);
    middleview.m_shiftSelection = -1;
    QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::ShiftModifier, "test2");
    middleview.keyPressEvent(event1);
    QKeyEvent *event2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Home, Qt::ShiftModifier, "test3");
    middleview.keyPressEvent(event2);
    delete event;
    //    delete event1;
    delete event2;
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
    if (middleview.count()) {
        middleview.m_nextSelection = 0;
    }
    middleview.selectAfterRemoved();
    middleview.m_nextSelection = -1;
    middleview.selectAfterRemoved();
}

TEST_F(ut_middleview_test, setNextSelection)
{
    MiddleView middleview;
    if (middleview.count()) {
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

TEST_F(ut_middleview_test, changeRightView)
{
    MiddleView middleview;
    bool value = true;
    middleview.changeRightView(value);
}

TEST_F(ut_middleview_test, handleDragEvent)
{
    MiddleView middleview;
    middleview.m_onlyCurItemMenuEnable = false;
    bool value = true;
    middleview.handleDragEvent(value);
}

TEST_F(ut_middleview_test, deleteModelIndexs)
{
    MiddleView middleview;
    QModelIndexList indexList;
    if (middleview.count()) {
        indexList.append(middleview.m_pDataModel->index(0, 0));
    }
    middleview.deleteModelIndexs(indexList);
}

TEST_F(ut_middleview_test, keyReleaseEvent)
{
    MiddleView middleview;
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier, "test");
    middleview.keyReleaseEvent(event);
    QKeyEvent *event2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "test");
    middleview.keyReleaseEvent(event);
}

TEST_F(ut_middleview_test, getSelectedCount)
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

TEST_F(ut_middleview_test, haveText)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.haveText();
}

TEST_F(ut_middleview_test, haveVoice)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.haveVoice();
}

TEST_F(ut_middleview_test, handleTouchSlideEvent)
{
    MiddleView middleview;
    middleview.handleTouchSlideEvent(24, 11, QCursor::pos());
}

TEST_F(ut_middleview_test, isMultipleSelected)
{
    MiddleView middleview;
    middleview.isMultipleSelected();
}

TEST_F(ut_middleview_test, setMouseState)
{
    MiddleView middleview;

    MiddleView::MouseState mouseState {MiddleView::MouseState::normal};
    middleview.setMouseState(mouseState);
}

TEST_F(ut_middleview_test, getModifierState)
{
    MiddleView middleview;
    middleview.getModifierState();
}

TEST_F(ut_middleview_test, setModifierState)
{
    MiddleView middleview;
    MiddleView::ModifierState modifierState {MiddleView::ModifierState::noModifier};
    middleview.setModifierState(modifierState);
}

TEST_F(ut_middleview_test, onNoteChanged)
{
    MiddleView middleview;
    middleview.onNoteChanged();
}

TEST_F(ut_middleview_test, getCurrVNotedataList)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.getCurrVNotedataList();
}

TEST_F(ut_middleview_test, getCurrVNotedata)
{
    MiddleView middleview;
    middleview.m_index = middleview.currentIndex();
    middleview.getCurrVNotedata();
}

TEST_F(ut_middleview_test, deleteCurrentRow)
{
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    middleview.deleteCurrentRow();
}
