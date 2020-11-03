/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#include "moveview.h"
#include "db/vnotefolderoper.h"
#include "common/vnoteforlder.h"

#include <QPainter>
#include <QPainterPath>
#include <DApplicationHelper>
#include <DLog>

MoveView::MoveView(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(180, 36);
}

void MoveView::setFolder(VNoteFolder *folder)
{
    m_folder = folder;
}

void MoveView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    DPalette pb;
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform);
    painter.setPen(Qt::NoPen);
    QColor color;
    color = pb.color(DPalette::Normal,DPalette::ToolTipBase);
    color.setAlphaF(0.9);
    painter.setBrush(color);
    QPainterPath PainterPath;
    PainterPath.addRoundedRect(QRect(0, 0, width(), height()), 8, 8);
    painter.drawPath(PainterPath);
    if(m_folder){
        VNoteFolderOper folderOps(m_folder);
        QFontMetrics fontMetrics = painter.fontMetrics();
        QString strNum = QString::number(folderOps.getNotesCount());
        int numWidth = fontMetrics.width(strNum);
        QRect paintRect = rect();
        int iconSpace = (paintRect.height() - 24) / 2;
        QRect iconRect(paintRect.left() + 11, paintRect.top() + iconSpace, 24, 24);
        QRect numRect(paintRect.right() - numWidth - 7, paintRect.top(), numWidth, paintRect.height());
        QRect nameRect(iconRect.right() + 12, paintRect.top(),
                       numRect.left() - iconRect.right() - 15, paintRect.height());
        painter.setPen(QPen(pb.color(DPalette::Normal, DPalette::Text)));
        painter.drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, strNum);
        painter.drawPixmap(iconRect, m_folder->UI.icon);
        QString elideText = fontMetrics.elidedText(m_folder->name, Qt::ElideRight, nameRect.width());
        painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
    }

}
