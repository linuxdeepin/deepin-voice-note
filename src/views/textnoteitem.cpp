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

#include "textnoteitem.h"
#include "textnoteedit.h"
#include "common/utils.h"

#include <DStyle>
#include <DApplicationHelper>

#include <QVBoxLayout>
#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QDebug>

/**
 * @brief TextNoteItem::TextNoteItem
 * @param noteBlock 绑定的数据
 * @param parent
 */
TextNoteItem::TextNoteItem(VNoteBlock *noteBlock, QWidget *parent)
    : DetailItemWidget(parent)
    , m_noteBlock(noteBlock)
{
    initUi();
    initConnection();
    Utils::blockToDocument(m_noteBlock, m_textEdit->document());
    onChangeTheme();
}

/**
 * @brief TextNoteItem::initUi
 */
void TextNoteItem::initUi()
{
    m_textEdit = new TextNoteEdit(this);
    m_textEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_textEdit->document()->setDocumentMargin(5);
    DStyle::setFocusRectVisible(m_textEdit, false);
    DPalette pb = DApplicationHelper::instance()->palette(m_textEdit);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    m_textEdit->setPalette(pb);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_textEdit);
    this->setLayout(layout);
}

/**
 * @brief TextNoteItem::initConnection
 */
void TextNoteItem::initConnection()
{
    QTextDocument *document = m_textEdit->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, [=] {
        m_textEdit->setFixedHeight(static_cast<int>(document->size().height()));
        this->setFixedHeight(m_textEdit->height() + 10);
        onTextCursorChange();
    });
    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChange()));
    connect(m_textEdit, SIGNAL(sigFocusIn(Qt::FocusReason)), this, SIGNAL(sigFocusIn(Qt::FocusReason)));
    connect(m_textEdit, SIGNAL(sigFocusOut()), this, SIGNAL(sigFocusOut()));
    connect(m_textEdit, SIGNAL(selectionChanged()), this, SIGNAL(sigSelectionChanged()));
    connect(m_textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(onTextCursorChange()));

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged,
            this, &TextNoteItem::onChangeTheme);
}

/**
 * @brief TextNoteItem::updateSearchKey
 * @param searchKey 搜索关键字
 */
void TextNoteItem::updateSearchKey(QString searchKey)
{
    if (m_noteBlock) {
        if (m_textDocumentUndo == true && m_serchKey == searchKey) {
            return;
        }
        m_serchKey = searchKey;
        m_isSearching = true;
        if (m_textDocumentUndo == false && m_searchCount) {
            Utils::setDefaultColor(m_textEdit->document(), m_textEdit->palette().text().color());
        }
        DPalette pb;
        m_searchCount = Utils::highTextEdit(m_textEdit->document(), m_serchKey, pb.color(DPalette::Highlight), m_textDocumentUndo);
        m_textDocumentUndo = m_searchCount == 0 ? false : true;
        m_isSearching = false;
    }
}

/**
 * @brief TextNoteItem::setTextCursor
 * @param cursor
 */
void TextNoteItem::setTextCursor(const QTextCursor &cursor)
{
    m_textEdit->setTextCursor(cursor);
}

/**
 * @brief TextNoteItem::getTextCursor
 * @return 编辑器光标
 */
QTextCursor TextNoteItem::getTextCursor()
{
    return m_textEdit->textCursor();
}

/**
 * @brief TextNoteItem::getNoteBlock
 * @return 绑定数据
 */
VNoteBlock *TextNoteItem::getNoteBlock()
{
    return m_noteBlock;
}

/**
 * @brief TextNoteItem::textIsEmpty
 * @return true 没有内容
 */
bool TextNoteItem::textIsEmpty()
{
    return m_textEdit->document()->isEmpty();
}

/**
 * @brief TextNoteItem::getCursorRect
 * @return 光标区域
 */
QRect TextNoteItem::getCursorRect()
{
    return m_textEdit->cursorRect(m_textEdit->textCursor());
}

/**
 * @brief TextNoteItem::selectText
 * @param globalPos
 * @param op
 */
void TextNoteItem::selectText(const QPoint &globalPos, QTextCursor::MoveOperation op)
{
    return m_textEdit->selectText(globalPos, op);
}

/**
 * @brief TextNoteItem::selectAllText
 */
void TextNoteItem::selectAllText()
{
    m_textEdit->setSelectAll(true);
}

/**
 * @brief TextNoteItem::selectText
 * @param op
 */
void TextNoteItem::selectText(QTextCursor::MoveOperation op)
{
    return m_textEdit->moveCursor(op, QTextCursor::KeepAnchor);
}

/**
 * @brief TextNoteItem::removeSelectText
 */
void TextNoteItem::removeSelectText()
{
    return m_textEdit->removeSelectText();
}

/**
 * @brief TextNoteItem::clearSelection
 */
void TextNoteItem::clearSelection()
{
    return m_textEdit->clearSelection();
}

/**
 * @brief TextNoteItem::hasSelection
 * @return true 有选中内容
 */
bool TextNoteItem::hasSelection()
{
    return m_textEdit->hasSelection();
}

/**
 * @brief TextNoteItem::getSelectFragment
 * @return 选中片段
 */
QTextDocumentFragment TextNoteItem::getSelectFragment()
{
    return getTextCursor().selection();
}

/**
 * @brief TextNoteItem::setFocus
 */
void TextNoteItem::setFocus(bool hasVoice)
{
    m_textEdit->setSelectVoice(hasVoice);
    return m_textEdit->setFocus();
}

/**
 * @brief TextNoteItem::hasFocus
 * @return true 有焦点
 */
bool TextNoteItem::hasFocus()
{
    return m_textEdit->hasFocus();
}

/**
 * @brief TextNoteItem::isSelectAll
 * @return true 全选
 */
bool TextNoteItem::isSelectAll()
{
    return m_textEdit->isSelectAll();
}

/**
 * @brief TextNoteItem::getTextDocument
 * @return 整个文档管理对象
 */
QTextDocument *TextNoteItem::getTextDocument()
{
    return m_textEdit->document();
}

/**
 * @brief TextNoteItem::pasteText
 */
void TextNoteItem::pasteText()
{
    QClipboard *board = QApplication::clipboard();
    if (board) {
        QString clipBoardText = board->text();
        clipBoardText.replace('\t', ' ');
        m_textEdit->insertPlainText(clipBoardText);
    }
}

/**
 * @brief TextNoteItem::onTextChange
 */
void TextNoteItem::onTextChange()
{
    if (m_isSearching == false) {
        if (m_searchCount) {
            m_textDocumentUndo = false;
        }
        emit sigTextChanged();
    }
}

/**
 * @brief TextNoteItem::onTextCursorChange
 */
void TextNoteItem::onTextCursorChange()
{
    if (this->hasFocus()) {
        QRect rc = m_textEdit->cursorRect();
        int height = rc.top() ? rc.bottom() + 5 : -(rc.bottom() + 5);
        if (m_lastCursorHeight != height) {
            m_lastCursorHeight = height;
            emit sigCursorHeightChange(this, height);
        }
    }
}

/**
 * @brief TextNoteItem::setLastCursorHeight
 * @param height
 */
void TextNoteItem::setLastCursorHeight(int height)
{
    m_lastCursorHeight = height;
}

/**
 * @brief TextNoteItem::onChangeTheme
 */
void TextNoteItem::onChangeTheme()
{
    DPalette appDp = DApplicationHelper::instance()->applicationPalette();
    DPalette dp = DApplicationHelper::instance()->palette(m_textEdit);
    dp.setBrush(DPalette::Highlight, appDp.color(DPalette::Normal, DPalette::Highlight));
    dp.setBrush(DPalette::HighlightedText, appDp.color(DPalette::Normal, DPalette::HighlightedText));
    dp.setBrush(DPalette::Text, appDp.color(DPalette::Active, DPalette::Text));
    m_textEdit->setPalette(dp);

    m_searchCount = 1;
    m_textDocumentUndo = false;
    updateSearchKey(m_serchKey);
}

void TextNoteItem::setSelectVoice(bool flag)
{
    m_textEdit->setSelectVoice(flag);
}
