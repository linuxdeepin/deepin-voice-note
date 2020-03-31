#include "rightview.h"
#include "textnoteitem.h"
#include "voicenoteitem.h"
#include "globaldef.h"
#include "vnoteapplication.h"
#include "common/utils.h"

#include "common/vnoteitem.h"
#include "common/vnotedatamanager.h"
#include "common/actionmanager.h"
#include "common/exportnoteworker.h"
#include "common/opsstateinterface.h"
#include "db/vnoteitemoper.h"

#include "dialog/vnotemessagedialog.h"

#include <QBoxLayout>
#include <QDebug>
#include <QStandardPaths>
#include <QTimer>
#include <QScrollBar>
#include <QScrollArea>
#include <QAbstractTextDocumentLayout>
#include <QList>
#include <QClipboard>
#include <QThreadPool>

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DFileDialog>
#include <DMessageBox>
#include <DApplication>
#include <DStyle>

RightView::RightView(QWidget *parent)
    : QWidget(parent)
{
    initUi();
    initMenu();
    initAppSetting();
    initConnection();
    onChangeTheme();
}

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
    QString content = DApplication::translate(
                          "RightView",
                          "The voice note has been deleted");
    m_fileHasDelDialog = new DDialog("", content, this);
    m_fileHasDelDialog->setIcon(QIcon::fromTheme("dialog-warning"));
    m_fileHasDelDialog->addButton(DApplication::translate(
                                      "RightView",
                                      "OK"), false, DDialog::ButtonNormal);
    m_playAnimTimer = new QTimer(this);
    m_playAnimTimer->setInterval(300);
}

void RightView::initMenu()
{
    //Init voice context Menu
    m_noteDetailContextMenu = ActionManager::Instance()->detialContextMenu();
}

void RightView::initAppSetting()
{
    m_qspSetting = reinterpret_cast<VNoteApplication*>(qApp)->appSetting();
}

DetailItemWidget *RightView::insertTextEdit(VNoteBlock *data, bool focus, QRegExp reg)
{
    TextNoteItem *editItem = new TextNoteItem(data, this, reg);
    int index = 0;
    if (m_curItemWidget != nullptr) {
        index = m_viewportLayout->indexOf(m_curItemWidget) + 1;
    }
    m_viewportLayout->insertWidget(index, editItem);
    if (focus) {
        QTextCursor cursor = editItem->getTextCursor();
        cursor.movePosition(QTextCursor::End);
        editItem->setTextCursor(cursor);
        editItem->setFocus();
    }
    connect(editItem, &TextNoteItem::sigCursorHeightChange, this, &RightView::adjustVerticalScrollBar);
    connect(editItem, &TextNoteItem::sigFocusOut, this, &RightView::onTextEditFocusOut);
    //connect(editItem, &TextNoteItem::sigDelEmpty, this, &RightView::onTextEditDelEmpty);
    connect(editItem, &TextNoteItem::sigTextChanged, this, &RightView::onTextEditTextChange);
    return  editItem;
}

DetailItemWidget *RightView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{

    if (m_curItemWidget == nullptr || m_noteItemData == nullptr) {
        qDebug() << "can not insert";
        return  nullptr;
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

    connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
    connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
    QString cutStr = "";
    int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
    DetailItemWidget *itemWidget = static_cast<DetailItemWidget *>(m_curItemWidget);
    if (itemWidget->getNoteBlock()->blockType == VNoteBlock::Text) {
        QTextCursor cursor = itemWidget->getTextCursor();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cutStr = cursor.selectedText();
        cursor.removeSelectedText();
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

    bool needTextEdit = false;
    QLayoutItem *nextlayoutItem = m_viewportLayout->itemAt(curIndex + 1);
    DetailItemWidget *nextWidget = static_cast<DetailItemWidget *>(nextlayoutItem->widget());
    VNoteBlock *nextBlock = nullptr;

    if(nextWidget == m_placeholderWidget){ //最后要加个编辑框
        needTextEdit = true;
    }else {
        nextBlock = nextWidget->getNoteBlock();
        if(nextBlock->blockType == VNoteBlock::Voice){ //下一个还是语音项要加编辑框
            needTextEdit = true;
        }else if (nextBlock->blockType == VNoteBlock::Text) {
            if(!cutStr.isEmpty()){
                nextBlock->blockText = cutStr + nextWidget->getAllText();
                nextWidget->updateData();
            }
        }
    }

    if (needTextEdit) {
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        textBlock->blockText = cutStr;
        m_curItemWidget = insertTextEdit(textBlock, true);
        m_noteItemData->addBlock(item->getNoteBlock(), textBlock);
    } else {
        int index = m_viewportLayout->indexOf(m_curItemWidget);
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(index + 1);
        if (layoutItem) {
            DetailItemWidget *nextWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
            if (nextWidget && nextWidget != m_placeholderWidget) {
                m_curItemWidget = nextWidget;
                m_curItemWidget->setFocus();
            }
        }
    }

    int height = 0;
    QRect rc =  m_curItemWidget->getCursorRect();
    if (!rc.isEmpty()) {
        height = rc.bottom();
    }
    adjustVerticalScrollBar(m_curItemWidget, height);

    updateData();

    return m_curItemWidget;
}

void RightView::onTextEditFocusOut()
{
    DetailItemWidget *widget = static_cast<DetailItemWidget *>(sender());
    QString text = widget->getAllText();
    VNoteBlock *block =  widget->getNoteBlock();
    if (text != block->blockText) {
        block->blockText = text;
        updateData();
    }
}

void RightView::onTextEditDelEmpty()
{
    DetailItemWidget *widget = static_cast<DetailItemWidget *>(sender());
    delWidget(widget);
    updateData();
}

void RightView::onTextEditTextChange()
{
    m_fIsNoteModified = true;
    if (m_isFristTextChange == false) {
        m_isFristTextChange = true;
        saveNote();
    }
}

void RightView::initData(VNoteItem *data, QRegExp reg, bool fouse)
{
    while (m_viewportLayout->indexOf(m_placeholderWidget) != 0) {
        QWidget *widget = m_viewportLayout->itemAt(0)->widget();
        m_viewportLayout->removeWidget(widget);
        widget->deleteLater();
    }
    if (fouse) {
        this->setFocus();
    }
    m_isFristTextChange = false;
    m_noteItemData = data;
    m_curAsrItem = nullptr;
    m_curPlayItem = nullptr;

    if (m_noteItemData == nullptr) {
        m_curItemWidget = nullptr;
        return;
    }
    int size = m_noteItemData->datas.dataConstRef().size();
    if (size) {
        for (auto it : m_noteItemData->datas.dataConstRef()) {
            if (VNoteBlock::Text == it->getType()) {
                m_curItemWidget = insertTextEdit(it, fouse, reg);
            } else if (VNoteBlock::Voice == it->getType()) {
                VoiceNoteItem *item = new VoiceNoteItem(it, this);
                item->setAnimTimer(m_playAnimTimer);
                int preIndex = -1;
                if (m_curItemWidget != nullptr) {
                    preIndex = m_viewportLayout->indexOf(m_curItemWidget);
                }
                m_viewportLayout->insertWidget(preIndex + 1, item);
                m_curItemWidget = item;
                connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
                connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
            }
        }
    } else {
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        m_curItemWidget = insertTextEdit(textBlock, fouse, reg);
        m_noteItemData->addBlock(nullptr, textBlock);
    }
}

void RightView::onVoicePlay(VoiceNoteItem *item)
{
    VNVoiceBlock *data = item->getNoteBlock()->ptrVoice;
    if (data) {
        if (!checkFileExist(data->voicePath)) {
            delWidget(item);
            updateData();
            return;
        }
        for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
            QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
            DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
            if (widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
                VoiceNoteItem *tmpWidget = static_cast< VoiceNoteItem *>(widget);
                tmpWidget->showPlayBtn();
            }
        }
        item->showPauseBtn();
        m_curPlayItem = item;
        emit sigVoicePlay(data);
    }
}

void RightView::onVoicePause(VoiceNoteItem *item)
{
    item->showPlayBtn();
    emit sigVoicePause(item->getNoteBlock()->ptrVoice);
}

void RightView::onPlayUpdate()
{
    if (nullptr != m_curPlayItem) {
        m_curPlayItem->updateAnim();
    }
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
    //qInfo() << __FUNCTION__ << "Is note changed:" << m_fIsNoteModified;
    if (m_noteItemData && m_fIsNoteModified) {
        for (int i = 0; i < m_viewportLayout->count() - 1; i++) {

            QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
            DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
            if (widget->hasFocus()) {
                widget->getNoteBlock()->blockText = widget->getAllText();
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

void RightView::initAction(DetailItemWidget *widget)
{
    ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::NoteDetailCtxMenu, false);

    if(m_viewportLayout->count() == 2){
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(0);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if(widget && !widget->textIsEmpty()){
            ActionManager::Instance()->enableAction(ActionManager::DetailSelectAll, true);
        }
    }else {
         ActionManager::Instance()->enableAction(ActionManager::DetailSelectAll, true);
    }

    int voiceCount, textCount, tolCount;
    getSelectionCount(voiceCount, textCount);
    tolCount = voiceCount + textCount;

    VNoteBlock *blockData = nullptr;
    OpsStateInterface * stateInterface = gVNoteOpsStates();

    if (widget != nullptr) {
        blockData = widget->getNoteBlock();
    }

    if (tolCount) {
        ActionManager::Instance()->enableAction(ActionManager::DetailCopy, true);

        if (textCount != 0) {
            if (!blockData || blockData->blockType == VNoteBlock::Text) {
                ActionManager::Instance()->enableAction(ActionManager::DetailCut, true);
                ActionManager::Instance()->enableAction(ActionManager::DetailDelete, true);
            }
        }
    }

    if (blockData != nullptr) {
        if (blockData->blockType == VNoteBlock::Voice) {
            if (!checkFileExist(blockData->ptrVoice->voicePath)) {
                delWidget(widget);
                updateData();
                return;
            }

            if (!tolCount) {
                ActionManager::Instance()->enableAction(ActionManager::DetailVoiceSave, true);
                if (blockData->ptrVoice->blockText.isEmpty() && !stateInterface->isVoice2Text()) {
                     ActionManager::Instance()->enableAction(ActionManager::DetailVoice2Text, true);
                }
            }

            VoiceNoteItem *item = static_cast<VoiceNoteItem *>(widget);
            bool enable = true;

            if (stateInterface->isVoice2Text() &&
                    (m_curAsrItem == item || m_curAsrItem->hasSelection())) {
                enable = false;
            }

            if ((m_curPlayItem == item) || (m_curPlayItem && m_curPlayItem->hasSelection())) {
                enable = false;
            }

            if (tolCount && textCount == 0) {
                if (tolCount != 1 || !item->hasSelection()) {
                    enable = false;
                }
            }

            ActionManager::Instance()->enableAction(ActionManager::DetailDelete, enable);
            if (tolCount && enable) {
                ActionManager::Instance()->enableAction(ActionManager::DetailCut, enable);
            }

        } else if (blockData->blockType == VNoteBlock::Text) {
            if (!tolCount) {
                ActionManager::Instance()->enableAction(ActionManager::DetailPaste, true);
            }
        }
    }
}

void RightView::onMenuShow(DetailItemWidget *widget)
{
    initAction(widget);
    m_noteDetailContextMenu->exec(QCursor::pos());
}

void RightView::delWidget(DetailItemWidget *widget, bool merge)
{
    QList<VNoteBlock *> noteBlockList;
    if (widget != nullptr && widget != m_placeholderWidget) {
        int index = m_viewportLayout->indexOf(widget);
        VNoteBlock *noteBlock = widget->getNoteBlock();
        if (noteBlock->blockType == VNoteBlock::Text) {
            if (index == 0 || (index == m_viewportLayout->count() - 2 && index != 1)) { //第一个和最后一个编辑框不删，只清空内容
                widget->getNoteBlock()->blockText = "";
                widget->updateData();
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
            if (noteBlock->blockType == VNoteBlock::Text &&
                    preWidget->getNoteBlock()->blockType == VNoteBlock::Voice &&
                    nextWidget->getNoteBlock()->blockType == VNoteBlock::Voice) {
                widget->getNoteBlock()->blockText = "";
                widget->updateData();
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

        if(m_curPlayItem == widget){
            m_curPlayItem = nullptr;
        }

        if(m_curAsrItem == widget){
            m_curAsrItem = nullptr;
        }

        widget->disconnect();
        noteBlockList.push_back(noteBlock);
        m_viewportLayout->removeWidget(widget);
        widget->deleteLater();

        if (merge && preWidget && nextWidget && //合并编辑框
                preWidget != m_placeholderWidget &&
                nextWidget != m_placeholderWidget &&
                preWidget->getNoteBlock()->blockType == VNoteBlock::Text &&
                nextWidget->getNoteBlock()->blockType == VNoteBlock::Text) {

            noteBlock = nextWidget->getNoteBlock();
            noteBlock->blockText = preWidget->getAllText() + nextWidget->getAllText();
            nextWidget->updateData();

            preWidget->disconnect();
            noteBlockList.push_back(preWidget->getNoteBlock());
            m_viewportLayout->removeWidget(preWidget);
            preWidget->deleteLater();

            QTextCursor cursor = nextWidget->getTextCursor();
            cursor.movePosition(QTextCursor::End);
            nextWidget->setTextCursor(cursor);
            nextWidget->setFocus();

            if (m_curItemWidget == preWidget) {
                m_curItemWidget = nextWidget;
            }
        }
    }

    for (auto i : noteBlockList) {
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

void RightView::setEnablePlayBtn(bool enable)
{
    for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());

        if (widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
            VoiceNoteItem *tmpWidget = static_cast< VoiceNoteItem *>(widget);
            tmpWidget->enblePlayBtn(enable);
        }
    }
}

DetailItemWidget *RightView::getMenuItem()
{
    return m_curItemWidget;
}

void RightView::updateData()
{
    m_fIsNoteModified = true;
    saveNote();
}

bool RightView::checkFileExist(const QString &file)
{
    QFileInfo fi(file);
    if (!fi.exists()) {
        m_fileHasDelDialog->exec();
        return false;
    }
    return true;
}

void RightView::adjustVerticalScrollBar(QWidget *widget, int defaultHeight)
{
    int tolHeight = defaultHeight + 20;
    int index = m_viewportLayout->indexOf(widget);
    for (int i = 0; i < index; i++) {
        tolHeight += m_viewportLayout->itemAt(i)->widget()->height();
    }
    emit sigCursorChange(tolHeight, false);
}

void RightView::mouseMoveEvent(QMouseEvent *event)
{
    emit sigCursorChange(event->pos().y(), true);
    mouseMoveSelect(event);
}

void RightView::mousePressEvent(QMouseEvent *event)
{
    DWidget::mousePressEvent(event);
    Qt::MouseButton btn = event->button();
    DetailItemWidget *widget = getWidgetByPos(event->pos());

    if (widget != nullptr) {
        m_curItemWidget = widget;
    }

    if (btn == Qt::RightButton) {
        onMenuShow(m_curItemWidget);
    } else if (btn == Qt::LeftButton) {
        clearAllSelection();
        if (m_curItemWidget != nullptr) {
            m_curItemWidget->setFocus();
        }
    }
}

void RightView::mouseReleaseEvent(QMouseEvent *event)
{
    DWidget::mouseReleaseEvent(event);
}

DetailItemWidget *RightView::getWidgetByPos(const QPoint &pos)
{
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->geometry().contains(pos)) {
            return static_cast<DetailItemWidget *>(widget);
        }
    }
    return  nullptr;
}

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
            for (int i = 0 ; i < m_viewportLayout->count() - 1; i++) {
                QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
                DetailItemWidget *tmpWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
                if (i <= minIndex || i >= maxIndex) {
                    if (i == curIndex) {
                        tmpWidget->selectText(op);
                    } else if (i != widgetIndex) {
                        tmpWidget->clearSelection();
                    }
                } else {
                    tmpWidget->selectAllText();
                }
            }
        } else if (minIndex == 0 && maxIndex == 0) {
            QLayoutItem *layoutItem = m_viewportLayout->itemAt(1);
            if (layoutItem) {
                DetailItemWidget *tmpWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
                if(tmpWidget && tmpWidget != m_placeholderWidget){
                    tmpWidget->clearSelection();
                }
            }
        }
    }
}

QString RightView::copySelectText()
{
    QString text = "";
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast< DetailItemWidget *>(layoutItem->widget());

        bool hasSelection = widget->hasSelection();
        if (hasSelection) {
            text.append(widget->getSelectText());
        }
    }
    QClipboard *board = QApplication::clipboard();
    if (board) {
        board->clear();
        board->setText(text);
    }
    return text;
}

void RightView::cutSelectText(bool isByAction)
{
    if (m_curItemWidget) {
        if (!isByAction) {
            initAction(m_curItemWidget);
            QAction *cutAction = ActionManager::Instance()->getActionById(ActionManager::DetailCut);
            if (!cutAction->isEnabled()) {
                qDebug() << "can not cut";
                return;
            }
        }

        copySelectText();
        delSelectText();
    }
}

void RightView::delSelectText()
{
    QList<DetailItemWidget *> delListWidget;
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget->hasSelection()) {
            if (i == 0 || i == m_viewportLayout->count() - 2) {
                VNoteBlock *block = widget->getNoteBlock();
                if (block->blockType == VNoteBlock::Text) {
                    if (widget->getAllText() != widget->getSelectText()) {
                        widget->removeSelectText();
                        widget->getNoteBlock()->blockText =  widget->getAllText();
                        continue;
                    }
                }
            }
            delListWidget.push_back(widget);
        }
    }
    int size = delListWidget.size();
    if (size) {
        for (int i = 0; i < size; i++) {
            bool merge = i == delListWidget.size() - 1 ? true : false;
            delWidget(delListWidget[i], merge);
        }
        updateData();
        clearAllSelection();
    }
}

void RightView::clearAllSelection()
{
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        widget->clearSelection();
    }
}

void RightView::selectAllItem()
{
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        widget->selectAllText();
    }
}

void RightView::getSelectionCount(int &voiceCount, int &textCount)
{
    voiceCount = textCount  = 0;
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget->hasSelection()) {
            if (widget->getNoteBlock()->blockType == VNoteBlock::Text) {
                textCount ++;
            } else {
                voiceCount ++;
            }
        }
    }
}

void RightView::pasteText()
{
    if (m_curItemWidget && m_curItemWidget->getNoteBlock()->blockType == VNoteBlock::Text) {
        auto textCursor = m_curItemWidget->getTextCursor();
        QClipboard *board = QApplication::clipboard();
        if (board) {
            QString clipBoardText = board->text();
            textCursor.insertText(clipBoardText);
        }
        m_curItemWidget->setFocus();
    }
}

void RightView::keyPressEvent(QKeyEvent *e)
{
    DWidget::keyPressEvent(e);
    if (e->modifiers() == Qt::ControlModifier) {
        switch (e->key()) {
        case Qt::Key_C:
            if(hasSelection()){
               copySelectText();
            }
            break;
        case Qt::Key_A:
            selectAllItem();
            break;
        case Qt::Key_X:
            if(hasSelection()){
                if(showDelDialog()){
                    VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote);
                    connect(&confirmDialog, &VNoteMessageDialog::accepted, this, [this](){
                        cutSelectText(false);
                    });
                    confirmDialog.exec();
                }else {
                   cutSelectText(false);
                }
            }
            break;
        case Qt::Key_V:
            pasteText();
            break;
        default:
            break;
        }
    } else if (e->key() == Qt::Key_Delete) {
        if(showDelDialog()){
            VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote);
            connect(&confirmDialog, &VNoteMessageDialog::accepted, this, [this](){
                doDelAction(false);
            });
            confirmDialog.exec();
        }else {
           doDelAction(false);
        }
    }
}

void RightView::setCurVoicePlay(VoiceNoteItem *item)
{
    m_curPlayItem = item;
}

void RightView::setCurVoiceAsr(VoiceNoteItem *item)
{
    m_curAsrItem = item;
}

VoiceNoteItem *RightView::getCurVoicePlay()
{
    return  m_curPlayItem;
}

VoiceNoteItem *RightView::getCurVoiceAsr()
{
    return  m_curAsrItem;
}

bool RightView::hasSelection()
{
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget->hasSelection()) {
            return true;
        }
    }
    return false;
}

void RightView::doDelAction(bool isByAction)
{
    if (m_curItemWidget) {
        if (!isByAction) {
            initAction(m_curItemWidget);
            QAction *delAction = ActionManager::Instance()->getActionById(ActionManager::DetailDelete);
            if (!delAction->isEnabled()) {
                qDebug() << "can not del";
                return;
            }
        }

        if (hasSelection()) {
            delSelectText();
        } else if (m_curItemWidget) {
            VNoteBlock *block = m_curItemWidget->getNoteBlock();
            if (block->blockType == VNoteBlock::Voice) {
                delWidget(m_curItemWidget);
                updateData();
            }
        }

    }
}

bool RightView::showDelDialog()
{
    bool showDialog = false;
    int voiceCount, textCount;
    getSelectionCount(voiceCount, textCount);
    if (voiceCount) {
        showDialog = true;
    } else if (m_curItemWidget) {
        if (!textCount && m_curItemWidget->getNoteBlock()->blockType == VNoteBlock::Voice) {
            showDialog = true;
        }
    }
    return showDialog;
}

void RightView::initConnection()
{
    connect(DGuiApplicationHelper::instance(),
                     &DGuiApplicationHelper::themeTypeChanged,
            this, &RightView::onChangeTheme);

    connect(m_playAnimTimer, &QTimer::timeout, this, &RightView::onPlayUpdate);
}

void RightView::onChangeTheme()
{
}

void RightView::saveMp3()
{
    if(m_curItemWidget){
        VNoteBlock *block = m_curItemWidget->getNoteBlock();
        if(block->blockType == VNoteBlock::Voice){
            DFileDialog dialog;
            dialog.setFileMode(DFileDialog::DirectoryOnly);
            dialog.setLabelText(DFileDialog::Accept,DApplication::translate("RightView","Save"));

            QString historyDir = m_qspSetting->value(VNOTE_EXPORT_VOICE_PATH_KEY).toString();
            if (historyDir.isEmpty()) {
                historyDir = QDir::homePath();
            }
            dialog.setDirectory(historyDir);
            if (QDialog::Accepted == dialog.exec()) {
                // save the directory string to config file.
                m_qspSetting->setValue(VNOTE_EXPORT_VOICE_PATH_KEY, dialog.directoryUrl().toLocalFile());

                QString exportDir = dialog.selectedFiles().at(0);

                ExportNoteWorker* exportWorker = new ExportNoteWorker(
                            exportDir,ExportNoteWorker::ExportOneVoice, m_noteItemData, block);
                exportWorker->setAutoDelete(true);

                QThreadPool::globalInstance()->start(exportWorker);
            }
        }
    }
}
