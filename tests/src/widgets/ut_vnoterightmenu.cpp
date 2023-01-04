/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     ningyuchuang <ningyuchuang@uniontech.com>
* Maintainer: ningyuchuang <ningyuchuang@uniontech.com>
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

#include "ut_vnoterightmenu.h"
#include "vnoterightmenu.h"
#include "globaldef.h"

UT_VNoteRightMenu::UT_VNoteRightMenu()
{
}

TEST_F(UT_VNoteRightMenu, UT_VNoteRightMenu_mouseMoveEvent_001)
{
    VNoteRightMenu rightmenu;
    QPointF localPos;
    QMouseEvent *mouseMoveEvent = new QMouseEvent(QEvent::MouseMove, localPos, localPos, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSource::MouseEventSynthesizedByQt);
    rightmenu.m_timer->start(100);
    rightmenu.m_moved = true;
    rightmenu.mouseMoveEvent(mouseMoveEvent);
    EXPECT_FALSE(rightmenu.m_timer->isActive());
    delete mouseMoveEvent;
}

TEST_F(UT_VNoteRightMenu, UT_VNoteRightMenu_setPressPoint_001)
{
    VNoteRightMenu rightmenu;
    QPoint pos(10, 10);
    rightmenu.setPressPoint(pos);
    EXPECT_EQ(rightmenu.m_touchPoint, pos);
}
