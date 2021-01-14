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

#include "rightview.h"
#include "textnoteitem.h"
#include "voicenoteitem.h"
#include "globaldef.h"
#include "vnoteapplication.h"
#include "common/utils.h"

#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "common/vnotedatamanager.h"
#include "common/actionmanager.h"
#include "task/exportnoteworker.h"
#include "common/opsstateinterface.h"
#include "db/vnoteitemoper.h"
#include "common/setting.h"

#include "dialog/vnotemessagedialog.h"

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DFileDialog>
#include <DMessageBox>
#include <DApplication>
#include <DStyle>

#include <QBoxLayout>
#include <QDebug>
#include <QStandardPaths>
#include <QScrollBar>
#include <QScrollArea>
#include <QAbstractTextDocumentLayout>
#include <QList>
#include <QClipboard>
#include <QThreadPool>

/**
 * @brief RightView::RightView
 * @param parent
 */
RightView::RightView(QWidget *parent)
    : QWidget(parent)
{
    initUi();
    initMenu();
    initConnection();
}

/**
 * @brief RightView::initUi
 */
void RightView::initUi()
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    m_viewportLayout = new QVBoxLayout;
    m_viewportLayout->setSpacing(0);
    m_viewportLayout->setContentsMargins(20, 0, 20, 0);
    this->setLayout(m_viewportLayout);
    m_placeholderWidget = new DWidget(this); //占位的
    m_placeholderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_viewportLayout->addWidget(m_placeholderWidget, 1, Qt::AlignBottom);

    m_playAnimTimer = new QTimer(this);
    m_playAnimTimer->setInterval(300);

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
}

/**
 * @brief RightView::initMenu
 */
void RightView::initMenu()
{
    //Init voice context Menu
    m_noteDetailContextMenu = ActionManager::Instance()->detialContextMenu();
    m_noteDetailVoiceMenu = ActionManager::Instance()->detialVoiceMenu();
}

/**
 * @brief RightView::insertTextEdit
 * @param data
 * @param focus
 * @param op
 * @param reg
 * @return 插入的编辑器窗口
 */
DetailItemWidget *RightView::insertTextEdit(VNoteBlock *data, bool focus, QTextCursor::MoveOperation op, QString reg)
{
    TextNoteItem *editItem = nullptr;
    int index = 0;
    if (m_curItemWidget != nullptr) {
        index = m_viewportLayout->indexOf(m_curItemWidget) + 1;
    }

    auto it = m_mapWidgetCache.find(data);
    if (it != m_mapWidgetCache.end()) {
        editItem = static_cast<TextNoteItem *>(it.value());
        editItem->setVisible(true);
        editItem->setLastCursorHeight(0);
    } else {
        editItem = new TextNoteItem(data, this);
        m_mapWidgetCache.insert(data, editItem);
    }

    m_viewportLayout->insertWidget(index, editItem);

    if (focus) {
        editItem->setFocus();
    }

    editItem->updateSearchKey(reg);

    QTextCursor cursor = editItem->getTextCursor();
    cursor.movePosition(op);
    editItem->setTextCursor(cursor);

    connect(editItem, &TextNoteItem::sigCursorHeightChange, this, &RightView::adjustVerticalScrollBar);
    connect(editItem, &TextNoteItem::sigFocusIn, this, &RightView::onTextEditFocusIn);
    connect(editItem, &TextNoteItem::sigFocusOut, this, &RightView::onTextEditFocusOut);
    connect(editItem, &TextNoteItem::sigSelectionChanged, this, &RightView::onTextEditSelectChange);
    connect(editItem, &TextNoteItem::sigTextChanged, this, &RightView::onTextEditTextChange);
    return editItem;
}

/**
 * @brief RightView::insertVoiceItem
 * @param voicePath
 * @param voiceSize
 * @return 插入的语音窗口
 */
DetailItemWidget *RightView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    if (m_curItemWidget == nullptr || m_noteItemData == nullptr) {
        qDebug() << "can not insert";
        return nullptr;
    }

    if (m_curItemWidget->getNoteBlock()->blockType == VNoteBlock::Voice) {
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        m_noteItemData->addBlock(m_curItemWidget->getNoteBlock(), textBlock);
        m_curItemWidget = insertTextEdit(textBlock, false);
    }

    VNoteItemOper noteOps(m_noteItemData);

    VNoteBlock *data = m_noteItemData->newBlock(VNoteBlock::Voice);

    data->ptrVoice->voiceSize = voiceSize;
    data->ptrVoice->voicePath = voicePath;
    data->ptrVoice->createTime = QDateTime::currentDateTime();
    data->ptrVoice->voiceTitle = noteOps.getDefaultVoiceName();

    VoiceNoteItem *item = new VoiceNoteItem(data, this);
    item->setAnimTimer(m_playAnimTimer);
    m_mapWidgetCache.insert(data, item);

    connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
    connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
    connect(item, &VoiceNoteItem::sigCursorHeightChange, this, &RightView::adjustVoiceVerticalScrollBar);

    QTextDocumentFragment cutStr;
    int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
    VNoteBlock *curBlock = m_curItemWidget->getNoteBlock();

    if (curBlock->blockType == VNoteBlock::Text) {
        QTextCursor cursor = m_curItemWidget->getTextCursor();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        m_curItemWidget->setTextCursor(cursor);
        cutStr = m_curItemWidget->getSelectFragment();
        cursor.removeSelectedText();
        if (!cutStr.isEmpty()) {
            Utils::documentToBlock(curBlock, m_curItemWidget->getTextDocument());
        }
    }

    curIndex += 1;
    m_viewportLayout->insertWidget(curIndex, item);

    VNoteBlock *preBlock = nullptr;
    if (curIndex > 0) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(curIndex - 1);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        preBlock = widget->getNoteBlock();
    }

    m_noteItemData->addBlock(preBlock, data);
    m_curItemWidget = item;

    QLayoutItem *nextlayoutItem = m_viewportLayout->itemAt(curIndex + 1);
    DetailItemWidget *nextWidget = static_cast<DetailItemWidget *>(nextlayoutItem->widget());

    if (nextWidget == m_placeholderWidget || nextWidget->getNoteBlock()->blockType == VNoteBlock::Voice) {
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        m_noteItemData->addBlock(m_curItemWidget->getNoteBlock(), textBlock);
        m_curItemWidget = insertTextEdit(textBlock, false);
    } else {
        m_curItemWidget = nextWidget;
    }

    if (!cutStr.isEmpty()) {
        QTextCursor cursor = m_curItemWidget->getTextCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.insertFragment(cutStr);
        Utils::documentToBlock(m_curItemWidget->getNoteBlock(), m_curItemWidget->getTextDocument());
        cursor.movePosition(QTextCursor::Start);
        m_curItemWidget->setTextCursor(cursor);
    }

    m_curItemWidget->setFocus();

    int height = 0;
    QRect rc = m_curItemWidget->getCursorRect();
    if (!rc.isEmpty()) {
        height = rc.bottom();
    }
    adjustVerticalScrollBar(m_curItemWidget, height);

    updateData();

    return m_curItemWidget;
}

/**
 * @brief RightView::onTextEditFocusIn
 */
void RightView::onTextEditFocusIn()
{
    if (m_curAsrItem) {
        TextNoteItem *widget = static_cast<TextNoteItem *>(sender());
        int height = 0;
        QRect rc = widget->getCursorRect();
        if (!rc.isEmpty()) {
            height = rc.bottom();
        }
        adjustVerticalScrollBar(widget, height);
    }
    if (!m_virtualKeyboardShow) {
        m_virtualKeyboardShow = true;
        emit virtualKeyboardShow(m_virtualKeyboardShow);
    }
}

/**
 * @brief RightView::onTextEditFocusOut
 */
void RightView::onTextEditFocusOut()
{
    if (m_fIsNoteModified) {
        TextNoteItem *widget = static_cast<TextNoteItem *>(sender());
        Utils::documentToBlock(widget->getNoteBlock(), widget->getTextDocument());
    }
    saveNote();
    if (m_virtualKeyboardShow) {
        VNoteBlock *block = m_curItemWidget->getNoteBlock();
        bool hideVirtualKeyboard = false;
        if (block->getType() == VNoteBlock::Voice) {
            hideVirtualKeyboard = true;
        } else if (!m_curItemWidget->hasFocus()) {
            QPoint pos = mapFromGlobal(QCursor::pos());
            if (!geometry().contains(pos)) {
                hideVirtualKeyboard = true;
            }
        }
        if (hideVirtualKeyboard) {
            m_virtualKeyboardShow = false;
            emit virtualKeyboardShow(m_virtualKeyboardShow);
        }
    }
}

/**
 * @brief RightView::onTextEditTextChange
 */
void RightView::onTextEditTextChange()
{
    m_fIsNoteModified = true;

    if (m_isFristTextChange == false) {
        saveNote();
        m_isFristTextChange = true;
    }
}

/**
 * @brief RightView::initData
 * @param data
 * @param reg 搜索关键字
 * @param fouse true 设置焦点
 */
void RightView::initData(VNoteItem *data, QString reg, bool fouse)
{
    this->setVisible(false);
    clearAllSelection();
    if (m_noteItemData == data) {
        m_searchKey = reg;
        for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
            QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
            DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
            if (widget->getNoteBlock()->blockType == VNoteBlock::Text) {
                widget->updateSearchKey(m_searchKey);
            }
        }
        this->setVisible(true);
        return;
    }

    while (m_viewportLayout->indexOf(m_placeholderWidget) != 0) {
        QLayoutItem *layoutItem = m_viewportLayout->takeAt(0);
        QWidget *widget = layoutItem->widget();
        widget->disconnect();
        widget->setVisible(false);
        delete layoutItem;
        layoutItem = nullptr;
    }

    if (fouse) {
        this->setFocus();
    }

    m_searchKey = reg;
    m_noteItemData = data;
    m_curAsrItem = nullptr;
    m_curPlayItem = nullptr;
    m_isFristTextChange = false;

    if (m_noteItemData == nullptr) {
        m_curItemWidget = nullptr;
        this->setVisible(true);
        return;
    }

    int size = m_noteItemData->datas.dataConstRef().size();
    QTextCursor::MoveOperation op = fouse ? QTextCursor::End : QTextCursor::Start;
    if (size) {
        for (auto it : m_noteItemData->datas.dataConstRef()) {
            if (VNoteBlock::Text == it->getType()) {
                m_curItemWidget = insertTextEdit(it, false, op, reg);
            } else if (VNoteBlock::Voice == it->getType()) {
                VoiceNoteItem *item = nullptr;

                auto itWidget = m_mapWidgetCache.find(it);
                if (itWidget != m_mapWidgetCache.end()) {
                    item = static_cast<VoiceNoteItem *>(itWidget.value());
                    item->setVisible(true);
                } else {
                    item = new VoiceNoteItem(it, this);
                    item->setAnimTimer(m_playAnimTimer);
                    m_mapWidgetCache.insert(it, item);
                }

                int preIndex = -1;
                if (m_curItemWidget != nullptr) {
                    preIndex = m_viewportLayout->indexOf(m_curItemWidget);
                }
                m_viewportLayout->insertWidget(preIndex + 1, item);
                m_curItemWidget = item;
                connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
                connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
                connect(item, &VoiceNoteItem::sigCursorHeightChange, this, &RightView::adjustVoiceVerticalScrollBar);
            }
        }
    } else {
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        m_curItemWidget = insertTextEdit(textBlock, fouse, op, reg);
        m_noteItemData->addBlock(nullptr, textBlock);
    }

    if (fouse == false) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(0);
        m_curItemWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
    } else {
        m_curItemWidget->setFocus();
        adjustVerticalScrollBar(m_curItemWidget, m_curItemWidget->height());
    }
    this->setVisible(true);
}

/**
 * @brief RightView::onVoicePlay
 * @param item
 */
void RightView::onVoicePlay(VoiceNoteItem *item)
{
    VNVoiceBlock *data = item->getNoteBlock()->ptrVoice;
    if (data) {
        if (!checkFileExist(data->voicePath)) {
            removeSelectWidget(item);
            delWidget(item);
            updateData();
            return;
        }
        for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
            QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
            DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
            if (widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
                VoiceNoteItem *tmpWidget = static_cast<VoiceNoteItem *>(widget);
                tmpWidget->showPlayBtn();
            }
        }
        item->showPauseBtn();
        m_curPlayItem = item;
        emit sigVoicePlay(data);
    }
}

/**
 * @brief RightView::onVoicePause
 * @param item
 */
void RightView::onVoicePause(VoiceNoteItem *item)
{
    item->showPlayBtn();
    emit sigVoicePause(item->getNoteBlock()->ptrVoice);
}

/**
 * @brief RightView::onPlayUpdate
 */
void RightView::onPlayUpdate()
{
    if (nullptr != m_curPlayItem) {
        m_curPlayItem->updateAnim();
    }
}

/**
 * @brief RightView::saveNote
 */
void RightView::saveNote()
{
    //qInfo() << __FUNCTION__ << "Is note changed:" << m_fIsNoteModified;
    if (m_noteItemData && m_fIsNoteModified) {
        for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
            QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
            DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
            VNoteBlock *block = widget->getNoteBlock();

            if (block->blockType == VNoteBlock::Text && widget->hasFocus()) {
                Utils::documentToBlock(block, widget->getTextDocument());
            }
        }

        VNoteItemOper noteOper(m_noteItemData);
        if (!noteOper.updateNote()) {
            qInfo() << "Save note error:" << *m_noteItemData;
        } else {
            m_fIsNoteModified = false;

            //Notify middle view refresh.
            emit contentChanged();
        }
    }
}

/**
 * @brief RightView::initAction
 * @param widget
 * @return -1,不弹出右键菜单
 */
int RightView::initAction(DetailItemWidget *widget)
{
    Q_UNUSED(widget);
    auto voiceWidgets = m_selectWidget.values(VoicePlugin);
    auto textWidgets = m_selectWidget.values(TextEditPlugin);
    bool canCutDel = true;
    OpsStateInterface *stateInterface = OpsStateInterface::instance();
    bool isAISrvAvailable = stateInterface->isAiSrvExist();
    if (m_curPlayItem && m_curPlayItem->hasSelection()) {
        canCutDel = false;
    }

    if (stateInterface->isVoice2Text() && m_curAsrItem && m_curAsrItem->hasSelection()) {
        canCutDel = false;
    }
    bool allTextEmpty = isAllWidgetEmpty(textWidgets);
    bool onlyOneVoice = false;

    if (voiceWidgets.size() == 1 && allTextEmpty) {
        onlyOneVoice = true;
        VNoteBlock *blockData = voiceWidgets[0]->getNoteBlock();
        if (voiceWidgets[0]->isSelectAll()) { //语音菜单
            if (!checkFileExist(blockData->ptrVoice->voicePath)) {
                removeSelectWidget(voiceWidgets[0]);
                delWidget(voiceWidgets[0]);
                updateData();
                return -1;
            }
            ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::NoteDetailvoiceMenu, false);
            if (isAISrvAvailable && !stateInterface->isVoice2Text() && blockData->blockText.isEmpty()) {
                ActionManager::Instance()->enableAction(ActionManager::DetailVoice2Text, true);
            }
            ActionManager::Instance()->enableAction(ActionManager::DetailVoiceSave, true);
            if (canCutDel) {
                ActionManager::Instance()->enableAction(ActionManager::DetailVoiceDelete, canCutDel);
            }
            return 0;
        }
    }
    return -1;
#if 0
    bool TTSisWorking = false;
    ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::NoteDetailCtxMenu, false);
    if (isAISrvAvailable) {
        TTSisWorking = VTextSpeechAndTrManager::isTextToSpeechInWorking();
        if (TTSisWorking) {
            ActionManager::Instance()->visibleAction(ActionManager::DetailStopreading, true);
            ActionManager::Instance()->visibleAction(ActionManager::DetailText2Speech, false);
            ActionManager::Instance()->enableAction(ActionManager::DetailStopreading, true);
        } else {
            ActionManager::Instance()->visibleAction(ActionManager::DetailStopreading, false);
            ActionManager::Instance()->visibleAction(ActionManager::DetailText2Speech, true);
        }
    }

    if (m_viewportLayout->count() == 2) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(0);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget && !widget->textIsEmpty()) {
            ActionManager::Instance()->enableAction(ActionManager::DetailSelectAll, true);
        }
    } else {
        ActionManager::Instance()->enableAction(ActionManager::DetailSelectAll, true);
    }

    if (!voiceWidgets.size() && allTextEmpty && widget) {
        VNoteBlock *blockData = widget->getNoteBlock();
        if (widget->hasFocus() && blockData->blockType == VNoteBlock::Text) {
            ActionManager::Instance()->enableAction(ActionManager::DetailPaste, true);
            if (isAISrvAvailable && VTextSpeechAndTrManager::getSpeechToTextEnable()) {
                ActionManager::Instance()->enableAction(ActionManager::DetailSpeech2Text, true);
            }
        }
        return 1;
    }

    if (voiceWidgets.size()) {
        if (!isAllWidgetEmpty(voiceWidgets)) {
            ActionManager::Instance()->enableAction(ActionManager::DetailCopy, true);
            if (isAISrvAvailable) {
                if (VTextSpeechAndTrManager::getTransEnable()) {
                    ActionManager::Instance()->enableAction(ActionManager::DetailTranslate, true);
                }
                if (!TTSisWorking && VTextSpeechAndTrManager::getTextToSpeechEnable()) {
                    ActionManager::Instance()->enableAction(ActionManager::DetailText2Speech, true);
                }
            }
        }

        if (canCutDel && !onlyOneVoice) {
            ActionManager::Instance()->enableAction(ActionManager::DetailCut, canCutDel);
            ActionManager::Instance()->enableAction(ActionManager::DetailContextDelete, canCutDel);
        }

    } else if (textWidgets.size()) {
        ActionManager::Instance()->enableAction(ActionManager::DetailCopy, true);
        if (isAISrvAvailable) {
            if (VTextSpeechAndTrManager::getTransEnable()) {
                ActionManager::Instance()->enableAction(ActionManager::DetailTranslate, true);
            }
            if (!TTSisWorking && VTextSpeechAndTrManager::getTextToSpeechEnable()) {
                ActionManager::Instance()->enableAction(ActionManager::DetailText2Speech, true);
            }
        }
        if (canCutDel) {
            ActionManager::Instance()->enableAction(ActionManager::DetailCut, canCutDel);
            ActionManager::Instance()->enableAction(ActionManager::DetailContextDelete, canCutDel);
        }
        return 1;
    }
    return 1;
#endif
}

/**
 * @brief RightView::onMenuShow
 * @param widget
 */
void RightView::onMenuShow(DetailItemWidget *widget)
{
    if (widget && widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
        if (!widget->hasSelection()) {
            if (TouchNormal == m_touchState) {
                clearAllSelection();
            }
            widget->selectAllText();
            m_selectWidget.insert(VoicePlugin, widget);
            m_curItemWidget = widget;
            QString selecText = m_curItemWidget->getSelectFragment().toPlainText();
            if (!selecText.isEmpty()) {
                QClipboard *board = QApplication::clipboard();
                if (board) {
                    board->setText(selecText, QClipboard::Selection);
                }
            }
        }
    } else {
        return;
    }

    int ret = initAction(widget);
    switch (ret) {
    case 0:
        if (!m_noteDetailVoiceMenu->isVisible()) {
            m_noteDetailVoiceMenu->popup(QCursor::pos());
        }
        break;
    case 1:
        if (!m_noteDetailVoiceMenu->isVisible()) {
            m_noteDetailContextMenu->popup(QCursor::pos());
        }
        break;
    default:
        break;
    }
}

/**
 * @brief RightView::delWidget
 * @param widget
 * @param merge true 合并编辑窗口
 */
void RightView::delWidget(DetailItemWidget *widget, bool merge)
{
    QList<VNoteBlock *> noteBlockList;
    if (widget != nullptr && widget != m_placeholderWidget) {
        int index = m_viewportLayout->indexOf(widget);
        VNoteBlock *noteBlock = widget->getNoteBlock();
        if (noteBlock->blockType == VNoteBlock::Text) {
            if (index == 0 || (index == m_viewportLayout->count() - 2 && index != 1)) { //第一个和最后一个编辑框不删，只清空内容
                QTextDocument *doc = widget->getTextDocument();
                if (doc != nullptr) {
                    doc->clear();
                }
                noteBlock->blockText = "";
                return;
            }
        }

        DetailItemWidget *preWidget = nullptr;
        DetailItemWidget *nextWidget = nullptr;
        QLayoutItem *layoutItem = nullptr;

        layoutItem = m_viewportLayout->itemAt(index - 1);
        if (layoutItem) {
            preWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
        }

        layoutItem = m_viewportLayout->itemAt(index + 1);
        if (layoutItem) {
            nextWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
        }

        if (preWidget && nextWidget) {
            //两个语音之间的编辑框不删除,只清空内容
            if (noteBlock->blockType == VNoteBlock::Text && preWidget->getNoteBlock()->blockType == VNoteBlock::Voice && nextWidget->getNoteBlock()->blockType == VNoteBlock::Voice) {
                QTextDocument *doc = widget->getTextDocument();
                if (doc) {
                    doc->clear();
                }
                widget->getNoteBlock()->blockText = "";
                return;
            }
        }

        if (m_curItemWidget == widget) {
            if (nextWidget != m_placeholderWidget) {
                m_curItemWidget = nextWidget;
            } else if (preWidget != nullptr) {
                m_curItemWidget = preWidget;
            } else {
                m_curItemWidget = nullptr;
            }
        }

        if (m_curPlayItem == widget) {
            m_curPlayItem = nullptr;
        }

        if (m_curAsrItem == widget) {
            m_curAsrItem = nullptr;
        }

        widget->setVisible(false);
        noteBlockList.push_back(noteBlock);
        layoutItem = m_viewportLayout->takeAt(index);
        widget->deleteLater();
        delete layoutItem;
        layoutItem = nullptr;

        if (merge && preWidget && nextWidget && //合并编辑框
            preWidget != m_placeholderWidget && nextWidget != m_placeholderWidget && preWidget->getNoteBlock()->blockType == VNoteBlock::Text && nextWidget->getNoteBlock()->blockType == VNoteBlock::Text) {
            noteBlock = nextWidget->getNoteBlock();

            QTextCursor cursor = nextWidget->getTextCursor();
            cursor.movePosition(QTextCursor::Start);
            nextWidget->setTextCursor(cursor);

            cursor.insertFragment(QTextDocumentFragment(preWidget->getTextDocument()));
            Utils::documentToBlock(noteBlock, nextWidget->getTextDocument());

            preWidget->setVisible(false);
            noteBlockList.push_back(preWidget->getNoteBlock());
            layoutItem = m_viewportLayout->takeAt(index - 1);
            preWidget->deleteLater();
            delete layoutItem;
            layoutItem = nullptr;

            nextWidget->setFocus();

            if (m_curItemWidget == preWidget) {
                m_curItemWidget = nextWidget;
            }
        }
    }

    for (auto i : noteBlockList) {
        auto it = m_mapWidgetCache.find(i);
        if (it != m_mapWidgetCache.end()) {
            m_mapWidgetCache.erase(it);
        }
        i->releaseSpecificData();
        m_noteItemData->delBlock(i);
    }

    if (m_curItemWidget) { //调整滚动条
        int height = 0;
        if (m_curItemWidget->getNoteBlock()->blockType == VNoteBlock::Text) {
            QRect rc = m_curItemWidget->getCursorRect();
            height += rc.bottom();
            m_curItemWidget->setFocus();
        }
        adjustVerticalScrollBar(m_curItemWidget, height);
    }
}

/**
 * @brief RightView::setEnablePlayBtn
 * @param enable
 */
void RightView::setEnablePlayBtn(bool enable)
{
    for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());

        if (widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
            VoiceNoteItem *tmpWidget = static_cast<VoiceNoteItem *>(widget);
            tmpWidget->enblePlayBtn(enable);
        }
    }
}

/**
 * @brief RightView::updateData
 */
void RightView::updateData()
{
    m_updateTimer->stop();
    m_fIsNoteModified = true;
    saveNote();
    m_updateTimer->start();
}

/**
 * @brief RightView::checkFileExist
 * @param file
 * @return true 文件存在
 */
bool RightView::checkFileExist(const QString &file)
{
    QFileInfo fi(file);
    if (!fi.exists()) {
        VNoteMessageDialog::getDialog(VNoteMessageDialog::VoiceDeleted, this)->exec();
        return false;
    }
    return true;
}

/**
 * @brief RightView::adjustVerticalScrollBar
 * @param widget
 * @param defaultHeight
 */
void RightView::adjustVerticalScrollBar(QWidget *widget, int defaultHeight)
{
    int tolHeight = defaultHeight;
    if (tolHeight >= 0) {
        tolHeight += 20;
    }
    int index = m_viewportLayout->indexOf(widget);
    for (int i = 0; i < index; i++) {
        tolHeight += m_viewportLayout->itemAt(i)->widget()->height();
    }
    emit sigCursorChange(tolHeight, false);
}

/**
 * @brief RightView::adjustVoiceVerticalScrollBar
 * @param widget
 * @param defaultHeight
 */
void RightView::adjustVoiceVerticalScrollBar(DetailItemWidget *widget, int defaultHeight)
{
    int index = m_viewportLayout->indexOf(widget);
    QLayoutItem *nextItem = m_viewportLayout->itemAt(index + 1);
    if (nextItem) {
        DetailItemWidget *nextWidget = static_cast<DetailItemWidget *>(nextItem->widget());
        if (nextWidget->hasFocus()) {
            widget = nextWidget;
            QRect rc = nextWidget->getCursorRect();
            defaultHeight = rc.bottom();
        }
    }
    adjustVerticalScrollBar(widget, defaultHeight);
}

/**
 * @brief RightView::mouseMoveEvent
 * @param event
 */
void RightView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_noteItemData == nullptr) {
        return;
    }
    double dis = 0;
    QPoint pt = m_touchPressPoint;
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        double distX = mapFromGlobal(QCursor::pos()).x() - m_touchPressPoint.x();
        double distY = mapFromGlobal(QCursor::pos()).y() - m_touchPressPoint.y();
        dis = qAbs(distX) > qAbs(distY) ? qAbs(distX) : qAbs(distY);
        QDateTime current = QDateTime::currentDateTime();
        qint64 timeParam = current.toMSecsSinceEpoch() - m_touchPressStartMs;
        bool isReturn = false;
        if (TouchState::TouchPressing == m_touchState) {
            if ((qAbs(distY) <= 10 || qAbs(distX) <= 10)) {
                return;
            }
            if (timeParam <= 250) {
                emit requestToSlide(distY > 0);
                m_touchState = TouchState::TouchSliding;
                isReturn = true;
            }
        } else if (TouchState::TouchSliding == m_touchState) {
            if (qAbs(distY) > 10) {
                double speed = distY / timeParam;
                emit requestToSlide(distY > 0);
                qDebug() << speed;
            }
            isReturn = true;
        }
        m_touchPressPoint = event->pos();
        m_touchPressStartMs = current.toMSecsSinceEpoch();
        if (isReturn)
            return;
    }
    if (dis > 1 || TouchNormal == m_touchState) {
        if (event->source() == Qt::MouseEventSynthesizedByQt) {
            if (TouchSelecting != m_touchState) {
                DetailItemWidget *widget = getWidgetByPos(pt);
                if (widget) {
                    widget->setCursorByPos(event->globalPos());
                }
            }
            m_touchState = TouchSelecting;
        }
        emit sigCursorChange(event->pos().y(), true);
        mouseMoveSelect(event);
    }
}

/**
 * @brief RightView::mousePressEvent
 * @param event
 */
void RightView::mousePressEvent(QMouseEvent *event)
{
    if (m_noteItemData == nullptr) {
        return;
    }

    Qt::MouseButton btn = event->button();
    DetailItemWidget *widget = getWidgetByPos(event->pos());

    if (nullptr != widget) {
        m_curItemWidget = widget;
    }

    if (m_curItemWidget) {
        m_curItemWidget->setFocus();
    }

    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        m_touchState = TouchState::TouchPressing;
        m_touchPressPoint = event->pos();
        m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
        if (m_popMenuTimer == nullptr) {
            m_popMenuTimer = new QTimer(this);
            m_popMenuTimer->setSingleShot(true);
            m_popMenuTimer->setInterval(1000);
            connect(m_popMenuTimer, &QTimer::timeout, this, [=] {
                if (m_touchState == TouchState::TouchPressing) {
                    onMenuShow(getWidgetByPos(mapFromGlobal(QCursor::pos())));
                }
            });
        }
        m_popMenuTimer->start();
    } else {
        m_touchState = TouchState::TouchNormal;
    }

    if (btn == Qt::RightButton) {
        onMenuShow(widget);
    } else if (btn == Qt::LeftButton) {
        if (event->source() != Qt::MouseEventSynthesizedByQt) {
            clearAllSelection();
        }
        if (m_curItemWidget->getNoteBlock()->blockType == VNoteBlock::Voice) {
            if (widget && !m_curItemWidget->isTextContainsPos(event->globalPos()) && !m_curItemWidget->hasSelection()) {
                clearAllSelection();
                m_curItemWidget->selectAllText();
                m_selectWidget.insert(VoicePlugin, widget);
            }
        }
    }
}

/**
 * @brief RightView::mouseReleaseEvent
 * @param event
 */
void RightView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_popMenuTimer && m_popMenuTimer->isActive()) {
        m_popMenuTimer->stop();
    }

    if (m_noteItemData == nullptr) {
        return;
    }

    DetailItemWidget *widget = getWidgetByPos(event->pos());

    if (TouchPressing == m_touchState) {
        QDateTime current = QDateTime::currentDateTime();
        qint64 timeParam = current.toMSecsSinceEpoch() - m_touchPressStartMs;
        if (timeParam < 1000) {
            clearAllSelection();
            if (widget) {
                //单机语音插件选中
                if (widget->getNoteBlock()->blockType == VNoteBlock::Voice && !widget->isTextContainsPos(event->globalPos())) {
                    widget->selectAllText();
                    m_selectWidget.insert(VoicePlugin, widget);
                } else if (widget->getNoteBlock()->blockType == VNoteBlock::Text) { //编辑器设置光标
                    widget->setCursorByPos(event->globalPos());
                }
            }
        }
        m_touchState = TouchState::TouchNormal;
    }

    Qt::MouseButton btn = event->button();
    if (btn == Qt::LeftButton) {
        m_selectWidget.clear();
        QString selecText = "";
        for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
            QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
            DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
            if (widget->hasSelection()) {
                if (widget->getNoteBlock()->blockType == VNoteBlock::Text) {
                    m_selectWidget.insert(TextEditPlugin, widget);
                } else {
                    m_selectWidget.insert(VoicePlugin, widget);
                }
                selecText.append(widget->getSelectFragment().toPlainText());
            }
        }

        if (!selecText.isEmpty()) {
            QClipboard *board = QApplication::clipboard();
            if (board) {
                board->setText(selecText, QClipboard::Selection);
            }
        }
    }

    if (m_curItemWidget) {
        m_curItemWidget->setFocus();
    }
}

/**
 * @brief RightView::getWidgetByPos
 * @param pos
 * @return 窗口
 */
DetailItemWidget *RightView::getWidgetByPos(const QPoint &pos)
{
    for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->geometry().contains(pos)) {
            return static_cast<DetailItemWidget *>(widget);
        }
    }
    return nullptr;
}

/**
 * @brief RightView::getIsNormalView
 * @param
 * 详情页是否为当前笔记
 */
bool RightView::getIsNormalView() const
{
    return m_isNormalView;
}

/**
 * @brief RightView::setIsNormalView
 * @param value
 * 更新详情页状态
 */
void RightView::setIsNormalView(bool value)
{
    m_isNormalView = value;
}

/**
 * @brief RightView::mouseMoveSelect
 * @param event
 */
void RightView::mouseMoveSelect(QMouseEvent *event)
{
    DetailItemWidget *widget = getWidgetByPos(event->pos());

    if (widget && m_curItemWidget) {
        int widgetIndex = m_viewportLayout->indexOf(widget);
        int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
        QTextCursor::MoveOperation op = QTextCursor::NoMove;
        int minIndex = 0;
        int maxIndex = 0;
        if (widgetIndex != curIndex) {
            if (widgetIndex < curIndex) {
                op = QTextCursor::End;
                minIndex = widgetIndex;
                maxIndex = curIndex;
            } else {
                op = QTextCursor::Start;
                minIndex = curIndex;
                maxIndex = widgetIndex;
            }
        }

        widget->selectText(event->globalPos(), op);

        if (minIndex != maxIndex) {
            op = op == QTextCursor::Start ? QTextCursor::End : QTextCursor::Start;
            for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
                QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
                DetailItemWidget *tmpWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
                if (i <= minIndex || i >= maxIndex) {
                    if (i == curIndex) {
                        tmpWidget->selectText(op);
                    } else {
                        if (i == widgetIndex) {
                            if (tmpWidget->getNoteBlock()->blockType == VNoteBlock::Voice) {
                                tmpWidget->selectAllText();
                            }
                            if (m_curItemWidget->getNoteBlock()->blockType == VNoteBlock::Voice
                                && m_curItemWidget->hasSelection()) {
                                m_curItemWidget->selectAllText();
                            }
                        } else {
                            tmpWidget->clearSelection();
                        }
                    }
                } else {
                    tmpWidget->selectAllText();
                }
            }
        } else if (minIndex == 0 && maxIndex == 0) {
            for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
                QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
                DetailItemWidget *tmpWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
                if (tmpWidget != m_curItemWidget) {
                    tmpWidget->clearSelection();
                }
            }
        }
    }
}

/**
 * @brief RightView::copySelectText
 * @param voiceText
 */
void RightView::copySelectText(bool voiceText)
{
    QString text = getSelectText(voiceText);
    if (!text.isEmpty()) {
        QClipboard *board = QApplication::clipboard();
        if (board) {
            board->clear();
            board->setText(text);
        }
    }
}

/**
 * @brief RightView::getSelectText
 * @param voiceText
 * @return 选中的文本
 */
QString RightView::getSelectText(bool voiceText)
{
    QString text = "";
    bool firstSelect = true;

    for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        bool hasSelection = voiceText;

        if (voiceText || widget->getNoteBlock()->blockType != VNoteBlock::Voice) {
            hasSelection = widget->hasSelection();
        }

        if (hasSelection) {
            QString selectText = widget->getSelectFragment().toPlainText();
            if (!selectText.isEmpty()) {
                if (!firstSelect && !selectText.startsWith('\n')) {
                    text.append("\n");
                }
                text.append(selectText);
            }
            firstSelect = false;
        }
    }

    return text;
}

/**
 * @brief RightView::cutSelectText
 */
void RightView::cutSelectText()
{
    int ret = showWarningDialog();
    if (ret == 1) {
        VNoteMessageDialog *confirmDialog = VNoteMessageDialog::getDialog(VNoteMessageDialog::CutNote, this);
        connect(confirmDialog, &VNoteMessageDialog::accepted, this, [this]() {
            copySelectText(false);
            delSelectText();
        });
        confirmDialog->exec();
    } else if (ret == 0) {
        copySelectText(false);
        delSelectText();
    }
}

/**
 * @brief RightView::delSelectText
 */
void RightView::delSelectText()
{
    QList<DetailItemWidget *> delListWidget;
    for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget->hasSelection()) {
            if (widget->isSelectAll() == false) {
                widget->removeSelectText();
                Utils::documentToBlock(widget->getNoteBlock(), widget->getTextDocument());
                continue;
            }
            delListWidget.push_back(widget);
        }
    }

    clearAllSelection();

    int size = delListWidget.size();
    if (size) {
        for (int i = 0; i < size; i++) {
            bool merge = i == delListWidget.size() - 1 ? true : false;
            delWidget(delListWidget[i], merge);
        }
    }
    updateData();
}

/**
 * @brief RightView::clearAllSelection
 */
void RightView::clearAllSelection()
{
    if (m_selectWidget.size()) {
        QMutableMapIterator<ItemWidgetType,
                            DetailItemWidget *>
            it(m_selectWidget);
        while (it.hasNext()) {
            it.next();
            it.value()->clearSelection();
        }
        closeMenu();
        VTextSpeechAndTrManager::onStopTextToSpeech();

        m_selectWidget.clear();
        QClipboard *board = QApplication::clipboard();
        if (board) {
            board->clear(QClipboard::Selection);
        }
    }
}

/**
 * @brief RightView::selectAllItem
 */
void RightView::selectAllItem()
{
    m_selectWidget.clear();
    QString selecText = "";

    for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        widget->selectAllText();
        if (widget->getNoteBlock()->blockType == VNoteBlock::Text) {
            m_selectWidget.insert(TextEditPlugin, widget);
        } else {
            m_selectWidget.insert(VoicePlugin, widget);
        }
        selecText.append(widget->getSelectFragment().toPlainText());
    }

    QClipboard *board = QApplication::clipboard();
    if (board) {
        board->setText(selecText, QClipboard::Selection);
    }
}

/**
 * @brief RightView::pasteText
 */
void RightView::pasteText()
{
    if (m_curItemWidget && m_curItemWidget->hasFocus() && m_curItemWidget->getNoteBlock()->blockType == VNoteBlock::Text) {
        auto voiceWidget = m_selectWidget.values(VoicePlugin);
        auto textWidget = m_selectWidget.values(TextEditPlugin);
        if (!voiceWidget.size() && isAllWidgetEmpty(textWidget)) { //没有选中才可以粘贴
            m_curItemWidget->pasteText();
            m_curItemWidget->setFocus();
        }
    }
}

/**
 * @brief RightView::keyPressEvent
 * @param e
 */
void RightView::keyPressEvent(QKeyEvent *e)
{
    DWidget::keyPressEvent(e);
    if (e->modifiers() == Qt::ControlModifier) {
        switch (e->key()) {
        case Qt::Key_C:
            copySelectText();
            break;
        case Qt::Key_A:
            selectAllItem();
            break;
        case Qt::Key_X:
            cutSelectText();
            break;

        case Qt::Key_V:
            pasteText();
            break;
        default:
            break;
        }
    } else if (e->key() == Qt::Key_Delete) {
        int ret = showWarningDialog();
        if (ret == 1) {
            VNoteMessageDialog *confirmDialog = VNoteMessageDialog::getDialog(VNoteMessageDialog::DeleteNote, this);
            connect(confirmDialog, &VNoteMessageDialog::accepted, this, [this]() {
                delSelectText();
            });
            confirmDialog->exec();
        } else if (ret == 0) {
            delSelectText();
        }
    }
}

/**
 * @brief RightView::setCurVoicePlay
 * @param item
 */
void RightView::setCurVoicePlay(VoiceNoteItem *item)
{
    m_curPlayItem = item;
}

/**
 * @brief RightView::setCurVoiceAsr
 * @param item
 */
void RightView::setCurVoiceAsr(VoiceNoteItem *item)
{
    m_curAsrItem = item;
}

/**
 * @brief RightView::getCurVoicePlay
 * @return 当前播放语音窗口
 */
VoiceNoteItem *RightView::getCurVoicePlay()
{
    return m_curPlayItem;
}

/**
 * @brief RightView::getCurVoiceAsr
 * @return 当前语音转文字窗口
 */
VoiceNoteItem *RightView::getCurVoiceAsr()
{
    return m_curAsrItem;
}

/**
 * @brief RightView::showWarningDialog
 * @return 0不显示，1显示，-1错误
 */
int RightView::showWarningDialog()
{
    auto voiceWidget = m_selectWidget.values(VoicePlugin);
    if (voiceWidget.size()) {
        if (m_curPlayItem && m_curPlayItem->hasSelection()) {
            return -1;
        }

        if (OpsStateInterface::instance()->isVoice2Text() && m_curAsrItem && m_curAsrItem->hasSelection()) {
            return -1;
        }

        if (voiceWidget.size() == 1 && !voiceWidget[0]->isSelectAll()) {
            return -1;
        }
        return 1;
    }

    auto textWidget = m_selectWidget.values(TextEditPlugin);
    if (isAllWidgetEmpty(textWidget)) {
        return -1;
    }

    return 0;
}

/**
 * @brief RightView::initConnection
 */
void RightView::initConnection()
{
    connect(m_playAnimTimer, &QTimer::timeout, this, &RightView::onPlayUpdate);
    connect(m_updateTimer, &QTimer::timeout, this, &RightView::saveNote);
    m_updateTimer->start();
}

/**
 * @brief RightView::saveMp3
 */
void RightView::saveMp3()
{
    DetailItemWidget *widget = getOnlyOneSelectVoice();

    if (widget == nullptr) {
        return;
    }
    if (widget != m_curItemWidget) {
        m_curItemWidget = widget;
    }

    if (m_curItemWidget) {
        VNoteBlock *block = m_curItemWidget->getNoteBlock();
        if (block->blockType == VNoteBlock::Voice) {
            DFileDialog dialog;
            dialog.setFileMode(DFileDialog::DirectoryOnly);

            dialog.setLabelText(DFileDialog::Accept, DApplication::translate("RightView", "Save"));
            dialog.setNameFilter("MP3(*.mp3)");
            QString historyDir = setting::instance()->getOption(VNOTE_EXPORT_VOICE_PATH_KEY).toString();
            if (historyDir.isEmpty()) {
                historyDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            }
            dialog.setDirectory(historyDir);
            if (QDialog::Accepted == dialog.exec()) {
                // save the directory string to config file.
                setting::instance()->setOption(VNOTE_EXPORT_VOICE_PATH_KEY, dialog.directoryUrl().toLocalFile());

                QString exportDir = dialog.directoryUrl().toLocalFile();
                //多选操作-导出语音
                QList<VNoteItem *> noteDataList;
                noteDataList.append(m_noteItemData);
                ExportNoteWorker *exportWorker = new ExportNoteWorker(
                    exportDir, ExportNoteWorker::ExportOneVoice, noteDataList, block);
                exportWorker->setAutoDelete(true);

                QThreadPool::globalInstance()->start(exportWorker);
            }
        }
    }
}

/**
 * @brief RightView::isAllWidgetEmpty
 * @param widget
 * @return true 窗口没有文本内容
 */
bool RightView::isAllWidgetEmpty(const QList<DetailItemWidget *> &widget)
{
    for (auto it : widget) {
        if (!it->textIsEmpty()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief RightView::getOnlyOneSelectVoice
 * @return 选中的语音窗口
 */
DetailItemWidget *RightView::getOnlyOneSelectVoice()
{
    auto voiceWidget = m_selectWidget.values(VoicePlugin);
    auto textWidget = m_selectWidget.values(TextEditPlugin);

    if (voiceWidget.size() == 1 && isAllWidgetEmpty(textWidget)) {
        if (voiceWidget[0]->isSelectAll()) {
            VNoteBlock *blockData = voiceWidget[0]->getNoteBlock();
            if (!checkFileExist(blockData->ptrVoice->voicePath)) {
                removeSelectWidget(voiceWidget[0]);
                delWidget(voiceWidget[0]);
                updateData();
                return nullptr;
            }
            return voiceWidget[0];
        }
    }

    return nullptr;
}

/**
 * @brief RightView::removeSelectWidget
 * @param widget
 */
void RightView::removeSelectWidget(DetailItemWidget *widget)
{
    QMutableMapIterator<ItemWidgetType, DetailItemWidget *> it(m_selectWidget);
    while (it.hasNext()) {
        if (widget == it.next().value()) {
            it.remove();
            break;
        }
    }
}

/**
 * @brief RightView::onTextEditSelectChange
 */
void RightView::onTextEditSelectChange()
{
    TextNoteItem *editItem = static_cast<TextNoteItem *>(sender());
    if (!editItem->hasSelection()) {
        removeSelectWidget(editItem);
    }
}

/**
 * @brief RightView::removeCacheWidget
 * @param data
 */
void RightView::removeCacheWidget(VNoteItem *data)
{
    if (data != nullptr && !m_mapWidgetCache.isEmpty()) {
        for (auto it : data->datas.dataConstRef()) {
            auto itWidget = m_mapWidgetCache.find(it);
            if (itWidget != m_mapWidgetCache.end()) {
                DetailItemWidget *widget = itWidget.value();
                widget->deleteLater();
                m_mapWidgetCache.erase(itWidget);
            }
        }
    }
}

/**
 * @brief RightView::removeCacheWidget
 * @param data
 */
void RightView::removeCacheWidget(const VNoteFolder *data)
{
    if (data != nullptr && !m_mapWidgetCache.isEmpty()) {
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(data->id);
        if (notes) {
            notes->lock.lockForRead();
            for (auto it : notes->folderNotes) {
                removeCacheWidget(it);
            }
            notes->lock.unlock();
        }
    }
}

/**
 * @brief RightView::closeMenu
 */
void RightView::closeMenu()
{
    m_noteDetailContextMenu->close();
    m_noteDetailVoiceMenu->close();
}

int RightView::getEditerGlobalY()
{
    int yPos = -1;
    if (m_curItemWidget) {
        QPoint pos = m_curItemWidget->mapToGlobal(m_curItemWidget->getCursorRect().bottomLeft());
        yPos = pos.y();
    }
    return yPos;
}
