/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     ningyuchuang <ningyuchuang@uniontech.com>
*
* Maintainer: ningyuchuang <ningyuchuang@uniontech.com>
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

#pragma once

#include <DMenu>

#include <QMouseEvent>
#include <QDebug>
#include <QTimer>

DWIDGET_USE_NAMESPACE
class VNoteRightMenu : public DMenu
{
    Q_OBJECT
public:
    explicit VNoteRightMenu(QWidget *parent = nullptr);
    ~VNoteRightMenu() override;
    void setPressPoint(QPoint point);

protected:
    //初始化信号槽连接
    void initConnections();
    //处理鼠标move事件
    void mouseMoveEvent(QMouseEvent *eve) override;
    //处理鼠标release事件
    void mouseReleaseEvent(QMouseEvent *eve) override;

private:
    QPoint m_touchPoint;
    bool m_moved {false};
    QTimer *m_timer;
signals:
    //触摸移动信号
    void menuTouchMoved(bool isTouch);
    //触摸释放信号
    void menuTouchReleased();
private slots:
};
