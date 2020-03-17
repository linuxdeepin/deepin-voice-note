#include "textnoteedit.h"

#include <QWheelEvent>
#include <DFontSizeManager>

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
    if(e->modifiers() != Qt::ControlModifier){
       DTextEdit::keyPressEvent(e);
    }

    if (e->key() == Qt::Key_Backspace) {
        if (this->document()->isEmpty()) {
            emit sigDelEmpty();
        }
    }

    e->ignore();
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

QString TextNoteEdit::getSelectText()
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

