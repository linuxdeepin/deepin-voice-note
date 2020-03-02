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
