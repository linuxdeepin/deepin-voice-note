/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co.,Ltd.
 *
 * Author:     He MingYang <hemingyang@uniontech.com>
 *
 * Maintainer: Liu Zheng <liuzheng@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QWidget>
#include "vnotepushbutton.h"
VNotePushbutton::VNotePushbutton(QWidget *parent) : DPushButton (parent) {
    setCheckable(true);
    setFlat(true);
}

void VNotePushbutton::enterEvent(QEvent *e)
{
    if (this->isEnabled()) {
        setFlat(false);
    }
    DPushButton::enterEvent(e);
}

void VNotePushbutton::leaveEvent(QEvent *e)
{
    setFlat(true);
    DPushButton::leaveEvent(e);
}
void VNotePushbutton::mousePressEvent(QMouseEvent *e)
{
    DPushButton::mousePressEvent(e);
}


