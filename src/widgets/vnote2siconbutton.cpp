/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
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

#include "vnote2siconbutton.h"
#include "common/utils.h"

/**
 * @brief VNote2SIconButton::VNote2SIconButton
 * @param normal 图标路径
 * @param press 图标路径
 * @param parent
 */
VNote2SIconButton::VNote2SIconButton(
    const QString normal, const QString press, QWidget *parent)
    : DFloatingButton(parent)
{
    m_icons[Normal] = normal;
    m_icons[Press] = press;

    updateIcon();
}
/**
 * @brief VNote2SIconButton::isPressed
 * @return true 显示press模式图标
 */
bool VNote2SIconButton::isPressed() const
{
    return (Press == m_state);
}

/**
 * @brief VNote2SIconButton::setState
 * @param state
 */
void VNote2SIconButton::setState(int state)
{
    if (m_state > Invalid && state < MaxState) {
        m_state = state;

        updateIcon();
    }
}

/**
 * @brief VNote2SIconButton::mouseReleaseEvent
 * @param event
 */
void VNote2SIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()) && isEnabled()) {
        if (Normal == m_state) {
            m_state = Press;
        } else {
            m_state = Normal;
        }

        updateIcon();
    }

    DIconButton::mouseReleaseEvent(event);
}

/**
 * @brief VNote2SIconButton::updateIcon
 */
void VNote2SIconButton::updateIcon()
{
    if (!m_icons[m_state].isEmpty()) {
        setIcon(Utils::loadSVG(m_icons[m_state], m_useCommonIcons));
    }
}

/**
 * @brief VNote2SIconButton::setCommonIcon
 * @param isCommon true 图标不分主题
 */
void VNote2SIconButton::setCommonIcon(bool isCommon)
{
    m_useCommonIcons = isCommon;
}

/**
 * @brief VNote2SIconButton::keyPressEvent
 * @param e
 */
void VNote2SIconButton::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        if (Normal == m_state) {
            m_state = Press;
        } else {
            m_state = Normal;
        }

        updateIcon();
    }
    DFloatingButton::keyPressEvent(e);
}
