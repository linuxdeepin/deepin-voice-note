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
#include "stub.h"

#include <standarditemcommon.h>
#include <vnoteitemoper.h>

#include <DFileDialog>
DWIDGET_USE_NAMESPACE

static int stub_dialog()
{
    return 1;
}

ut_middleview_test::ut_middleview_test()
{
}

void ut_middleview_test::SetUp()
{
    m_middleView = new MiddleView;
    VNoteFolder *folder = VNoteDataManager::instance()->getNoteFolders()->folders[0];
    VNoteItemOper noteOper;
    VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(folder->id);
    if (notes) {
        notes->lock.lockForRead();
        int i = 0;
        for (auto it : notes->folderNotes) {
            it->isTop = i % 2 == 0 ? true : false;
            i++;
            m_middleView->appendRow(it);
        }
        notes->lock.unlock();
    }
}

void ut_middleview_test::TearDown()
{
    delete m_middleView;
}

TEST_F(ut_middleview_test, setSearchKey)
{
    m_middleView->grab();
    m_middleView->setSearchKey("note");
    EXPECT_EQ(m_middleView->m_searchKey, QString("note"));
    m_middleView->grab();
    m_middleView->m_pItemDelegate->handleChangeTheme();
    m_middleView->clearAll();
    EXPECT_FALSE(m_middleView->rowCount());
    m_middleView->closeMenu();
    EXPECT_FALSE(m_middleView->m_noteMenu->isVisible());
}

TEST_F(ut_middleview_test, setCurrentId)
{
    MiddleView middleview;
    middleview.setCurrentId(1);
    EXPECT_EQ(1, middleview.getCurrentId());
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
    m_middleView->selectAll();
    QModelIndex index = m_middleView->currentIndex();
    QPoint localPos, globalPos;
    if (index.isValid()) {
        QRect rc = m_middleView->visualRect(index);
        localPos = rc.center();
        globalPos = m_middleView->mapToGlobal(localPos);
    }

    m_middleView->m_onlyCurItemMenuEnable = false;
    QMouseEvent *mousePressEvent = new QMouseEvent(QEvent::MouseButtonPress, localPos, localPos, localPos, Qt::LeftButton, Qt::RightButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    m_middleView->mousePressEvent(mousePressEvent);
    mousePressEvent->setModifiers(Qt::ControlModifier);
    m_middleView->mousePressEvent(mousePressEvent);
    m_middleView->setModifierState(MiddleView::shiftAndMouseModifier);
    m_middleView->mousePressEvent(mousePressEvent);
    mousePressEvent->setModifiers(Qt::ShiftModifier);
    m_middleView->mousePressEvent(mousePressEvent);
    m_middleView->m_onlyCurItemMenuEnable = true;
    m_middleView->mousePressEvent(mousePressEvent);
    delete mousePressEvent;

    m_middleView->m_onlyCurItemMenuEnable = false;
    QMouseEvent *rMousePressEvent = new QMouseEvent(QEvent::MouseButtonPress, localPos, localPos, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    m_middleView->mousePressEvent(rMousePressEvent);
    rMousePressEvent->setModifiers(Qt::ShiftModifier);
    m_middleView->mousePressEvent(rMousePressEvent);
    rMousePressEvent->setModifiers(Qt::ControlModifier);
    m_middleView->mousePressEvent(rMousePressEvent);
    delete rMousePressEvent;

    //mouseReleaseEvent
    QMouseEvent *releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    m_middleView->m_onlyCurItemMenuEnable = false;
    m_middleView->mouseReleaseEvent(releaseEvent);
    delete releaseEvent;

    //doubleClickEvent
    m_middleView->m_onlyCurItemMenuEnable = false;
    QMouseEvent *doubleClickEvent = new QMouseEvent(QEvent::MouseButtonDblClick, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_middleView->mouseDoubleClickEvent(doubleClickEvent);
    delete doubleClickEvent;

    //mouseMoveEvent
    m_middleView->m_onlyCurItemMenuEnable = false;
    QMouseEvent *mouseMoveEvent = new QMouseEvent(QEvent::MouseMove, localPos, localPos, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    m_middleView->setTouchState(MiddleView::TouchMoving);
    m_middleView->mouseMoveEvent(mouseMoveEvent);
    delete mouseMoveEvent;
}

TEST_F(ut_middleview_test, setTouchState)
{
    MiddleView middleview;
    middleview.setTouchState(middleview.TouchNormal);
    EXPECT_EQ(m_middleView->m_touchState, middleview.TouchNormal);
}

TEST_F(ut_middleview_test, doTouchMoveEvent)
{
    MiddleView middleview;
    QPointF localPos;
    QMouseEvent *mouseMoveEvent = new QMouseEvent(QEvent::MouseMove, localPos, localPos, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    middleview.m_touchPressPoint = QPoint(localPos.x(), localPos.y() - 11);
    QDateTime time = QDateTime::currentDateTime();
    middleview.m_touchPressStartMs = time.toMSecsSinceEpoch() - 260;
    middleview.setTouchState(MiddleView::TouchMoving);
    EXPECT_EQ(middleview.m_touchState, MiddleView::TouchMoving);
    middleview.doTouchMoveEvent(mouseMoveEvent);
    delete mouseMoveEvent;
}

TEST_F(ut_middleview_test, setOnlyCurItemMenuEnable)
{
    MiddleView middleview;
    bool enable = false;
    middleview.setOnlyCurItemMenuEnable(enable);
    EXPECT_EQ(middleview.m_onlyCurItemMenuEnable, enable);
}

TEST_F(ut_middleview_test, eventFilter)
{
    MiddleView middleview;
    QFocusEvent focusInEvent(QEvent::FocusIn, Qt::TabFocusReason);
    middleview.eventFilter(&middleview, &focusInEvent);
    EXPECT_EQ(middleview.m_pItemDelegate->m_tabFocus, true);
    QFocusEvent focusOutEvent(QEvent::FocusOut);
    middleview.eventFilter(&middleview, &focusOutEvent);
    EXPECT_EQ(middleview.m_pItemDelegate->m_tabFocus, false);
    QKeyEvent *event1 = new QKeyEvent(QEvent::Destroy, 0x01000001, Qt::NoModifier, "test");
    EXPECT_FALSE(middleview.eventFilter(nullptr, event1));
    middleview.eventFilter(nullptr, &focusInEvent);
    EXPECT_EQ(middleview.m_pItemDelegate->m_editVisible, true);
    delete event1;
}

TEST_F(ut_middleview_test, addRowAtHead)
{
    MiddleView middleview;
    VNoteItem *vnoteitem = new VNoteItem;
    vnoteitem->noteId = 2;
    vnoteitem->folderId = 2;
    vnoteitem->noteTitle = "test";
    middleview.addRowAtHead(vnoteitem);
    EXPECT_EQ(1, middleview.rowCount());
    delete vnoteitem;
}

TEST_F(ut_middleview_test, appendRow)
{
    MiddleView middleview;
    VNoteItem *noteData = new VNoteItem;
    VNoteItem *noteData1 = new VNoteItem;
    middleview.appendRow(noteData);
    middleview.appendRow(noteData1);
    EXPECT_EQ(2, middleview.rowCount());
    middleview.sortView();
    noteData1->isTop = 1;
    middleview.sortView();
    middleview.m_pSortViewFilter->sortView(MiddleViewSortFilter::title);
    EXPECT_EQ(middleview.m_pSortViewFilter->m_sortFeild, MiddleViewSortFilter::title);
    middleview.m_pSortViewFilter->sortView(MiddleViewSortFilter::createTime);
    EXPECT_EQ(middleview.m_pSortViewFilter->m_sortFeild, MiddleViewSortFilter::createTime);
    delete noteData;
    delete noteData1;
}

TEST_F(ut_middleview_test, rowCount)
{
    MiddleView middleview;
    EXPECT_EQ(qint32(0), middleview.rowCount());
}

TEST_F(ut_middleview_test, editNote)
{
    m_middleView->setCurrentIndex(0);
    m_middleView->editNote();
}

TEST_F(ut_middleview_test, saveAsText)
{
    typedef int (*fptr)();
    fptr A_foo = (fptr)(&DFileDialog::exec);
    Stub stub;
    stub.set(A_foo, stub_dialog);
    m_middleView->selectAll();
    EXPECT_FALSE(m_middleView->selectedIndexes().isEmpty());
    //m_middleView->saveAsText();
}

TEST_F(ut_middleview_test, saveRecords)
{
    typedef int (*fptr)();
    fptr A_foo = (fptr)(&DFileDialog::exec);
    Stub stub;
    stub.set(A_foo, stub_dialog);
    m_middleView->selectAll();
    EXPECT_FALSE(m_middleView->selectedIndexes().isEmpty());
    m_middleView->saveRecords();
}

TEST_F(ut_middleview_test, handleShiftAndPress)
{
    m_middleView->setCurrentIndex(0);
    QModelIndex index = m_middleView->currentIndex();
    m_middleView->handleShiftAndPress(index);
    EXPECT_EQ(m_middleView->getModifierState(), MiddleView::shiftAndMouseModifier);
}

TEST_F(ut_middleview_test, keyPressEvent)
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::ShiftModifier, "test");
    m_middleView->initPositionStatus(0);
    EXPECT_EQ(m_middleView->m_shiftSelection, -1);
    EXPECT_EQ(m_middleView->m_currentRow, 0);

    m_middleView->keyPressEvent(event);
    m_middleView->initPositionStatus(1);
    EXPECT_EQ(m_middleView->m_currentRow, 1);
    m_middleView->keyPressEvent(event);
    m_middleView->initPositionStatus(0);
    EXPECT_EQ(m_middleView->m_currentRow, 0);
    m_middleView->m_shiftSelection = 1;
    m_middleView->keyPressEvent(event);
    event->setModifiers(Qt::NoModifier);
    m_middleView->keyPressEvent(event);

    QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::ShiftModifier, "test2");
    m_middleView->initPositionStatus(0);
    m_middleView->keyPressEvent(event1);
    m_middleView->initPositionStatus(1);
    EXPECT_EQ(m_middleView->m_currentRow, 1);
    m_middleView->m_shiftSelection = 0;
    m_middleView->keyPressEvent(event1);
    m_middleView->m_shiftSelection = -1;
    m_middleView->keyPressEvent(event1);

    QKeyEvent *event2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Home, Qt::ShiftModifier, "test3");
    m_middleView->keyPressEvent(event2);
    event2->setModifiers(Qt::NoModifier);
    m_middleView->keyPressEvent(event2);

    QKeyEvent *event3 = new QKeyEvent(QEvent::KeyPress, Qt::Key_End, Qt::ShiftModifier, "test3");
    m_middleView->keyPressEvent(event3);
    event3->setModifiers(Qt::NoModifier);
    m_middleView->keyPressEvent(event3);

    QKeyEvent *event4 = new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier, "test3");
    m_middleView->keyPressEvent(event4);

    delete event;
    delete event1;
    delete event2;
    delete event3;
    delete event4;
}

TEST_F(ut_middleview_test, setVisibleEmptySearch)
{
    MiddleView middleview;
    middleview.setVisibleEmptySearch(true);
    EXPECT_FALSE(nullptr == middleview.m_emptySearch);
    middleview.setOnlyCurItemMenuEnable(true);
    EXPECT_EQ(middleview.m_onlyCurItemMenuEnable, true);
}

TEST_F(ut_middleview_test, setDragSuccess)
{
    MiddleView middleview;
    bool value = true;
    middleview.setDragSuccess(value);
    EXPECT_EQ(value, middleview.m_dragSuccess);
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
    EXPECT_EQ(middleview.m_touchState, MiddleView::TouchDraging);
}

TEST_F(ut_middleview_test, deleteModelIndexs)
{
    m_middleView->selectAll();
    QModelIndexList indexList = m_middleView->selectedIndexes();
    m_middleView->deleteModelIndexs(indexList);
    EXPECT_FALSE(m_middleView->rowCount());
}

TEST_F(ut_middleview_test, keyReleaseEvent)
{
    MiddleView middleview;
    middleview.m_shiftSelection = 1;
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier, "test");
    middleview.keyReleaseEvent(event);
    EXPECT_EQ(middleview.m_shiftSelection, -1);
    middleview.m_shiftSelection = 1;
    QKeyEvent *event2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "test");
    middleview.keyReleaseEvent(event2);
    EXPECT_EQ(middleview.m_shiftSelection, 1);
    delete event;
    delete event2;
}

TEST_F(ut_middleview_test, getSelectedCount)
{
    MiddleView middleview;
    EXPECT_EQ(middleview.getSelectedCount(), 0);
}

//TEST_F(ut_middleview_test,onMenuShow){
//    QPoint point(128,128);
//    MiddleView middleview;
//    middleview.selectionModel()->select(middleview.m_pSortViewFilter->index(0, 0), QItemSelectionModel::Select);
//    middleview.onMenuShow(point);
//    middleview.selectionModel()->select(middleview.m_pSortViewFilter->index(1, 0), QItemSelectionModel::Select);
//    middleview.onMenuShow(point);
//}

TEST_F(ut_middleview_test, haveText)
{
    m_middleView->selectAll();
    EXPECT_TRUE(m_middleView->getSelectedCount());
    EXPECT_TRUE(m_middleView->haveText());
}

TEST_F(ut_middleview_test, haveVoice)
{
    m_middleView->selectAll();
    EXPECT_TRUE(m_middleView->getSelectedCount());
    EXPECT_TRUE(m_middleView->haveVoice());
}

TEST_F(ut_middleview_test, handleTouchSlideEvent)
{
    MiddleView middleview;
    middleview.handleTouchSlideEvent(24, 11, QCursor::pos());
    EXPECT_EQ(middleview.m_touchPressPoint, QCursor::pos());
}

TEST_F(ut_middleview_test, isMultipleSelected)
{
    MiddleView middleview;
    EXPECT_FALSE(middleview.isMultipleSelected());
}

TEST_F(ut_middleview_test, setMouseState)
{
    MiddleView middleview;

    MiddleView::MouseState mouseState {MiddleView::MouseState::normal};
    middleview.setMouseState(mouseState);
    EXPECT_EQ(middleview.m_mouseState, mouseState);
}

TEST_F(ut_middleview_test, getModifierState)
{
    MiddleView middleview;
    EXPECT_EQ(MiddleView::noModifier, middleview.getModifierState());
}

TEST_F(ut_middleview_test, setModifierState)
{
    MiddleView middleview;
    MiddleView::ModifierState modifierState {MiddleView::ModifierState::noModifier};
    middleview.setModifierState(modifierState);
    EXPECT_EQ(modifierState, middleview.getModifierState());
}

TEST_F(ut_middleview_test, onNoteChanged)
{
    MiddleView middleview;
    middleview.onNoteChanged();
    EXPECT_EQ(middleview.m_shiftSelection, -1);
}

TEST_F(ut_middleview_test, getCurrVNotedataList)
{
    m_middleView->selectAll();
    QList<VNoteItem *> notes = m_middleView->getCurrVNotedataList();
    EXPECT_FALSE(notes.isEmpty());
}

TEST_F(ut_middleview_test, getCurrVNotedata)
{
    m_middleView->setCurrentIndex(0);
    EXPECT_FALSE(m_middleView->getCurrVNotedata() == nullptr);
}

TEST_F(ut_middleview_test, deleteCurrentRow)
{
    int count = m_middleView->rowCount();
    m_middleView->setCurrentIndex(0);
    m_middleView->deleteCurrentRow();
    EXPECT_EQ(m_middleView->rowCount(), count - 1);
}
