#include "rightview.h"
#include "textnoteitem.h"
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
#include <QClipboard>

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DFileDialog>
#include <DMessageBox>
#include <DApplication>
#include <DStyle>

//#define PlaceholderWidget "placeholder"
//#define VoiceWidget       "voiceitem"
//#define TextEditWidget    "textedititem"

RightView::RightView(QWidget *parent)
    : QWidget(parent)
{
    initUi();
    initMenu();
}

void RightView::initUi()
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    m_viewportLayout = new QVBoxLayout;
    m_viewportLayout->setSpacing(0);
    m_viewportLayout->setContentsMargins(20, 0, 20, 0);
    this->setLayout(m_viewportLayout);
    m_placeholderWidget = new DWidget(this); //占位的
    //m_placeholderWidget->setObjectName(PlaceholderWidget);
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
    m_fileHasDelDialog->setVisible(false);
}

void RightView::initMenu()
{
    //Init voice context Menu
    m_noteDetailContextMenu = ActionManager::Instance()->detialContextMenu();
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
    connect(editItem, &TextNoteItem::sigDelEmpty, this, &RightView::onTextEditDelEmpty);
    connect(editItem, &TextNoteItem::sigTextChanged, this, &RightView::onTextEditTextChange);
    return  editItem;
}

DetailItemWidget *RightView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNoteItemOper noteOps(m_noteItemData);

    VNoteBlock *data = m_noteItemData->newBlock(VNoteBlock::Voice);

    data->ptrVoice->voiceSize = voiceSize;
    data->ptrVoice->voicePath = voicePath;
    data->ptrVoice->createTime = QDateTime::currentDateTime();
    data->ptrVoice->voiceTitle = noteOps.getDefaultVoiceName();

    VoiceNoteItem *item = new VoiceNoteItem(data, this);

    connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
    connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);

    if (m_curItemWidget == nullptr) {
        m_viewportLayout->insertWidget(0, item);
        m_curItemWidget = item;
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        textBlock->blockText = "";
        m_curItemWidget = insertTextEdit(textBlock, true);
        m_noteItemData->addBlock(nullptr, textBlock);
    } else {
        QString cutStr = "";
        QString objName = m_curItemWidget->objectName();
        int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
        DetailItemWidget *itemWidget = static_cast<DetailItemWidget *>(m_curItemWidget);
        if (itemWidget->getNoteBlock()->blockType == VNoteBlock::Text) {
            if (itemWidget->textIsEmpty()) {
                m_viewportLayout->insertWidget(curIndex, item);
                VNoteBlock *preData = nullptr;
                if (curIndex > 0) {
                    DetailItemWidget *itemWidget = static_cast<DetailItemWidget *>(m_viewportLayout->itemAt(curIndex - 1)->widget());
                    if (itemWidget != m_placeholderWidget) {
                        preData = itemWidget->getNoteBlock();
                    }
                }
                m_noteItemData->addBlock(preData, data);
                m_curItemWidget = itemWidget;
                itemWidget->setFocus();
                int height = itemWidget->getCursorRect().bottom();
                adjustVerticalScrollBar(itemWidget, height);
                updateData();
                return m_curItemWidget;
            }
            QTextCursor cursor = itemWidget->getTextCursor();
            if (!cursor.atStart()) {
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cutStr = cursor.selectedText();
                cursor.removeSelectedText();
            } else {
                curIndex -= 1;
            }
        }
        curIndex += 1;
        m_viewportLayout->insertWidget(curIndex, item);
        m_noteItemData->addBlock(data);
        m_curItemWidget = item;
        if (!cutStr.isEmpty() || curIndex + 1 == m_viewportLayout->indexOf(m_placeholderWidget)) {
            VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
            textBlock->blockText = cutStr;
            m_curItemWidget = insertTextEdit(textBlock, true);
            m_noteItemData->addBlock(item->getNoteBlock(), textBlock);
        }
    }
    if (m_curItemWidget != nullptr) {
        int height = 0;
        QRect rc =  m_curItemWidget->getCursorRect();
        if (!rc.isEmpty()) {
            height = rc.bottom();
        }
        adjustVerticalScrollBar(m_curItemWidget, height);
    }
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
    if (widget->textIsEmpty()) {
        int index = m_viewportLayout->indexOf(widget);
        if (index > 0  && index != m_viewportLayout->count() - 1) {
            delWidget(widget);
        }
    }
}

void RightView::onTextEditTextChange()
{
    m_fIsNoteModified = true;
}

void RightView::initData(VNoteItem *data, QRegExp reg)
{
    while (m_viewportLayout->indexOf(m_placeholderWidget) != 0) {
        QWidget *widget = m_viewportLayout->itemAt(0)->widget();
        m_viewportLayout->removeWidget(widget);
        widget->deleteLater();
    }
    m_noteItemData = data;
    if (m_noteItemData == nullptr) {
        m_curItemWidget = nullptr;
        return;
    }
    int size = m_noteItemData->datas.dataConstRef().size();
    if (size) {
        for (auto it : m_noteItemData->datas.dataConstRef()) {
            if (VNoteBlock::Text == it->getType()) {
                m_curItemWidget = insertTextEdit(it, true, reg);
            } else if (VNoteBlock::Voice == it->getType()) {
                VoiceNoteItem *item = new VoiceNoteItem(it, this);
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
        m_curItemWidget = insertTextEdit(textBlock, true, reg);
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
        emit sigVoicePlay(data);
    }
}

void RightView::onVoicePause(VoiceNoteItem *item)
{
    item->showPlayBtn();
    emit sigVoicePause(item->getNoteBlock()->ptrVoice);
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
}

void RightView::initAction(DetailItemWidget *widget)
{
    QAction *voiceSaveAction = ActionManager::Instance()->getActionById(ActionManager::DetailVoiceSave);
    QAction *voice2TextAction = ActionManager::Instance()->getActionById(ActionManager::DetailVoice2Text);
    QAction *voiceDelAction = ActionManager::Instance()->getActionById(ActionManager::DetailDelete);
    QAction *copyAction = ActionManager::Instance()->getActionById(ActionManager::DetailCopy);
    QAction *cutAction = ActionManager::Instance()->getActionById(ActionManager::DetailCut);
    QAction *pasteAction = ActionManager::Instance()->getActionById(ActionManager::DetailPaste);
    QAction *selectAllAction = ActionManager::Instance()->getActionById(ActionManager::DetailSelectAll);

    selectAllAction->setVisible(true);
    voiceSaveAction->setVisible(false);
    voice2TextAction->setVisible(false);
    voiceDelAction->setVisible(false);
    copyAction->setVisible(false);
    cutAction->setVisible(false);
    pasteAction->setVisible(false);
    int voiceCount, textCount, tolCount;
    getSelectionCount(voiceCount, textCount);
    tolCount = voiceCount + textCount;
    if (tolCount) {
        copyAction->setVisible(true);
        if (textCount != 0) {
            cutAction->setVisible(true);
        }
    }
    if (widget != nullptr) {
        VNoteBlock *blockData =  widget->getNoteBlock();
        if (blockData->blockType == VNoteBlock::Voice) {
            if (!checkFileExist(blockData->ptrVoice->voicePath)) {
                delWidget(widget);
                updateData();
                return;
            }
            if (!tolCount) {
                voiceSaveAction->setVisible(true);
                voice2TextAction->setVisible(true);
                voiceDelAction->setVisible(true);
                selectAllAction->setVisible(false);
            }
        } else if (blockData->blockType == VNoteBlock::Text) {
            if (!tolCount) {
                pasteAction->setVisible(true);
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

        if (m_curItemWidget == widget) {
            if (nextWidget != m_placeholderWidget) {
                m_curItemWidget = nextWidget;
            } else if (preWidget != nullptr) {
                m_curItemWidget = preWidget;
            } else {
                m_curItemWidget = nullptr;
            }
        }

        if (m_menuWidget == widget) {
            m_menuWidget = nullptr;
        }

        if (index == m_viewportLayout->count() - 2) { //最后一个编辑框不删除
            if (!preWidget || preWidget->getNoteBlock()->blockType != VNoteBlock::Text) {
                widget->getNoteBlock()->blockText = "";
                widget->updateData();
                return;
            }
        }

        widget->disconnect();
        noteBlockList.push_back(widget->getNoteBlock());
        m_viewportLayout->removeWidget(widget);
        widget->deleteLater();

        if (merge && preWidget && nextWidget && //合并编辑框
                preWidget != m_placeholderWidget &&
                nextWidget != m_placeholderWidget &&
                preWidget->getNoteBlock()->blockType == VNoteBlock::Text &&
                nextWidget->getNoteBlock()->blockType == VNoteBlock::Text) {

            VNoteBlock *noteBlock = nextWidget->getNoteBlock();
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
        }
    }

    for (auto i : noteBlockList) {
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

VoiceNoteItem *RightView::getVoiceItem(VNoteBlock *data)
{
    for (int i = 0; i < m_viewportLayout->count() - 1; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget->getNoteBlock() == data) {
            return static_cast<VoiceNoteItem *>(widget);
        }
    }
    return nullptr;
}

DetailItemWidget *RightView::getMenuItem()
{
    return m_menuWidget;
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
    if (btn == Qt::RightButton) {
        m_menuWidget = widget;
        onMenuShow(widget);
    } else if (btn == Qt::LeftButton) {
        clearAllSelection();
//<<<<<<< Updated upstream
        if (widget != nullptr) {
            m_curItemWidget = widget;
        }
        if (m_curItemWidget != nullptr) {
            m_curItemWidget->setFocus();
        }
//=======
//        m_strSelectText = "";
//        m_pointStart = event->pos();
//        m_curItemWidget = getWidgetByPos(m_pointStart);
//>>>>>>> Stashed changes
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

//<<< <<< < Updated upstream
void RightView::mouseMoveSelect(QMouseEvent *event)
{
    DetailItemWidget *widget = getWidgetByPos(event->pos());

    if (widget && m_curItemWidget) {

#if 0
//        == == == =
        /**
         * @brief RightView::clearBeforeSelect
         * 清除上次选择未清除部分
         */
//        void RightView::clearBeforeSelect(const QPoint & curPos) {
//            int widgetIndex = 0;
//            int startIndex = 0;
//            int endIndex = 0;

//            QWidget *swidget = getWidgetByPos(m_pointStart);
//            QWidget *ewidget = getWidgetByPos(curPos);

//            if (swidget && ewidget && swidget != ewidget) {
//                widgetIndex = m_viewportLayout->indexOf(ewidget);

//                //向下选择
//                if (curPos.y() > m_pointStart.y()) {
//                    startIndex = widgetIndex + 1;
//                    endIndex = m_viewportLayout->count() - 1;
//                } else if (curPos.y() < m_pointStart.y()) {
//                    //向上选择
//                    startIndex = 0;
//                    endIndex = widgetIndex;
//                }

//                for (int i = startIndex; i < endIndex; i++) {
//                    QWidget *widget = m_viewportLayout->itemAt(i)->widget();
//                    if (widget && (widget->objectName() == TextEditWidget)) {
//                        TextNoteEdit *editItem = static_cast<TextNoteEdit *>(widget);
//                        if (editItem) {
//                            QTextCursor cursor = editItem->textCursor();
//                            cursor.clearSelection();
//                            editItem->setTextCursor(cursor);
////                    qInfo() << "select text:" << editItem->textCursor().selectedText();
//                        }
//                    }
//                }
//            }
//        }

//        /**
//         * @brief RightView::selectText2Clipboard
//         * 将选择的text添加到系统剪切板
//         */
//        void RightView::selectText2Clipboard() {
//            if (m_strSelectText != "") {
//                QClipboard *board = QApplication::clipboard();
//                if (board) {
//                    board->clear();
//                    board->setText(m_strSelectText);
//                    qInfo() << " Clip board text:" << board->text();
//                }
//            }
//        }

//        void RightView::mouseMoveSelect(QMouseEvent * event) {
//            QWidget *widget = getWidgetByPos(event->pos());
//            if (widget) {

//                clearBeforeSelect(event->pos());
//                >>> >>> > Stashed changes
#endif
        int widgetIndex = m_viewportLayout->indexOf(widget);
        int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
        QTextCursor::MoveOperation op = QTextCursor::NoMove;
        int minIndex = 0;
        int maxIndex = 0;
        if (widgetIndex != curIndex) {
            if (widgetIndex < curIndex) {
                op = QTextCursor::End;
                minIndex = widgetIndex + 1;
                maxIndex = curIndex - 1;
            } else {
                op = QTextCursor::Start;
                minIndex = curIndex + 1;
                maxIndex = widgetIndex - 1;
            }
        }

        widget->selectText(event->globalPos(), op);

        if (minIndex != maxIndex) {
            op = op == QTextCursor::Start ? QTextCursor::End : QTextCursor::Start;
            for (int i = 0 ; i < m_viewportLayout->count() - 1; i++) {
                QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
                DetailItemWidget *tmpWidget = static_cast<DetailItemWidget *>(layoutItem->widget());
                if (i < minIndex || i > maxIndex) {
                    if (i == curIndex) {
                        tmpWidget->selectText(op);
                    } else if (i != widgetIndex) {
                        tmpWidget->clearSelection();
                    }
                } else {
                    tmpWidget->selectAllText();
                }
            }
        }
    }
}

QString RightView::copySelectText(int *start, int *end)
{
    QString text = "";
    if (start && end) {
        *start = *end = -1;
    }
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
        DetailItemWidget *widget = static_cast< DetailItemWidget *>(layoutItem->widget());

        bool hasSelection = widget->hasSelection();
        if (hasSelection) {
            text.append(widget->getSelectText());
            if (start && end) {
                if (*start == -1) {
                    *start = i;
                } else {
                    *end = i;
                }
            }

        }
    }

    if (start && end && *end == -1) {
        *end = *start;
    }
    QClipboard *board = QApplication::clipboard();
    if (board) {
        board->clear();
        board->setText(text);
    }
    return text;
}

void RightView::cutSelectText()
{
    int start, end;
    copySelectText(&start, &end);
    if (start != -1) {
        QList<DetailItemWidget *> cutListWidget;
        int voiceCount, textCount;
        getSelectionCount(voiceCount, textCount);
        if (textCount == 0) {
            return;
        }
        for (int i = start ; i <= end; i++) {
            QLayoutItem *layoutItem = m_viewportLayout->itemAt(i);
            DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());

            if ((i == start || i == end) &&
                    widget->getNoteBlock()->blockType == VNoteBlock::Text) {
                if (widget->getAllText() != widget->getSelectText()) {
                    widget->removeSelectText();
                    widget->getNoteBlock()->blockText =  widget->getAllText();
                } else {
                    cutListWidget.push_back(widget);
                }
            } else {
                cutListWidget.push_back(widget);
            }
        }

        for (int i = 0; i < cutListWidget.size(); i++) {
            bool merge = i == cutListWidget.size() - 1 ? true : false;
            delWidget(cutListWidget[i], merge);
        }
    }
    updateData();
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
    if (m_menuWidget && m_menuWidget->getNoteBlock()->blockType == VNoteBlock::Text) {
        auto textCursor = m_menuWidget->getTextCursor();
        QClipboard *board = QApplication::clipboard();
        if (board) {
            QString clipBoardText = board->text();
            textCursor.insertText(clipBoardText);
        }
    }
}

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
            m_menuWidget = m_curItemWidget;
            pasteText();
            break;
        default:
            break;
        }
    }
}
