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

#include "textnoteedit.h"

#include <QWheelEvent>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <QDebug>
TextNoteEdit::TextNoteEdit(QWidget *parent)
    : DTextEdit(parent)
{
    setAlignment(Qt::AlignTop);//设置顶部对其
    setFrameShape(QFrame::NoFrame);//设置无边框
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//隐藏纵滚动条
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//隐藏横滚动条
    DFontSizeManager::instance()->bind(this, DFontSizeManager::T8);//DTK设置字体大小
    setContextMenuPolicy(Qt::NoContextMenu);
    setMouseTracking(true);

    //Edit get focus only by click
    //setFocusPolicy(Qt::ClickFocus);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged,
               this, &TextNoteEdit::onChangeTheme);

    this->installEventFilter(this);

    onChangeTheme();
}

void TextNoteEdit::focusInEvent(QFocusEvent *e)
{
    DTextEdit::focusInEvent(e);
    emit sigFocusIn();
}

void TextNoteEdit::focusOutEvent(QFocusEvent *e)
{
    DTextEdit::focusOutEvent(e);
    if (!m_menuPop) {
        emit sigFocusOut();
    }
}

void TextNoteEdit::wheelEvent(QWheelEvent *e)
{
    e->ignore();
}

void TextNoteEdit::contextMenuEvent(QContextMenuEvent *e)
{
    m_menuPop = true;
    DTextEdit::contextMenuEvent(e);
    m_menuPop = false;
}

void TextNoteEdit::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();

    if(e->modifiers() == Qt::ControlModifier || key == Qt::Key_Delete){
        e->ignore();
        return;
    }

    DTextEdit::keyPressEvent(e);
}

void TextNoteEdit::mousePressEvent(QMouseEvent *event)
{
    DTextEdit::mousePressEvent(event);
    event->ignore();
}

void TextNoteEdit::mouseReleaseEvent(QMouseEvent *event)
{
    DTextEdit::mouseReleaseEvent(event);
    event->ignore();
}

void TextNoteEdit::mouseMoveEvent(QMouseEvent *event)
{
    //DTextEdit::mouseMoveEvent(event);
    event->ignore();
}

void TextNoteEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->ignore();
}

void TextNoteEdit::selectText(const QPoint &globalPos, QTextCursor::MoveOperation op)
{
    QPoint pos = this->mapFromGlobal(globalPos);
    QTextCursor cursor = this->cursorForPosition(pos);
    int curPos = cursor.position();
    this->moveCursor(op);
    cursor = this->textCursor();
    cursor.setPosition(curPos, QTextCursor::KeepAnchor);
    this->setTextCursor(cursor);
}

void TextNoteEdit::clearSelection()
{
    QTextCursor textCursor = this->textCursor();
    if (textCursor.hasSelection()) {
        textCursor.clearSelection();
        this->setTextCursor(textCursor);
    }
}

QString TextNoteEdit::getSelectFragment()
{
    QTextCursor textCursor = this->textCursor();
    return textCursor.selectedText();
}

bool TextNoteEdit::hasSelection()
{
    QTextCursor textCursor = this->textCursor();
    return  textCursor.hasSelection();
}

void TextNoteEdit::removeSelectText()
{
    QTextCursor textCursor = this->textCursor();
    textCursor.removeSelectedText();
}

void TextNoteEdit::onChangeTheme()
{
    DPalette appDp = DApplicationHelper::instance()->applicationPalette();
    DPalette dp = DApplicationHelper::instance()->palette(this);
    dp.setBrush(DPalette::Highlight, appDp.color(DPalette::Normal,DPalette::Highlight));
    dp.setBrush(DPalette::HighlightedText, appDp.color(DPalette::Normal,DPalette::HighlightedText));
    this->setPalette(dp);
}

bool TextNoteEdit::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);
    if(e->type() == QEvent::FontChange){
        QFontMetrics metrics(this->font());
        this->setTabStopWidth(4 * metrics.width(' '));
    }
    return  false;
}
