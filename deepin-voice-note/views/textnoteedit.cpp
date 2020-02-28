#include "textnoteedit.h"

#include <QWheelEvent>

TextNoteEdit::TextNoteEdit(QWidget *parent)
    : DTextEdit(parent)
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
