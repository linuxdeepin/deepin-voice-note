#include "rightview.h"
#include "textnoteedit.h"
#include "voicenoteitem.h"

#include "common/vnoteitem.h"
#include "common/vnotedatamanager.h"
#include "common/actionmanager.h"

#include "db/vnoteitemoper.h"

#include <QBoxLayout>
#include <QDebug>
#include <QStandardPaths>
#include <QTimer>
#include <QScrollBar>
#include <QScrollArea>
#include <QAbstractTextDocumentLayout>
#include <QList>

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DFileDialog>
#include <DMessageBox>
#include <DApplication>
#include <DStyle>

#define PlaceholderWidget "placeholder"
#define VoiceWidget       "voiceitem"
#define TextEditWidget    "textedititem"

RightView::RightView(QWidget *parent)
    : QWidget(parent)
{
    initUi();
}

void RightView::initUi()
{
    m_viewportScrollArea = new QScrollArea(this);
    m_viewportScrollArea->setLineWidth(0);
    m_viewportLayout = new QVBoxLayout;
    m_viewportLayout->setSpacing(0);
    m_viewportLayout->setContentsMargins(20, 0, 20, 0);
    m_viewportWidget = new DWidget(this);
    m_viewportScrollArea->setWidgetResizable(true);
    m_viewportScrollArea->setWidget(m_viewportWidget);
    m_viewportWidget->setLayout(m_viewportLayout);
    m_placeholderWidget = new DWidget(m_viewportWidget); //占位的
    m_placeholderWidget->setObjectName(PlaceholderWidget);
    m_placeholderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_viewportLayout->addWidget(m_placeholderWidget, 1, Qt::AlignBottom);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_viewportScrollArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(mainLayout);
}

QWidget *RightView::insertTextEdit(VNoteBlock *data, bool focus)
{
    TextNoteEdit *editItem = new TextNoteEdit(m_noteItemData, static_cast<VNTextBlock *>(data), m_viewportWidget);
    DStyle::setFocusRectVisible(editItem, false);
    DPalette pb = DApplicationHelper::instance()->palette(editItem);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    //editItem->setPalette(pb);

    editItem->setPlainText(data->blockText);
    editItem->setObjectName(TextEditWidget);
    editItem->document()->setDocumentMargin(0);
    editItem->setFixedHeight(24);
    QTextDocument *document = editItem->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, [ = ] {
        editItem->setFixedHeight(static_cast<int>(document->size().height()));
    });
    connect(editItem, &TextNoteEdit::sigFocusIn, this, &RightView::onTextEditFocusIn);
    connect(editItem, &TextNoteEdit::sigFocusOut, this, &RightView::onTextEditFocusOut);
    connect(editItem, &TextNoteEdit::sigDelEmpty, this, &RightView::onTextEditDelEmpty);
    connect(editItem, &TextNoteEdit::textChanged, this, &RightView::onTextEditTextChange);
    int index = 0;
    if (m_curItemWidget != nullptr) {
        index = m_viewportLayout->indexOf(m_curItemWidget) + 1;
    }
    m_viewportLayout->insertWidget(index, editItem);
    if (focus) {
        QTextCursor cursor = editItem->textCursor();
        cursor.movePosition(QTextCursor::End);
        editItem->setTextCursor(cursor);
        editItem->setFocus();
    }
    return  editItem;
}

QWidget *RightView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNVoiceBlock *data = static_cast<VNVoiceBlock *>(m_noteItemData->datas.newBlock(VNoteBlock::Voice));
    static int i = 0;
    data->voiceSize = voiceSize;
    data->voicePath = voicePath;
    data->createTime = QDateTime::currentDateTime();
    data->voiceTitle = DApplication::translate("RightView", "NewVoice") + QString::number(i++);
    VoiceNoteItem *item = new VoiceNoteItem(m_noteItemData, static_cast<VNVoiceBlock *>(data), this);
    item->setObjectName(VoiceWidget);
    connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
    connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
    connect(item, &VoiceNoteItem::sigAction, this, &RightView::onVoiceMenu);

    if (m_curItemWidget == nullptr) {
        m_viewportLayout->insertWidget(0, item);
        m_curItemWidget = item;
        VNoteBlock *textBlock = m_noteItemData->datas.newBlock(VNoteBlock::Text);
        textBlock->blockText = "";
        m_curItemWidget = insertTextEdit(textBlock, true);
        m_noteItemData->datas.addBlock(nullptr, textBlock);
        m_fIsNoteModified = true;
        saveNote();
    } else {
        QString cutStr = "";
        QString objName = m_curItemWidget->objectName();
        int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
        if (objName == TextEditWidget) {
            TextNoteEdit *itemWidget = static_cast<TextNoteEdit *>(m_curItemWidget);
            if (itemWidget->document()->isEmpty()) {
                m_viewportLayout->insertWidget(curIndex, item);
                VNoteBlock *preData = nullptr;
                if (curIndex > 0) {
                    QWidget *preWidget = m_viewportLayout->itemAt(curIndex - 1)->widget();
                    QString preObjectName = preWidget->objectName();
                    if (preObjectName == TextEditWidget) {
                        preData = static_cast<TextNoteEdit *>(preWidget)->getNoteBlock();
                    } else if (preObjectName == VoiceWidget) {
                        preData = static_cast<VoiceNoteItem *>(preWidget)->getNoteBlock();
                    }
                }
                m_noteItemData->datas.addBlock(preData, data);
                m_curItemWidget = itemWidget;
                itemWidget->setFocus();
                m_fIsNoteModified = true;
                saveNote();
                return m_curItemWidget;
            }
            QTextCursor cursor = itemWidget->textCursor();
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            cutStr = cursor.selectedText();
            cursor.removeSelectedText();
        }
        curIndex += 1;
        m_viewportLayout->insertWidget(curIndex, item);
        m_noteItemData->datas.addBlock(data);
        m_curItemWidget = item;
        if (!cutStr.isEmpty() || curIndex + 1 == m_viewportLayout->indexOf(m_placeholderWidget)) {
            VNoteBlock *textBlock = m_noteItemData->datas.newBlock(VNoteBlock::Text);
            textBlock->blockText = cutStr;
            m_curItemWidget = insertTextEdit(textBlock, true);
            m_noteItemData->datas.addBlock(item->getNoteBlock(), textBlock);
        }
    }
    m_fIsNoteModified = true;
    saveNote();
    return m_curItemWidget;
}

void RightView::onTextEditFocusIn()
{
    m_curItemWidget = static_cast<DWidget *>(sender());
}

void RightView::onTextEditFocusOut()
{
    TextNoteEdit *widget = static_cast<TextNoteEdit *>(sender());
    QString editText = widget->toPlainText();
    if (widget->getNoteBlock()->blockText != editText) {
        widget->getNoteBlock()->blockText = editText;
        m_fIsNoteModified = true;
        saveNote();
    }
}

void RightView::onTextEditDelEmpty()
{
    TextNoteEdit *widget = static_cast<TextNoteEdit *>(sender());
    if (widget->document()->isEmpty()) {
        int index = m_viewportLayout->indexOf(widget);
        if (m_viewportLayout->count() - index > 2) {
            delWidget(widget);
        }
    }
}

void RightView::onTextEditTextChange()
{
    m_fIsNoteModified = true;
}

void RightView::initData(VNoteItem *data)
{
    this->setFocus();
    if (m_noteItemData == data) {
        return;
    }
    while (m_viewportLayout->indexOf(m_placeholderWidget) != 0) {
        QWidget *widget = m_viewportLayout->itemAt(0)->widget();
        m_viewportLayout->removeWidget(widget);
        widget->deleteLater();
    }
    m_noteItemData = data;
    if (m_noteItemData == nullptr) {
        return;
    }
    int size = m_noteItemData->datas.datas.size();
    if (size) {
        for (auto it : m_noteItemData->datas.datas) {
            if (VNoteBlock::Text == it->getType()) {
                m_curItemWidget = insertTextEdit(it, true);
            } else if (VNoteBlock::Voice == it->getType()) {
                VoiceNoteItem *item = new VoiceNoteItem(m_noteItemData, static_cast<VNVoiceBlock *>(it), this);
                item->setObjectName(VoiceWidget);
                int preIndex = -1;
                if (m_curItemWidget != nullptr) {
                    preIndex = m_viewportLayout->indexOf(m_curItemWidget);
                }
                m_viewportLayout->insertWidget(preIndex + 1, item);
                m_curItemWidget = item;
                connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
                connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
                connect(item, &VoiceNoteItem::sigAction, this, &RightView::onVoiceMenu);
            }
        }
    } else {
        VNoteBlock *textBlock = m_noteItemData->datas.newBlock(VNoteBlock::Text);
        textBlock->blockText = data->noteTitle;
        m_curItemWidget = insertTextEdit(textBlock, true);
        m_noteItemData->datas.addBlock(nullptr, textBlock);
    }
}

void RightView::onVoicePlay(VoiceNoteItem *item)
{
    for (int i = 0; i < m_viewportLayout->count(); i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *tmpWidget = static_cast< VoiceNoteItem *>(widget);
            tmpWidget->showPlayBtn();
        }
    }
    item->showPauseBtn();
}

void RightView::onVoicePause(VoiceNoteItem *item)
{
    item->showPlayBtn();
}

void RightView::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    //TODO:
    //    Add the note save code here.
    saveNote();
}

void RightView::saveNote()
{
    qInfo() << __FUNCTION__ << "Is note changed:" << m_fIsNoteModified;

    if (m_noteItemData && m_fIsNoteModified) {
        for (int i = 0; i < m_viewportLayout->count(); i++) {
            QWidget *widget = m_viewportLayout->itemAt(i)->widget();
            if (widget->objectName() == TextEditWidget) {
                TextNoteEdit *tmpWidget = static_cast< TextNoteEdit *>(widget);
                if (widget->hasFocus()) {
                    tmpWidget->getNoteBlock()->blockText = tmpWidget->toPlainText();
                }
            }
        }
        VNoteItemOper noteOper(m_noteItemData);
        if (!noteOper.updateNote()) {
            qInfo() << "Save note error:" << *m_noteItemData;
        } else {
            m_fIsNoteModified = false;
        }
    }
}

void RightView::onVoiceMenu(QAction *action)
{
    m_menuVoice = static_cast<VoiceNoteItem *>(sender());
    ActionManager::ActionKind kind = ActionManager::getActionKind(action);
    switch (kind) {
    case ActionManager::VoiceDelete:
        delWidget(m_menuVoice);
        break;
    case ActionManager::VoiceSave:
        break;
    case ActionManager::VoiceConversion:
        m_menuVoice->showAsrStartWindow();
        break;
    default:
        break;
    }
}

void RightView::delWidget(DWidget *widget)
{
    if (widget != nullptr) {
        if (m_curItemWidget == widget) {
            int index = m_viewportLayout->indexOf(widget);
            int curIndex = index > 0 ? index - 1 : index + 1;
            m_curItemWidget = m_viewportLayout->itemAt(curIndex)->widget();
        }
        widget->disconnect();
        m_viewportLayout->removeWidget(widget);
        QString objName = widget->objectName();
        VNoteBlock *noteBlock = nullptr;
        if (objName == TextEditWidget) {
            noteBlock =  static_cast<TextNoteEdit *>(widget)->getNoteBlock();
        } else if (objName == VoiceWidget) {
            noteBlock =  static_cast<VoiceNoteItem *>(widget)->getNoteBlock();
        }
        if (noteBlock) {
            m_noteItemData->datas.delBlock(noteBlock);
        }
        widget->deleteLater();
        int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
        //两个文本块合为一个
        if (m_curItemWidget->objectName() == TextEditWidget) {
            QWidget *siblingWidget = nullptr;
            if (curIndex > 0) {
                siblingWidget = m_viewportLayout->itemAt(curIndex - 1)->widget();
            } else {
                QLayoutItem *layoutItem = m_viewportLayout->itemAt(curIndex + 1);
                if (layoutItem) {
                    siblingWidget = layoutItem->widget();
                }
            }
            if (siblingWidget && siblingWidget->objectName() == TextEditWidget) {
                TextNoteEdit *editSibing = static_cast<TextNoteEdit *>(siblingWidget);
                TextNoteEdit *editCurrent = static_cast<TextNoteEdit *>(m_curItemWidget);
                editCurrent->disconnect();
                VNTextBlock *textBlock = editSibing->getNoteBlock();
                if (curIndex > 0) {
                    textBlock->blockText = editSibing->toPlainText() + editCurrent->toPlainText();
                } else {
                    textBlock->blockText =  editCurrent->toPlainText() + editSibing->toPlainText();
                }
                editSibing->setPlainText(textBlock->blockText);
                m_viewportLayout->removeWidget(editCurrent);
                m_curItemWidget = editSibing;
                m_noteItemData->datas.delBlock(editCurrent->getNoteBlock());
                editCurrent->deleteLater();
                QTextCursor cursor = editSibing->textCursor();
                cursor.movePosition(QTextCursor::End);
                editSibing->setTextCursor(cursor);
                editSibing->setFocus();
            }
        }
        m_fIsNoteModified = true;
        saveNote();
    }
}

void RightView::setEnablePlayBtn(bool enable)
{
    for (int i = 0; i < m_viewportLayout->count(); i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *tmpWidget = static_cast< VoiceNoteItem *>(widget);
            tmpWidget->enblePlayBtn(enable);
        }
    }
}
