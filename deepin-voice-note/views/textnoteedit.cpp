#include "textnoteedit.h"

#include <QWheelEvent>

TextNoteEdit::TextNoteEdit(VNoteItem *textNote, VNTextBlock *noteBlock, QWidget *parent)
    : DTextEdit(parent)
    , m_textNode(textNote)
    , m_noteBlock (noteBlock)
{

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
    if (e->key() == Qt::Key_Backspace) {
        if (this->document()->isEmpty()) {
            emit sigDelEmpty();
        }
    }
    DTextEdit::keyPressEvent(e);
}

VNoteItem *TextNoteEdit::getNoteItem()
{
    return  m_textNode;
}
VNTextBlock *TextNoteEdit::getNoteBlock()
{
    return  m_noteBlock;
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

void TextNoteEdit::selectText(const QPoint &globalPos,QTextCursor::MoveOperation op)
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
