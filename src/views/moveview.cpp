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
#include "globaldef.h"

#include <DApplicationHelper>

#include <QPainter>
#include <QPainterPath>

MoveView::MoveView(QWidget *parent)
    : QWidget(parent)
{
    //shadow
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    if (m_folder) {
        setFixedSize(224, 91);
    } else {
        setFixedSize(282, 91);
    }
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
 * @brief MoveView::setNoteList
 * 设置笔记数据列表
 */
void MoveView::setNoteList(QList<VNoteItem *> noteList)
{
    m_noteList = noteList;
}

/**
 * @brief MoveView::paintEvent
 */
void MoveView::paintEvent(QPaintEvent *)
{
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        m_isDarkThemeType = false;
    } else {
        m_isDarkThemeType = true;
    }
    QString imagePath = QString(STAND_ICON_PAHT);
    //深色主题
    if (m_isDarkThemeType) {
        imagePath.append("dark/");
    }
    //浅色主题
    else {
        imagePath.append("light/");
    }
    if (m_folder) {
        m_backGroundPixMap = QPixmap(imagePath.append("drag_notePad.svg"));
    } else {
        if (1 < m_notesNumber) {
            m_backGroundPixMap = QPixmap(imagePath.append("drag_notes.svg"));
        } else if (1 == m_notesNumber) {
            m_backGroundPixMap = QPixmap(imagePath.append("drag_note.svg"));
        }
    }
    QPainter painter(this);
    //从系统获取画板
    DPalette pb = DApplicationHelper::instance()->applicationPalette();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform);
    painter.setPen(Qt::NoPen);
    QFontMetrics fontMetrics = painter.fontMetrics();
    QRect paintRect = rect();
    if (m_folder) {
        //设置透明度
        if (!m_isDarkThemeType) {
            painter.setOpacity(0.9);
        }
        painter.drawPixmap(rect(), m_backGroundPixMap);
        painter.setOpacity(1);
        //设置背景颜色
        QColor color;
        color = pb.color(DPalette::Normal, DPalette::Window);
        color.setAlphaF(0.80);
        painter.setBrush(color);
        QPainterPath PainterPath;

        VNoteFolderOper folderOps(m_folder);
        QString strNum = QString::number(folderOps.getNotesCount());
        int numWidth = fontMetrics.width(strNum);
        int iconSpace = (paintRect.height() - 24) / 2;
        //绘制内部信息
        painter.setPen(QPen(pb.color(DPalette::Normal, DPalette::Text)));
        QRect iconRect(paintRect.left() + 32, paintRect.top() + iconSpace, 24, 24);
        QRect numRect(paintRect.right() - numWidth - 30, paintRect.top(), numWidth, paintRect.height());
        QRect nameRect(iconRect.right() + 12, paintRect.top(),
                       numRect.left() - iconRect.right() - 14, paintRect.height());
        //绘制记事本笔记数量
        painter.drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, strNum);
        //绘制记事本图标
        painter.drawPixmap(iconRect, m_folder->UI.icon);
        //绘制记事本名称
        QString elideText = fontMetrics.elidedText(m_folder->name, Qt::ElideRight, nameRect.width());
        painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
    } else {
        //多个笔记拖拽
        if (m_notesNumber > 1) {
            if (m_isDarkThemeType) {
                painter.setOpacity(0.98);
            }
            //设置透明度
            painter.drawPixmap(rect(), m_backGroundPixMap);
            painter.setOpacity(1);

            QColor color;
            color = pb.color(DPalette::Normal, DPalette::Window);
            painter.setBrush(color);
            painter.setPen(QPen(pb.color(DPalette::Normal, DPalette::Text)));
            paintRect.setLeft(paintRect.left() + 42);
            paintRect.setRight(paintRect.right() - 42);
            //绘制笔记名称
            QString elideText = fontMetrics.elidedText(m_noteList[0]->noteTitle, Qt::ElideRight, paintRect.width());
            painter.drawText(paintRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);

            //字体大小设置成固定大小，以免显示遮挡
            int widths = 0;
            QString numString = "";
            if (99 < m_notesNumber) {
                widths = fontMetrics.width(("..."));
                numString = "...";
            } else {
                widths = fontMetrics.width(QString::number(m_notesNumber));
                numString = QString::number(m_notesNumber);
            }
            //绘制笔记数量背景圆
            QColor color2("#FD5E5E");
            painter.setPen(QPen(color2));
            painter.setBrush(color2);
            if (m_notesNumber < 10) {
                painter.drawEllipse(QPointF(248, 28), 12.5, 12.5);
            } else {
                painter.drawEllipse(QPointF(248, 28), 12.5 + (widths - 7) / 4, 12.5 + (widths - 7) / 4);
            }
            QColor color3("#FFFFFF");
            painter.setPen(QPen(color3));
            painter.setBrush(color3);
            //绘制笔记数量
            QRect numberRect(QPoint(240 - ((widths - 7) / 2), 19), QSize(16 + (widths - 7), 19));
            painter.drawText(numberRect, Qt::AlignCenter, numString);
        }
        //单个笔记拖拽
        else if (1 == m_notesNumber) {
            if (!m_isDarkThemeType) {
                painter.setOpacity(0.9);
            }
            painter.drawPixmap(rect(), m_backGroundPixMap);
            painter.setOpacity(1);
            QColor color;
            color = pb.color(DPalette::Normal, DPalette::Window);
            painter.setBrush(color);
            painter.setPen(QPen(pb.color(DPalette::Normal, DPalette::Text)));
            paintRect.setLeft(paintRect.left() + 42);
            paintRect.setRight(paintRect.right() - 42);
            //绘制笔记名称
            QString elideText = fontMetrics.elidedText(m_noteList[0]->noteTitle, Qt::ElideRight, paintRect.width());
            painter.drawText(paintRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
        }
    }
}

/**
 * @brief MiddleView::mouseDoubleClickEvent
 * @param event
 * 设置笔记数量
 */
void MoveView::setNotesNumber(int value)
{
    m_notesNumber = value;
}
