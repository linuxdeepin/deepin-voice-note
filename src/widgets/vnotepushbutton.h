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
#ifndef VNOTEPUSHBUTTON_H
#define VNOTEPUSHBUTTON_H

#include <DPushButton>
#include <DPalette>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

class VNotePushbutton : public DPushButton
{
    Q_OBJECT
public:
    explicit VNotePushbutton(QWidget *parent = nullptr);
    ~VNotePushbutton() override {}
protected:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
};
#endif // TOOLBUTTON_H
