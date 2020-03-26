#include "textnoteitem.h"
#include "textnoteedit.h"
#include "common/utils.h"

#include <QVBoxLayout>
#include <QAbstractTextDocumentLayout>

#include <DStyle>
#include <DApplicationHelper>

TextNoteItem::TextNoteItem(VNoteBlock *noteBlock, QWidget *parent, QRegExp reg)
    : DetailItemWidget(parent)
    , m_noteBlock(noteBlock)
    , m_serchKey(reg)
{
    initUi();
    initConnection();
    updateData();
}

void TextNoteItem::initUi()
{
    m_textEdit = new TextNoteEdit(this);
    m_textEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_textEdit->document()->setDocumentMargin(0);
    m_textCharFormat = m_textEdit->currentCharFormat();
    DStyle::setFocusRectVisible(m_textEdit, false);
    DPalette pb = DApplicationHelper::instance()->palette(m_textEdit);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    m_textEdit->setPalette(pb);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_textEdit);
    this->setLayout(layout);
}

void TextNoteItem::initConnection()
{
    QTextDocument *document = m_textEdit->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, [ = ] {
        m_textEdit->setFixedHeight(static_cast<int>(document->size().height()));
        int height = m_textEdit->cursorRect(m_textEdit->textCursor()).bottom();
        this->setFixedHeight(m_textEdit->height());
        emit sigCursorHeightChange(this, height);
    });
    connect(m_textEdit, SIGNAL(textChanged()), this, SIGNAL(sigTextChanged()));
    connect(m_textEdit, SIGNAL(sigFocusIn()), this, SIGNAL(sigFocusIn()));
    connect(m_textEdit, SIGNAL(sigFocusOut()), this, SIGNAL(sigFocusOut()));
    connect(m_textEdit, SIGNAL(sigDelEmpty()), this, SIGNAL(sigDelEmpty()));
}

void TextNoteItem::updateData()
{
    if (m_noteBlock) {
        m_textEdit->setPlainText(m_noteBlock->blockText);
        if (!m_serchKey.isEmpty()) {
            DPalette pb;
            Utils::highTextEdit(m_textEdit, m_textCharFormat, m_serchKey, pb.color(DPalette::Highlight));
        }
    }
}

void TextNoteItem::setTextCursor(const QTextCursor &cursor)
{
    m_textEdit->setTextCursor(cursor);
}

QTextCursor TextNoteItem::getTextCursor()
{
    return  m_textEdit->textCursor();
}

VNoteBlock *TextNoteItem::getNoteBlock()
{
    return  m_noteBlock;
}

bool TextNoteItem::textIsEmpty()
{
    return  m_textEdit->document()->isEmpty();
}

QRect TextNoteItem::getCursorRect()
{
    return m_textEdit->cursorRect(m_textEdit->textCursor());
}

void TextNoteItem::selectText(const QPoint &globalPos, QTextCursor::MoveOperation op)
{
    return m_textEdit->selectText(globalPos, op);
}

void TextNoteItem::selectAllText()
{
    if(textIsEmpty()){
        m_select = true;
    }else {
        m_textEdit->selectAll();
    }
}

void TextNoteItem::selectText(QTextCursor::MoveOperation op)
{
    return m_textEdit->moveCursor(op, QTextCursor::KeepAnchor);
}

void TextNoteItem::removeSelectText()
{
    return m_textEdit->removeSelectText();
}

void TextNoteItem::clearSelection()
{
    m_select = false;
    return m_textEdit->clearSelection();
}

bool TextNoteItem::hasSelection()
{
    return m_select || m_textEdit->hasSelection();
}

QString TextNoteItem::getAllText()
{
    return  m_textEdit->toPlainText();
}

QString TextNoteItem::getSelectText()
{
    return  m_textEdit->getSelectText();
}

void TextNoteItem::setFocus()
{
    return m_textEdit->setFocus();
}

bool TextNoteItem::hasFocus()
{
    return  m_textEdit->hasFocus();
}