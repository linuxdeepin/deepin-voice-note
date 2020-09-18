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

#define protected public
#include "vnote2siconbutton.h"
#undef protected

#include "utils.h"

ut_vnote2siconbutton_test::ut_vnote2siconbutton_test()
{

}

TEST_F(ut_vnote2siconbutton_test, isPressed)
{
    VNote2SIconButton vnote2siconbutton("test", "test");
    vnote2siconbutton.isPressed();
    vnote2siconbutton.setCommonIcon(true);
}

TEST_F(ut_vnote2siconbutton_test, mouseReleaseEvent)
{
    VNote2SIconButton vnote2siconbutton("test", "test");
    QPointF localPos;
    QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    vnote2siconbutton.mouseReleaseEvent(event);
}
