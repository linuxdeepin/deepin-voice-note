#include "textnoteedit.h"

TextNoteEdit::TextNoteEdit(QWidget *parent)
    : DTextEdit(parent)
{
}

void TextNoteEdit::focusInEvent(QFocusEvent *e)
{
    emit sigFocusIn();
    DTextEdit::focusInEvent(e);
}

void TextNoteEdit::focusOutEvent(QFocusEvent *e)
{
    emit sigFocusOut();
    DTextEdit::focusOutEvent(e);
}
