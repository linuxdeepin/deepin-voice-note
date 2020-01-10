#include "textnoteedit.h"

#include <QScrollBar>
#include <QWheelEvent>

#include <DApplicationHelper>
#include <DLog>

TextNoteEdit::TextNoteEdit(QWidget *parent)
    : DTextEdit(parent)
{
   ;
}

void TextNoteEdit::focusInEvent(QFocusEvent *e)
{
    DTextEdit::focusInEvent(e);
    emit sigFocusIn();
}

void TextNoteEdit::focusOutEvent(QFocusEvent *e)
{
    DTextEdit::focusOutEvent(e);
    if(!m_menuPop){
        QScrollBar *bar = this->verticalScrollBar();
        bar->setValue(bar->minimum());
        emit sigFocusOut();
    }
}

void TextNoteEdit::wheelEvent(QWheelEvent *e)
{
    if (this->hasFocus()) {
        QScrollBar *scrollbar = this->verticalScrollBar();
        if (nullptr != scrollbar) {
            int position = scrollbar->value();

            position -= e->delta();
            scrollbar->setValue(position);
        }

        e->accept();
    }
}

void TextNoteEdit::contextMenuEvent(QContextMenuEvent *e)
{
    m_menuPop = true;
    DTextEdit::contextMenuEvent(e);
    m_menuPop = false;
}
