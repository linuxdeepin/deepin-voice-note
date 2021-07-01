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

#include "ut_vnoteiconbutton.h"
#include "vnoteiconbutton.h"
#include "globaldef.h"

ut_vnoteiconbutton_test::ut_vnoteiconbutton_test()
{
}

TEST_F(ut_vnoteiconbutton_test, mousePressEvent)
{
    VNoteIconButton vnoteiconbutton(nullptr, "home_page_logo.svg", "home_page_logo.svg", "home_page_logo.svg");
    QPointF localPos;
    QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    vnoteiconbutton.mouseMoveEvent(event);
    vnoteiconbutton.mousePressEvent(event);
    vnoteiconbutton.mouseReleaseEvent(event);
    delete event;
}

TEST_F(ut_vnoteiconbutton_test, setSeparateThemIcons)
{
    VNoteIconButton vnoteiconbutton(nullptr, "home_page_logo.svg", "home_page_logo.svg", "home_page_logo.svg");
    vnoteiconbutton.enterEvent(nullptr);
    vnoteiconbutton.leaveEvent(nullptr);
    vnoteiconbutton.setSeparateThemIcons(false);
    vnoteiconbutton.SetDisableIcon("home_page_logo.svg");
    vnoteiconbutton.setBtnDisabled(false);
    vnoteiconbutton.setBtnDisabled(true);
    vnoteiconbutton.getFocusReason();
    vnoteiconbutton.onThemeChanged(DGuiApplicationHelper::LightType);
}
