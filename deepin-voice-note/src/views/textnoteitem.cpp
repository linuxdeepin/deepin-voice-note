#include "textnoteitem.h"
#include "textnoteedit.h"
#include "common/utils.h"

#include <QVBoxLayout>
#include <QAbstractTextDocumentLayout>
#include <QClipboard>

#include <DStyle>
#include <DApplicationHelper>

TextNoteItem::TextNoteItem(VNoteBlock *noteBlock, QWidget *parent, QString reg)
    : DetailItemWidget(parent)
    , m_noteBlock(noteBlock)
    , m_serchKey(reg)
{
    initUi();
    initConnection();
    Utils::blockToDocument(m_noteBlock,m_textEdit->document());
    updateSearchKey(m_serchKey);
    m_textEdit->moveCursor(QTextCursor::Start);
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
    layout->setContentsMargins(0, 5, 0, 5);
    layout->addWidget(m_textEdit);
    this->setLayout(layout);
}

void TextNoteItem::initConnection()
{
    QTextDocument *document = m_textEdit->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, [ = ] {
        m_textEdit->setFixedHeight(static_cast<int>(document->size().height()));
        int height = m_textEdit->cursorRect(m_textEdit->textCursor()).bottom() + 5;
        this->setFixedHeight(m_textEdit->height() + 10);
        if (this->hasFocus())
        {
            emit sigCursorHeightChange(this, height);
        }
    });
    connect(m_textEdit, SIGNAL(textChanged()), this, SIGNAL(sigTextChanged()));
    connect(m_textEdit, SIGNAL(sigFocusIn()), this, SIGNAL(sigFocusIn()));
    connect(m_textEdit, SIGNAL(sigFocusOut()), this, SIGNAL(sigFocusOut()));
    connect(m_textEdit, SIGNAL(selectionChanged()), this, SIGNAL(sigSelectionChanged()));
}

void TextNoteItem::updateSearchKey(QString searchKey)
{
    if (m_noteBlock) {
        m_serchKey = searchKey;
        if(m_searchCount != 0){
            Utils::setDefaultColor(m_textEdit->document(), m_textCharFormat.foreground().color());
        }
        if (!m_serchKey.isEmpty()) {
            DPalette pb;
            m_searchCount = Utils::highTextEdit(m_textEdit, m_textCharFormat, m_serchKey, pb.color(DPalette::Highlight));
        }else {
            m_searchCount = 0;
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
    if(m_selectAll == false){
        if (!textIsEmpty()) {
            m_textEdit->selectAll();
        }
        m_selectAll = true;
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
    m_selectAll = false;
    return m_textEdit->clearSelection();
}

bool TextNoteItem::hasSelection()
{
    return m_selectAll || m_textEdit->hasSelection();
}

QTextDocumentFragment TextNoteItem::getSelectFragment()
{
    return  getTextCursor().selection();
}

void TextNoteItem::setFocus()
{
    return m_textEdit->setFocus();
}

bool TextNoteItem::hasFocus()
{
    return  m_textEdit->hasFocus();
}

bool TextNoteItem::isSelectAll()
{
    return m_selectAll;
}

QTextDocument* TextNoteItem::getTextDocument()
{
    return m_textEdit->document();
}

void TextNoteItem::pasteText()
{
    QClipboard *board = QApplication::clipboard();
    if (board) {
        QString clipBoardText = board->text();
        m_textEdit->insertPlainText(clipBoardText);
    }
}
