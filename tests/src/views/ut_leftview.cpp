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
#include "leftview.h"
#include "leftviewdelegate.h"
#include "leftviewsortfilter.h"
#include "vnoteforlder.h"
#include "middleview.h"
#include "vnotedatamanager.h"
#include "globaldef.h"

#include <QLineEdit>
#include <QDateTime>
#include <QMimeData>

ut_leftview_test::ut_leftview_test()
{
}

TEST_F(ut_leftview_test, getNotepadRoot)
{
    LeftView leftview;
    leftview.m_pItemDelegate->handleChangeTheme();
    EXPECT_TRUE(nullptr != leftview.getNotepadRoot());
}

TEST_F(ut_leftview_test, getNotepadRootIndex)
{
    LeftView leftview;
    leftview.getNotepadRootIndex();
    EXPECT_TRUE(leftview.getNotepadRootIndex().isValid());
}

TEST_F(ut_leftview_test, mouseEvent)
{
    LeftView leftview;
    QPointF localPos;
    //mousePressEvent
    leftview.m_onlyCurItemMenuEnable = false;
    QMouseEvent *mousePressEvent = new QMouseEvent(QEvent::MouseButtonPress, localPos, localPos, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    QMouseEvent *mousePressEvent2 = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    leftview.mousePressEvent(mousePressEvent);

    leftview.mousePressEvent(mousePressEvent2);
    leftview.m_onlyCurItemMenuEnable = true;
    leftview.mousePressEvent(mousePressEvent);
    leftview.mousePressEvent(mousePressEvent2);
    delete mousePressEvent;
    delete mousePressEvent2;
    //mouseReleaseEvent
    QMouseEvent *releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    leftview.m_onlyCurItemMenuEnable = false;
    QPoint point = QPoint(leftview.visualRect(leftview.currentIndex()).topLeft().x() + 10, leftview.visualRect(leftview.currentIndex()).topLeft().y() - 10);
    leftview.mouseReleaseEvent(releaseEvent);
    delete releaseEvent;

    //doubleClickEvent
    leftview.m_onlyCurItemMenuEnable = false;
    QMouseEvent *doubleClickEvent = new QMouseEvent(QEvent::MouseButtonDblClick, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    leftview.mouseDoubleClickEvent(doubleClickEvent);
    delete doubleClickEvent;

    //mouseMoveEvent
    leftview.m_onlyCurItemMenuEnable = false;
    QMouseEvent *mouseMoveEvent = new QMouseEvent(QEvent::MouseMove, localPos, localPos, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    leftview.setTouchState(leftview.TouchMoving);
    leftview.mouseMoveEvent(mouseMoveEvent);
    delete mouseMoveEvent;

    QMouseEvent *mouseMoveEvent2 = new QMouseEvent(QEvent::MouseMove, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    leftview.setTouchState(leftview.TouchPressing);
    leftview.mouseMoveEvent(mouseMoveEvent2);
    delete mouseMoveEvent2;
}

TEST_F(ut_leftview_test, setTouchState)
{
    LeftView leftview;
    leftview.setTouchState(leftview.TouchNormal);
    EXPECT_EQ(LeftView::TouchNormal, leftview.m_touchState);
}

TEST_F(ut_leftview_test, doTouchMoveEvent)
{
    LeftView leftview;
    QPointF localPos;
    QMouseEvent *mouseMoveEvent = new QMouseEvent(QEvent::MouseMove, localPos, localPos, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    leftview.m_touchPressPoint = QPoint(localPos.x(), localPos.y() - 11);
    QDateTime time = QDateTime::currentDateTime();
    leftview.m_touchPressStartMs = time.toMSecsSinceEpoch() - 260;
    leftview.setTouchState(leftview.TouchMoving);
    leftview.doTouchMoveEvent(mouseMoveEvent);
    delete mouseMoveEvent;
    QMouseEvent *mouseMoveEvent1 = new QMouseEvent(QEvent::MouseMove, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    leftview.m_touchPressPoint = QPoint(localPos.x(), localPos.y() - 11);
    QDateTime time1 = QDateTime::currentDateTime();
    leftview.m_touchPressStartMs = time.toMSecsSinceEpoch() - 260;
    leftview.setTouchState(leftview.TouchMoving);
    leftview.doTouchMoveEvent(mouseMoveEvent1);
    delete mouseMoveEvent1;
}

TEST_F(ut_leftview_test, handleDragEvent)
{
    LeftView leftview;
    bool isTouch = true;
    leftview.handleDragEvent(isTouch);
    EXPECT_EQ(LeftView::TouchDraging, leftview.m_touchState);
}

TEST_F(ut_leftview_test, dropEvent)
{
    LeftView leftview;
    QPointF localPos;
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(NOTES_DRAG_KEY, QByteArray());
    QDropEvent *event = new QDropEvent(localPos, Qt::MoveAction, mimeData,
                                       Qt::LeftButton, Qt::NoModifier, QEvent::Drop);
    leftview.dropEvent(event);
    delete event;
    delete mimeData;
}

TEST_F(ut_leftview_test, setOnlyCurItemMenuEnable)
{
    LeftView leftview;
    bool enable = false;
    leftview.setOnlyCurItemMenuEnable(enable);
    EXPECT_EQ(enable, leftview.m_onlyCurItemMenuEnable);
}

TEST_F(ut_leftview_test, keyPressEvent)
{
    LeftView leftview;
    leftview.m_onlyCurItemMenuEnable = true;
    QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier, "test");
    leftview.keyPressEvent(event1);
    leftview.m_onlyCurItemMenuEnable = false;
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier, "test2");
    leftview.keyPressEvent(event);
    delete event;
    delete event1;
}

TEST_F(ut_leftview_test, restoreNotepadItem)
{
    LeftView leftview;
    EXPECT_TRUE(!leftview.restoreNotepadItem().isValid());
}

TEST_F(ut_leftview_test, addFolder)
{
    LeftView leftview;
    QModelIndex notepadRootIndex = leftview.getNotepadRootIndex();
    leftview.expand(notepadRootIndex);
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    for (auto it : folders->folders) {
        leftview.addFolder(it);
    }
    leftview.editFolder();
    EXPECT_TRUE(0 != leftview.folderCount());
    leftview.sort();
    int sortNum = 0;
    for (auto it : folders->folders) {
        it->sortNumber = sortNum++;
    }
    leftview.sort();
    EXPECT_FALSE(leftview.grab().isNull());
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
    leftview.setDefaultNotepadItem();
    EXPECT_TRUE(leftview.currentIndex().isValid());
    VNoteFolder *removeFolder = leftview.removeFolder();
    EXPECT_EQ(&folder, removeFolder);
}

TEST_F(ut_leftview_test, closeMenu)
{
    LeftView leftview;
    leftview.m_notepadMenu->setVisible(true);
    leftview.closeMenu();
    EXPECT_TRUE(!leftview.m_notepadMenu->isVisible());
}

TEST_F(ut_leftview_test, closeEditor)
{
    LeftView leftview;
    QWidget *editor = new QWidget;
    leftview.closeEditor(editor, QAbstractItemDelegate::NoHint);
    EXPECT_TRUE(!editor->isVisible());
    delete editor;
}

TEST_F(ut_leftview_test, doMove)
{
    LeftView leftview;
    QModelIndexList indexList;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        for (auto it : folders->folders) {
            leftview.appendFolder(it);
        }
    }
    QModelIndex index = leftview.setDefaultNotepadItem();
    QString sortString = leftview.getFolderSort();
    leftview.doDragMove(index, index.siblingAtRow(index.row() + 1));
    leftview.setFolderSort();
    EXPECT_TRUE(sortString != leftview.getFolderSort());
}

TEST_F(ut_leftview_test, getFolderSort)
{
    LeftView leftview;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        for (auto it : folders->folders) {
            leftview.appendFolder(it);
        }
    }
    EXPECT_TRUE(!leftview.getFolderSort().isEmpty());
    EXPECT_EQ(true, leftview.setFolderSort());
}

TEST_F(ut_leftview_test, setDrawNotesNum)
{
    LeftView leftview;
    QLineEdit *lineedit = new QLineEdit();
    lineedit->setText("test2agrrserfgargafgasrfgargargarsgarefgaehbsrtbhaerfgaertfaerfgaerfarfarfa3qw4taerarfa");

    leftview.m_pItemDelegate->setDrawNotesNum(false);
    leftview.m_pItemDelegate->setDragState(false);
    EXPECT_EQ(false, leftview.m_pItemDelegate->m_draging);
    leftview.m_pItemDelegate->setDrawHover(false);
    EXPECT_EQ(false, leftview.m_pItemDelegate->m_drawHover);
    leftview.m_pItemDelegate->setModelData(lineedit, leftview.m_pDataModel, leftview.currentIndex());
    delete lineedit;
}

TEST_F(ut_leftview_test, eventFilter)
{
    LeftView leftview;
    QFocusEvent event(QEvent::FocusOut);
    leftview.eventFilter(&leftview, &event);
    EXPECT_EQ(false, leftview.m_pItemDelegate->m_tabFocus);
    QFocusEvent event1(QEvent::FocusIn);
    event1.m_reason = Qt::TabFocusReason;
    leftview.eventFilter(&leftview, &event1);
    EXPECT_EQ(true, leftview.m_pItemDelegate->m_tabFocus);
    QEvent event2(QEvent::DragLeave);
    leftview.eventFilter(nullptr, &event2);
    EXPECT_EQ(false, leftview.m_pItemDelegate->m_drawHover);
    QEvent event3(QEvent::DragEnter);
    leftview.eventFilter(nullptr, &event3);
    EXPECT_EQ(true, leftview.m_pItemDelegate->m_drawHover);
}

TEST_F(ut_leftview_test, sort)
{
    LeftView leftview;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        for (auto it : folders->folders) {
            leftview.appendFolder(it);
        }
    }
    leftview.sort();
    EXPECT_TRUE(!leftview.getFolderSort().isEmpty());
}

TEST_F(ut_leftview_test, handleTouchSlideEvent)
{
    LeftView leftview;
    QPoint point(0, 10);
    leftview.handleTouchSlideEvent(10, 5.0, point);
    EXPECT_EQ(point, leftview.m_touchPressPoint);
}

TEST_F(ut_leftview_test, dragEvent)
{
    LeftView leftview;
    QMimeData data;
    QDragEnterEvent event(QPoint(0, 0), Qt::IgnoreAction, &data, Qt::LeftButton, Qt::NoModifier);
    leftview.dragEnterEvent(&event);
    data.setData(NOTEPAD_DRAG_KEY, QByteArray());
    leftview.m_folderDraing = true;
    leftview.dragEnterEvent(&event);
    leftview.dragMoveEvent(&event);
    EXPECT_EQ(true, leftview.m_pItemDelegate->m_draging);
    QDragLeaveEvent event1;
    leftview.dragLeaveEvent(&event1);
    EXPECT_EQ(false, leftview.m_pItemDelegate->m_draging);
}
