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
#include "common/vnoteitem.h"

#include <QPainter>
#include <QPainterPath>
#include <DApplicationHelper>
#include <DLog>

const int shadow[10] = {15,13,11,9,6,5,4,3,2,1};
MoveView::MoveView(QWidget *parent)
    : QWidget(parent)
{
    ////shadow
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground,true);
    setFixedSize(200,56);
}

/**
 * @brief MoveView::setFolder
 * @param folder 记事本数据
 */
void MoveView::setFolder(VNoteFolder *folder)
{
    m_folder = folder;
}

/**
 * @brief MoveView::setNote
 * @param note 笔记数据
 */
void MoveView::setNote(VNoteItem *note)
{
    m_note = note;
}

/**
 * @brief MoveView::paintEvent
 */
void MoveView::paintEvent(QPaintEvent *)
{
    if(m_note){
        setFixedSize(260,56);
    }
    QPainter painter(this);
    ///从系统获取画板
    DPalette pb = DApplicationHelper::instance()->applicationPalette();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform);
    painter.setPen(Qt::NoPen);
    //设置背景颜色
    QColor color;
    color = pb.color(DPalette::Normal,DPalette::Window);
    color.setAlphaF(0.80);
    painter.setBrush(color);
    QPainterPath PainterPath;
    //绘制内部
    PainterPath.addRoundedRect(QRect( 10, 10, width() - 21, height()-19), 7, 7);
    painter.drawPath(PainterPath);
    //绘制边界
    PainterPath.addRoundedRect(QRect( 9, 9, width() - 21, height() - 19), 8, 8);
    painter.setBrush(pb.color(DPalette::Normal,DPalette::FrameShadowBorder));
    painter.drawPath(PainterPath);

    QFontMetrics fontMetrics = painter.fontMetrics();
    painter.setPen(QPen(pb.color(DPalette::Normal, DPalette::Text)));
    //绘制记事本拖动缩略图
    if(m_folder){
        VNoteFolderOper folderOps(m_folder);
        QString strNum = QString::number(folderOps.getNotesCount());
        int numWidth = fontMetrics.width(strNum);
        QRect paintRect = rect();
        int iconSpace = (paintRect.height() - 24) / 2;
        //绘制内部信息
        QRect iconRect(paintRect.left() + 22, paintRect.top() + iconSpace, 24, 24);
        QRect numRect(paintRect.right() - numWidth - 17, paintRect.top(), numWidth, paintRect.height());
        QRect nameRect(iconRect.right() + 12, paintRect.top(),
                       numRect.left() - iconRect.right() - 15, paintRect.height());
        painter.drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, strNum);
        painter.drawPixmap(iconRect, m_folder->UI.icon);
        QString elideText = fontMetrics.elidedText(m_folder->name, Qt::ElideRight, nameRect.width());
        painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
        //绘制笔记拖动缩略图
    }else if (m_note) {
        QRect paintRect = rect();
        paintRect.setLeft(paintRect.left() + 30);
        paintRect.setRight(paintRect.right() - 30);
        QString elideText = fontMetrics.elidedText(m_note->noteTitle, Qt::ElideRight, paintRect.width());
        painter.drawText(paintRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
    }
    //绘制阴影效果
    QPainter painterShadow(this);
    painterShadow.setRenderHint(QPainter::Antialiasing, true);
    QColor colorShadow(0,0,0,50);
    for(int i=0; i<10; i++)
    {
        QPainterPath pathw;
        QPainterPath line;
        pathw.setFillRule(Qt::WindingFill);
        //左上加深
        pathw.addRoundedRect(10-(i/2)-1, 10-(i/2)-1, this->width()-(10-i)*2+3-((i+1-i/2))-3, this->height()-(10-i)*2+3-((i+1-i/2)),8,8);
        //右下加深
        //pathw.addRoundedRect(10-i-1, 10-i-1, this->width()-(10-i)*2+3-(i/2+1), this->height()-(10-i)*2+3-(i/2+1),7,9);
        colorShadow.setAlpha(shadow[i]);
        painterShadow.setPen(colorShadow);
        //绘制外部阴影
        painterShadow.drawPath(pathw);
    }
}
