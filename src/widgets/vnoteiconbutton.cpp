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

#include "widgets/vnoteiconbutton.h"
#include "globaldef.h"

#include <QImageReader>

#include <DLog>

VNoteIconButton::VNoteIconButton(QWidget *parent, QString normal,QString hover, QString press)
    : DIconButton(parent)
{
    m_icons[Normal] = normal;
    m_icons[Hover] = hover;
    m_icons[Press] = press;
    DStyle::setFocusRectVisible(this, false);
    updateIcon();

    //TODO:x
    //    Need update when theme change
    connect(DGuiApplicationHelper::instance(),
                     &DGuiApplicationHelper::paletteTypeChanged,
            this, &VNoteIconButton::onThemeChanged);
}

VNoteIconButton::~VNoteIconButton()
{
}

void VNoteIconButton::setSeparateThemIcons(bool separate)
{
    m_separateThemeIcon = separate;
}

void VNoteIconButton::SetDisableIcon(const QString &disableIcon)
{
    if (!disableIcon.isEmpty()) {
        m_icons[Disabled] = disableIcon;
    }
}

void VNoteIconButton::setBtnDisabled(bool disabled)
{
    //The disable icon should be supplied
    if (!m_icons[Disabled].isEmpty()) {
        m_isDisabled = disabled;

        if (m_isDisabled) {
            m_state = Disabled;
        } else {
            m_state = Normal;
        }

        updateIcon();

        setDisabled(m_isDisabled);
    }
}

void VNoteIconButton::enterEvent(QEvent *event)
{
    Q_UNUSED(event);

    if (!m_isDisabled) {
        m_state = Hover;

        updateIcon();
    }
}

void VNoteIconButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    if (!m_isDisabled) {
        m_state = Normal;

        updateIcon();
    }
}

void VNoteIconButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (!m_isDisabled) {
        m_state = Press;

        updateIcon();

        event->accept();
    }
}

void VNoteIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();

    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (!m_isDisabled) {
        if (rect().contains(event->pos())) {

            m_state = Hover;

            updateIcon();

            Q_EMIT clicked();
        }
    }
}

void VNoteIconButton::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if (!m_isDisabled) {
        if (m_state != Hover) {
            m_state = Hover;

            updateIcon();
        }
    }
}

void VNoteIconButton::onThemeChanged(DGuiApplicationHelper::ColorType type)
{
    Q_UNUSED(type);

    updateIcon();
}

QPixmap VNoteIconButton::loadPixmap(const QString &path)
{
    DGuiApplicationHelper::ColorType theme =
            DGuiApplicationHelper::instance()->themeType();

    QString iconPath(STAND_ICON_PAHT);

    //Default use different icons under
    //different themes
    if (m_separateThemeIcon) {
        if (DGuiApplicationHelper::ColorType::LightType == theme) {
            iconPath += QString("light/");
        } else {
            iconPath += QString("dark/");
        }
    }

    iconPath += path;

    qreal ratio = 1.0;

    const qreal devicePixelRatio = devicePixelRatioF();

    QPixmap pixmap;

    if (!qFuzzyCompare(ratio, devicePixelRatio)) {
        QImageReader reader;
        reader.setFileName(qt_findAtNxFile(iconPath, devicePixelRatio, &ratio));
        if (reader.canRead()) {
            reader.setScaledSize(reader.size() * (devicePixelRatio / ratio));
            pixmap = QPixmap::fromImage(reader.read());
            pixmap.setDevicePixelRatio(devicePixelRatio);
        }
    } else {
        pixmap.load(iconPath);
    }

    return pixmap;
}

void VNoteIconButton::updateIcon()
{
    if (!m_icons[m_state].isEmpty()) {
        setIcon(loadPixmap(m_icons[m_state]));
    }
}
