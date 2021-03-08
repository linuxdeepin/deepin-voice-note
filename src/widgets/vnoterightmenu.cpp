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

#include "vnoterightmenu.h"

VNoteRightMenu::VNoteRightMenu(QWidget *parent)
    : DMenu(parent)
    , m_timer(new QTimer(this))
{
    initConnections();
}

VNoteRightMenu::~VNoteRightMenu()
{
}

/**
 * @brief VNoteRightMenu::initConnections 初始化信号槽连接
 * @param
 */
void VNoteRightMenu::initConnections()
{
    connect(m_timer, &QTimer::timeout, this, [=] {
        int distY = QCursor::pos().y() - m_touchPoint.y();
        int distX = QCursor::pos().x() - m_touchPoint.x();
        if ((qAbs(distX) > 8) || (qAbs(distY) > 8))
            m_moved = true;
        else {
            m_moved = false;
        }
    });
    connect(this, &VNoteRightMenu::aboutToShow, this, [=] {
        m_timer->start(50);
    });
    connect(this, &VNoteRightMenu::aboutToHide, this, [=] {
        m_timer->stop();
    });
}

/**
 * @brief VNoteRightMenu::mouseMoveEvent 处理鼠标move事件
 * @eve 事件
 */
void VNoteRightMenu::mouseMoveEvent(QMouseEvent *eve)
{
    if (eve->source() == Qt::MouseEventSynthesizedByQt) {
        if (m_moved) {
            m_timer->stop();
            emit menuTouchMoved(true);
        }
    }
    DMenu::mouseMoveEvent(eve);
}

/**
 * @brief VNoteRightMenu::mouseReleaseEvent 处理鼠标release事件
 * @eve 事件
 */
void VNoteRightMenu::mouseReleaseEvent(QMouseEvent *eve)
{
    if (eve->source() == Qt::MouseEventSynthesizedByQt) {
        emit menuTouchReleased();
        m_timer->stop();
    }
    return DMenu::mouseReleaseEvent(eve);
}

/**
 * @brief VNoteRightMenu::setPressPointY 设置菜单弹出位置
 * @eve 位置
 */
void VNoteRightMenu::setPressPoint(QPoint point)
{
    m_touchPoint = point;
}
