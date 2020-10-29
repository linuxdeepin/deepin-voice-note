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
#include "thumbnail.h"

#include <QPainter>
#include <DFontSizeManager>

thumbnail::thumbnail(QAbstractItemView *parent) :
    QWidget(parent),
    m_itemView(parent)
{
    initUi();
}

void thumbnail::initUi()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_itemView);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_imageBtn = new DPushButton(this);
    m_imageBtn->setFixedSize(24, 24);
    m_imageBtn->setIconSize(QSize(24, 24));
    m_imageBtn->setFlat(true);
    m_imageBtn->setFocusPolicy(Qt::NoFocus);
    m_imageBtn->move(RADIUS - RECT, RADIUS - RECT);

    m_textLbl = new DLabel(this);
    m_textLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_textLbl->setFocusPolicy(Qt::NoFocus);
    DFontSizeManager::instance()->bind(m_textLbl, DFontSizeManager::T6);

    m_textLbl1 = new DLabel(this);
    m_textLbl1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_textLbl1->setFocusPolicy(Qt::NoFocus);
    DFontSizeManager::instance()->bind(m_textLbl1, DFontSizeManager::T6);

    m_layout = new QHBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_textLbl);
    m_layout->addWidget(m_textLbl1);
    m_layout->setContentsMargins(2 * RADIUS, 0, 0, 0);
}

void thumbnail::setupthumbnail(QIcon icon, const QString &text, const QString &text1)
{
    m_imageBtn->setVisible(true);
    m_textLbl1->setVisible(true);
    m_imageBtn->setIcon(QIcon(icon));
    setFixedSize(180, 36);
    m_textLbl1->setFixedSize(20, 20);
    m_textLbl->setFixedSize(110, 20);
    m_layout->setContentsMargins(2 * RADIUS, 0, 0, 0);
    m_textLbl->setText(text);
    m_textLbl1->setText(text1);

    int textSize = fontMetrics().width(text);  //字符超长检测
    if(textSize > TEXT_LENGTH){
        QString Elide_text = fontMetrics().elidedText(text, Qt::ElideRight, TEXT_LENGTH);
        m_textLbl->setText(Elide_text);
    }
}

void thumbnail::setupthumbnail(const QString &text)
{
    m_imageBtn->setVisible(false);
    m_textLbl1->setVisible(false);
    m_textLbl->setText(text);
    m_textLbl->setFixedSize(240, 18);
    m_layout->setContentsMargins(RADIUS, 0, 0, 0);
    setFixedSize(240, 36);
    int textSize = fontMetrics().width(text);  //字符超长检测
    if(textSize > TEXT_LENGTH){
        QString Elide_text = fontMetrics().elidedText(text, Qt::ElideRight, TEXT_LENGTH);
        m_textLbl->setText(Elide_text);
    }
}

void thumbnail::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform);
    painter.setBrush(QBrush(m_parentPb.color(DPalette::Normal, DPalette::ToolTipBase)));
    QColor coverColor = m_parentPb.color(DPalette::Disabled, DPalette::TextTips);
    coverColor.setAlphaF(0.9);
    painter.setPen(QPen(coverColor));
    QPainterPath PainterPath;
    PainterPath.addRoundedRect(QRect(0, 0, width(), height()), 8, 8);  //Rect
    painter.drawPath(PainterPath);
}
