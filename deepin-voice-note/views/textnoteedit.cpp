#include "textnoteedit.h"

#include <QScrollBar>
#include <QWheelEvent>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <DApplicationHelper>
#include <DLog>

TextNoteEdit::TextNoteEdit(QWidget *parent)
    : DTextEdit(parent)
{
    QTextDocument *document = this->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();

    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &TextNoteEdit::onDocumentSizeChanged);
    connect(document,&QTextDocument::blockCountChanged,this,&TextNoteEdit::onAdjustDocument);
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

void TextNoteEdit::setDocMargin(const QMargins &margins)
{
    m_docMargins = margins;
}

void TextNoteEdit::setLineHeight(int height)
{
   m_lineHeight = height;
}

void TextNoteEdit::onAdjustDocument()
{
    if(m_docMargins.isNull() && m_lineHeight <= 0){
        return;
    }
    QTextDocument *document = this->document();
    for (QTextBlock it = document->begin() ; it != document->end(); it = it.next()){
        QTextCursor textCursor(it);
        QTextBlockFormat textBlockFormat = it.blockFormat();
        bool setFlag = false;
        if(m_lineHeight > 0){
            int height = static_cast<int>(textBlockFormat.lineHeight());
            if(height != m_lineHeight){
                textBlockFormat.setLineHeight(m_lineHeight, QTextBlockFormat::FixedHeight);
                setFlag = true;
            }
        }
        if(!m_docMargins.isNull()) {
            int topMargin = static_cast<int>(textBlockFormat.topMargin());
            if(topMargin != m_docMargins.top()){
                textBlockFormat.setTopMargin(m_docMargins.top());
                setFlag = true;
            }
            int leftMargin = static_cast<int>(textBlockFormat.leftMargin());
            if(leftMargin != m_docMargins.left()){
                textBlockFormat.setLeftMargin(m_docMargins.left());
                setFlag = true;
            }
            int rightMargin = static_cast<int>(textBlockFormat.rightMargin());
            if(rightMargin != m_docMargins.right()){
                textBlockFormat.setRightMargin(m_docMargins.right());
                setFlag = true;
            }
            int bottomMargin = static_cast<int>(textBlockFormat.bottomMargin());
            if(bottomMargin != m_docMargins.bottom()){
                textBlockFormat.setBottomMargin(m_docMargins.bottom());
                setFlag = true;
            }
        }
        if(setFlag){
            textCursor.setBlockFormat(textBlockFormat);
        }
    }
}

 void TextNoteEdit::insertFromMimeData(const QMimeData *source)
 {
     DTextEdit::insertFromMimeData(source);
     onAdjustDocument();
 }

 void TextNoteEdit::setPlainText(const QString &text)
 {
     DTextEdit::setPlainText(text);
     onAdjustDocument();
 }

 void TextNoteEdit::onDocumentSizeChanged()
 {
     emit sigDocumentSizeChange();
 }
