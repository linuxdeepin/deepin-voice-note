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

#include "ut_vnote2siconbutton.h"
#include "vnote2siconbutton.h"
#include "utils.h"

UT_VNote2SIconButton::UT_VNote2SIconButton()
{
}

TEST_F(UT_VNote2SIconButton, isPressed)
{
    VNote2SIconButton vnote2siconbutton("test", "test");
    vnote2siconbutton.isPressed();
    vnote2siconbutton.setCommonIcon(true);
    EXPECT_EQ(vnote2siconbutton.m_useCommonIcons, true);
}

TEST_F(UT_VNote2SIconButton, UT_VNote2SIconButton_mouseReleaseEvent_001)
{
    VNote2SIconButton vnote2siconbutton("test", "test");
    QPointF localPos;
    QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    int status = vnote2siconbutton.m_state;
    vnote2siconbutton.mouseReleaseEvent(event);
    EXPECT_TRUE(status != vnote2siconbutton.m_state);
    delete event;
}

TEST_F(UT_VNote2SIconButton, UT_VNote2SIconButton_keyPressEvent_001)
{
    VNote2SIconButton vnote2siconbutton("test", "test");
    int status = vnote2siconbutton.m_state;
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    vnote2siconbutton.keyPressEvent(&e);
    EXPECT_TRUE(status != vnote2siconbutton.m_state);
}
