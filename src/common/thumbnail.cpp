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

thumbnail::thumbnail(QWidget *parent) :
    QWidget(parent)
{
    initUi();
}

void thumbnail::initUi()
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    setFixedSize(180, 36);

    ImageButton = new QPushButton(this);
    ImageButton->setFixedSize(24, 24);
    ImageButton->setIconSize(QSize(24, 24));
    ImageButton->setFlat(true);
    ImageButton->setFocusPolicy(Qt::NoFocus);
    ImageButton->move(RADIUS - RECT, RADIUS - RECT);

    TextLabel = new DLabel(this);
    TextLabel->setFixedSize(90, 18);
    TextLabel->setAlignment(Qt::AlignLeft);
    TextLabel->setFont(QFont("PingFangSC", 10, QFont::Normal));
    DPalette pa = DApplicationHelper::instance()->palette(TextLabel);
    pa.setColor(DPalette::TextTitle, pa.textTiele().color());
    DApplicationHelper::instance()->setPalette(TextLabel, pa);
    TextLabel->setFocusPolicy(Qt::NoFocus);

    TextLabel1 = new DLabel(this);
    TextLabel1->setFixedSize(20, 18);
    TextLabel1->setAlignment(Qt::AlignRight);
    TextLabel1->setFont(QFont("PingFangSC", 10, QFont::Normal));
    DApplicationHelper::instance()->setPalette(TextLabel1, pa);
    TextLabel1->setFocusPolicy(Qt::NoFocus);

    layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->addWidget(TextLabel);
    layout->addWidget(TextLabel1);
    layout->setContentsMargins(2 * RADIUS, 0, 0, 0);
}

void thumbnail::setupthumbnail(QIcon icon, const QString &text, const QString &text1)
{
    ImageButton->setVisible(true);
    TextLabel1->setVisible(true);
    ImageButton->setIcon(QIcon(icon));
    setFixedSize(180, 36);
    TextLabel1->setFixedSize(20, 18);
    layout->setContentsMargins(2 * RADIUS, 0, 0, 0);
    TextLabel->setText(text);
    TextLabel1->setText(text1);

    int textSize = fontMetrics().width(text);  //字符超长检测
    if(textSize > TEXT_LENGTH){
        QString Elide_text = fontMetrics().elidedText(text, Qt::ElideRight, TEXT_LENGTH);
        TextLabel->setText(Elide_text);
    }
}

void thumbnail::setupthumbnail(const QString &text)
{
    ImageButton->setVisible(false);
    TextLabel1->setVisible(false);
    TextLabel->setText(text);
    TextLabel->setFixedSize(230, 18);
    layout->setContentsMargins(RADIUS, 0, 0, 0);
    setFixedSize(240, 36);
    int textSize = fontMetrics().width(text);  //字符超长检测
    if(textSize > TEXT_LENGTH){
        QString Elide_text = fontMetrics().elidedText(text, Qt::ElideRight, TEXT_LENGTH);
        TextLabel->setText(Elide_text);
    }
}

void thumbnail::setIconSize(int size)
{
    ImageButton->setIconSize(QSize(size, size));
}

void thumbnail::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform);
    painter.setPen(Qt::NoPen);
    QColor color(247, 247, 247);
    color.setAlphaF(0.9);
    painter.setBrush(color);
    QPainterPath PainterPath;
    PainterPath.addRoundedRect(QRect(0, 0, width(), height()), 8, 8);  //Rect
    painter.drawPath(PainterPath);
}
