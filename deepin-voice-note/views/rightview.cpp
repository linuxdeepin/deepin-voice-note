#include "rightview.h"
#include "textnoteedit.h"
#include "voicenoteitem.h"

#include "common/vnoteitem.h"
#include "common/vnotedatamanager.h"
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
    m_viewportLayout->setContentsMargins(0, 0, 0, 0);
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
    mainLayout->setContentsMargins(20, 0, 20, 0);
    this->setLayout(mainLayout);
}

QWidget *RightView::insertTextEdit(int preWidgetIndex, QString strText, bool focus)
{
    QLayoutItem *preItem = m_viewportLayout->itemAt(preWidgetIndex);
    if (preItem != nullptr) {
        QWidget *preWidget = preItem->widget();
        QString objName = preWidget->objectName();
        if (objName == TextEditWidget) {
            return preWidget;
        }
    }
    TextNoteEdit *editItem = new TextNoteEdit(m_viewportWidget);
    DStyle::setFocusRectVisible(editItem, false);
    DPalette pb = DApplicationHelper::instance()->palette(editItem);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    editItem->setPalette(pb);

    editItem->setPlainText(strText);
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
    m_viewportLayout->insertWidget(preWidgetIndex + 1, editItem);
    if (focus) {
        QTextCursor cursor = editItem->textCursor();
        cursor.movePosition(QTextCursor::End);
        editItem->setTextCursor(cursor);
        editItem->setFocus();
    }
    return  editItem;
}

QWidget *RightView::insertVoiceItem()
{
    if (m_curItemWidget != nullptr) {
        QString objName = m_curItemWidget->objectName();
        int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
        if (objName == TextEditWidget) {
            TextNoteEdit *itemWidget = static_cast<TextNoteEdit *>(m_curItemWidget);
            QString cutStr = "";
            QTextCursor cursor = itemWidget->textCursor();
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            cutStr = cursor.selectedText();
            cursor.removeSelectedText();
            if (itemWidget->document()->isEmpty()) {
                m_viewportLayout->removeWidget(itemWidget);
                itemWidget->deleteLater();
                curIndex -= 1;
            }
            VoiceNoteItem *item = new VoiceNoteItem(nullptr, this);
            item->setObjectName(VoiceWidget);
            connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
            connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
            m_viewportLayout->insertWidget(curIndex + 1, item);
            m_curItemWidget = item;

            m_curItemWidget = insertTextEdit(m_viewportLayout->indexOf(item), cutStr, true);
            qDebug() << m_viewportLayout->count();
        }
    }
    return nullptr;
}

void RightView::onTextEditFocusIn()
{
    m_curItemWidget = static_cast<DWidget *>(sender());
}

void RightView::onTextEditFocusOut()
{
    TextNoteEdit *widget = static_cast<TextNoteEdit *>(sender());
    if (widget->document()->isEmpty()) {
        int index = m_viewportLayout->indexOf(widget);
        if (m_viewportLayout->count() - index > 2) {
            m_viewportLayout->removeWidget(widget);
            widget->deleteLater();
            if (m_curItemWidget == widget) {
                m_curItemWidget = nullptr;
            }
        }
    }
}

void RightView::onTextEditDelEmpty()
{
    TextNoteEdit *widget = static_cast<TextNoteEdit *>(sender());
    if (widget->document()->isEmpty()) {
        int index = m_viewportLayout->indexOf(widget);
        if (m_viewportLayout->count() - index > 2) {
            m_viewportLayout->removeWidget(widget);
            widget->deleteLater();
            if (m_curItemWidget == widget) {
                m_curItemWidget = nullptr;
            }
        }
    }
}

void RightView::onTextEditTextChange()
{
    ;
}

void RightView::initData(VNoteItem *data)
{
    this->setFocus();
    while (m_viewportLayout->indexOf(m_placeholderWidget) != 0) {
        QWidget *widget = m_viewportLayout->itemAt(0)->widget();
        m_viewportLayout->removeWidget(widget);
        widget->deleteLater();
    }
    m_noteItemData = data;
    m_curItemWidget = insertTextEdit(-1, data->noteTitle, true);
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
