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

#include "ut_textnoteedit.h"

#define protected public
#define private public
#include "textnoteedit.h"
#undef protected
#undef private

ut_textnoteedit_test::ut_textnoteedit_test()
{

}

TEST_F(ut_textnoteedit_test, focusInEvent)
{
    TextNoteEdit textnoteedit;
    QFocusEvent* event = new QFocusEvent(QEvent::FocusIn);
    textnoteedit.focusInEvent(event);
}

TEST_F(ut_textnoteedit_test, focusOutEvent)
{
    TextNoteEdit textnoteedit;
    QFocusEvent* event = new QFocusEvent(QEvent::FocusOut);
    textnoteedit.focusOutEvent(event);
}

TEST_F(ut_textnoteedit_test, wheelEvent)
{
    TextNoteEdit textnoteedit;
    QPointF pos;
    QWheelEvent* event = new QWheelEvent(pos, 1, Qt::NoButton, Qt::NoModifier);
    textnoteedit.wheelEvent(event);
}

TEST_F(ut_textnoteedit_test, contextMenuEvent)
{
    TextNoteEdit textnoteedit;
    QPoint pos;
    QContextMenuEvent* event = new QContextMenuEvent(QContextMenuEvent::Mouse, pos, pos, Qt::NoModifier);
    textnoteedit.contextMenuEvent(event);
}

TEST_F(ut_textnoteedit_test, keyPressEvent)
{
    TextNoteEdit textnoteedit;
    QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, 0x43, Qt::AltModifier, "test");
    textnoteedit.keyPressEvent(event);
    QKeyEvent* event1 = new QKeyEvent(QEvent::KeyPress, 0x43, Qt::ControlModifier, "test");
    textnoteedit.keyPressEvent(event1);
    QKeyEvent* event2 = new QKeyEvent(QEvent::KeyPress, 0x01000001, Qt::NoModifier, "test");
    textnoteedit.keyPressEvent(event2);
}

TEST_F(ut_textnoteedit_test, mouseEvent)
{
    TextNoteEdit textnoteedit;
    QPointF localPos;
    QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    textnoteedit.mousePressEvent(event);
    textnoteedit.mouseReleaseEvent(event);
    textnoteedit.mouseMoveEvent(event);
    textnoteedit.mouseDoubleClickEvent(event);
}

TEST_F(ut_textnoteedit_test, clearSelection)
{
    TextNoteEdit textnoteedit;
    textnoteedit.clearSelection();
}

TEST_F(ut_textnoteedit_test, getSelectFragment)
{
    TextNoteEdit textnoteedit;
    textnoteedit.getSelectFragment();
}

TEST_F(ut_textnoteedit_test, hasSelection)
{
    TextNoteEdit textnoteedit;
    textnoteedit.hasSelection();
}

TEST_F(ut_textnoteedit_test, removeSelectText)
{
    TextNoteEdit textnoteedit;
    textnoteedit.removeSelectText();
}
