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

#include "textnoteedit.h"

#include <DFontSizeManager>
#include <DApplicationHelper>

#include <QWheelEvent>
#include <QTextBlock>

/**
 * @brief TextNoteEdit::TextNoteEdit
 * @param parent
 */
TextNoteEdit::TextNoteEdit(QWidget *parent)
    : DTextEdit(parent)
{
    setAlignment(Qt::AlignTop); //设置顶部对其
    setFrameShape(QFrame::NoFrame); //设置无边框
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏纵滚动条
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏横滚动条
    DFontSizeManager::instance()->bind(this, DFontSizeManager::T8); //DTK设置字体大小
    setContextMenuPolicy(Qt::NoContextMenu);
    setMouseTracking(true);
}

/**
 * @brief TextNoteEdit::focusInEvent
 * @param e
 */
void TextNoteEdit::focusInEvent(QFocusEvent *e)
{
    DTextEdit::focusInEvent(e);
    emit sigFocusIn(e->reason());
}

/**
 * @brief TextNoteEdit::focusOutEvent
 * @param e
 */
void TextNoteEdit::focusOutEvent(QFocusEvent *e)
{
    DTextEdit::focusOutEvent(e);
    if (!m_menuPop) {
        emit sigFocusOut();
    }
}

/**
 * @brief TextNoteEdit::wheelEvent
 * @param e
 */
void TextNoteEdit::wheelEvent(QWheelEvent *e)
{
    e->ignore();
}

/**
 * @brief TextNoteEdit::contextMenuEvent
 * @param e
 */
void TextNoteEdit::contextMenuEvent(QContextMenuEvent *e)
{
    m_menuPop = true;
    DTextEdit::contextMenuEvent(e);
    m_menuPop = false;
}

/**
 * @brief TextNoteEdit::keyPressEvent
 * @param e
 */
void TextNoteEdit::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();

    if (this->hasSelectVoice()) {
        if (key == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
            return;
        }
        e->ignore();
        return;
    }

    if (e->modifiers() == Qt::ControlModifier) {
        e->ignore();
        return;
    }

    if (key == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
        indentText();
        return;
    }

    DTextEdit::keyPressEvent(e);
}

/**
 * @brief TextNoteEdit::mousePressEvent
 * @param event
 */
void TextNoteEdit::mousePressEvent(QMouseEvent *event)
{
    DTextEdit::mousePressEvent(event);
    event->ignore();
}

/**
 * @brief TextNoteEdit::mouseReleaseEvent
 * @param event
 */
void TextNoteEdit::mouseReleaseEvent(QMouseEvent *event)
{
    DTextEdit::mouseReleaseEvent(event);
    event->ignore();
}

/**
 * @brief TextNoteEdit::mouseMoveEvent
 * @param event
 */
void TextNoteEdit::mouseMoveEvent(QMouseEvent *event)
{
    //DTextEdit::mouseMoveEvent(event);
    event->ignore();
}

/**
 * @brief TextNoteEdit::mouseDoubleClickEvent
 * @param event
 */
void TextNoteEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->ignore();
}

/**
 * @brief TextNoteEdit::selectText
 * @param globalPos 全局光标坐标
 * @param op 文本光标移动方式
 */
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

/**
 * @brief TextNoteEdit::clearSelection
 */
void TextNoteEdit::clearSelection()
{
    setSelectAll(false);
    setSelectVoice(false);
    QTextCursor textCursor = this->textCursor();
    if (textCursor.hasSelection()) {
        textCursor.clearSelection();
        this->setTextCursor(textCursor);
    }
}

/**
 * @brief TextNoteEdit::getSelectFragment
 * @return 选中的片段
 */
QString TextNoteEdit::getSelectFragment()
{
    QTextCursor textCursor = this->textCursor();
    return textCursor.selectedText();
}

/**
 * @brief TextNoteEdit::hasSelection
 * @return true 有选中
 */
bool TextNoteEdit::hasSelection()
{
    if (m_isSelectAll) {
        return true;
    }
    QTextCursor textCursor = this->textCursor();
    return textCursor.hasSelection();
}

/**
 * @brief TextNoteEdit::removeSelectText
 */
void TextNoteEdit::removeSelectText()
{
    QTextCursor textCursor = this->textCursor();
    textCursor.removeSelectedText();
}

/**
 * @brief TextNoteEdit::indentText
 */
void TextNoteEdit::indentText()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    int indent = 4 - (cursor.positionInBlock() % 4);
    QString spaces(indent, ' ');
    cursor.insertText(spaces);
    cursor.endEditBlock();
}

void TextNoteEdit::setSelectVoice(bool flag)
{
    m_hasSelectVoice = flag;
    setAttribute(Qt::WA_InputMethodEnabled, !m_hasSelectVoice);
}

void TextNoteEdit::setSelectAll(bool flag)
{
    m_isSelectAll = flag;
    if (flag && !document()->isEmpty()) {
        selectAll();
    }
}

bool TextNoteEdit::isSelectAll()
{
    return m_isSelectAll;
}

bool TextNoteEdit::hasSelectVoice()
{
    return m_hasSelectVoice;
}
