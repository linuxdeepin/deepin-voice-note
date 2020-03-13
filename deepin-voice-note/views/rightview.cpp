#include "rightview.h"
//#include "textnoteedit.h"
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

#define PlaceholderWidget "placeholder"
#define VoiceWidget       "voiceitem"
#define TextEditWidget    "textedititem"

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
    m_placeholderWidget->setObjectName(PlaceholderWidget);
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
    m_voiceContextMenu = ActionManager::Instance()->voiceContextMenu();
}

QWidget *RightView::insertTextEdit(VNoteBlock *data, bool focus)
{
    TextNoteEdit *editItem = new TextNoteEdit(m_noteItemData, data->ptrText, this);
    editItem->setContextMenuPolicy(Qt::NoContextMenu);
    DStyle::setFocusRectVisible(editItem, false);
    DPalette pb = DApplicationHelper::instance()->palette(editItem);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    //editItem->setPalette(pb);

    editItem->setPlainText(data->blockText);
    editItem->setObjectName(TextEditWidget);
    editItem->document()->setDocumentMargin(0);
    editItem->setFixedHeight(24);
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
    QTextDocument *document = editItem->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, [ = ] {
        editItem->setFixedHeight(static_cast<int>(document->size().height()));
        int height = editItem->cursorRect(editItem->textCursor()).bottom();
        adjustVerticalScrollBar(editItem, height);
    });
    connect(editItem, &TextNoteEdit::sigFocusIn, this, &RightView::onTextEditFocusIn);
    connect(editItem, &TextNoteEdit::sigFocusOut, this, &RightView::onTextEditFocusOut);
    connect(editItem, &TextNoteEdit::sigDelEmpty, this, &RightView::onTextEditDelEmpty);
    connect(editItem, &TextNoteEdit::textChanged, this, &RightView::onTextEditTextChange);
    return  editItem;
}

QWidget *RightView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNoteItemOper noteOps(m_noteItemData);

    VNoteBlock *data = m_noteItemData->newBlock(VNoteBlock::Voice);

    data->ptrVoice->voiceSize = voiceSize;
    data->ptrVoice->voicePath = voicePath;
    data->ptrVoice->createTime = QDateTime::currentDateTime();
    data->ptrVoice->voiceTitle = noteOps.getDefaultVoiceName();

    VoiceNoteItem *item = new VoiceNoteItem(m_noteItemData, data->ptrVoice, this);
    item->setObjectName(VoiceWidget);
    connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
    connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
    connect(item, &VoiceNoteItem::voiceMenuShow, this, &RightView::onVoiceMenuShow);

    if (m_curItemWidget == nullptr) {
        m_viewportLayout->insertWidget(0, item);
        m_curItemWidget = item;
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        textBlock->blockText = "";
        m_curItemWidget = insertTextEdit(textBlock, true);
        m_noteItemData->addBlock(nullptr, textBlock);
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
                m_noteItemData->addBlock(preData, data);
                m_curItemWidget = itemWidget;
                itemWidget->setFocus();
                int height = itemWidget->cursorRect(itemWidget->textCursor()).bottom();
                adjustVerticalScrollBar(itemWidget, height);
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
        m_noteItemData->addBlock(data);
        m_curItemWidget = item;
        if (!cutStr.isEmpty() || curIndex + 1 == m_viewportLayout->indexOf(m_placeholderWidget)) {
            VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
            textBlock->blockText = cutStr;
            m_curItemWidget = insertTextEdit(textBlock, true);
            m_noteItemData->addBlock(item->getNoteBlock(), textBlock);
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
    int size = m_noteItemData->datas.dataConstRef().size();
    if (size) {
        for (auto it : m_noteItemData->datas.dataConstRef()) {
            if (VNoteBlock::Text == it->getType()) {
                m_curItemWidget = insertTextEdit(it, true);
            } else if (VNoteBlock::Voice == it->getType()) {
                VoiceNoteItem *item = new VoiceNoteItem(m_noteItemData, it->ptrVoice, this);
                item->setObjectName(VoiceWidget);
                int preIndex = -1;
                if (m_curItemWidget != nullptr) {
                    preIndex = m_viewportLayout->indexOf(m_curItemWidget);
                }
                m_viewportLayout->insertWidget(preIndex + 1, item);
                m_curItemWidget = item;
                connect(item, &VoiceNoteItem::sigPlayBtnClicked, this, &RightView::onVoicePlay);
                connect(item, &VoiceNoteItem::sigPauseBtnClicked, this, &RightView::onVoicePause);
                connect(item, &VoiceNoteItem::voiceMenuShow, this, &RightView::onVoiceMenuShow);
            }
        }
    } else {
        VNoteBlock *textBlock = m_noteItemData->newBlock(VNoteBlock::Text);
        m_curItemWidget = insertTextEdit(textBlock, true);
        m_noteItemData->addBlock(nullptr, textBlock);
    }
}

void RightView::onVoicePlay(VoiceNoteItem *item)
{
    if (!checkFileExist(item->getNoteBlock()->voicePath)) {
        delWidget(item);
        return;
    }
    for (int i = 0; i < m_viewportLayout->count(); i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *tmpWidget = static_cast< VoiceNoteItem *>(widget);
            tmpWidget->showPlayBtn();
        }
    }
    item->showPauseBtn();
    emit sigVoicePlay(item->getNoteBlock());
}

void RightView::onVoicePause(VoiceNoteItem *item)
{
    item->showPlayBtn();
    emit sigVoicePause(item->getNoteBlock());
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

            //Notify middle view refresh.
            emit contentChanged();
        }
    }
}

void RightView::onVoiceMenuShow()
{
    m_menuVoice = static_cast<VoiceNoteItem *>(sender());
    if (!checkFileExist(m_menuVoice->getNoteBlock()->voicePath)) {
        delWidget(m_menuVoice);
        return;
    }
    m_voiceContextMenu->exec(QCursor::pos());
}

/**
 * @brief RightView::onDltSelectContant
 * 删除选择的文本内容及textEdit控件
 */
void RightView::onDltSelectContant()
{
    if (m_selectTextItem.count() > 0) {
        foreach (auto textedit, m_selectTextItem) {
            if (textedit) {
                QTextCursor cursor = textedit->textCursor();
                if (cursor.hasSelection()) {
                    cursor.removeSelectedText();
                    cursor.clearSelection();
                    if (textedit->toPlainText() == "") {
                        textedit->deleteLater();
                    }
                }
            }
        }
    }
}

/**
 * @brief RightView::onPaste
 * 粘贴文本内容到editItem中
 */
void RightView::onPaste()
{
    if (m_curItemWidget) {
        if (m_curItemWidget->objectName() == TextEditWidget) {
            auto editItem = static_cast<TextNoteEdit *>(m_curItemWidget);
            auto textCursor = editItem->textCursor();
            QClipboard *board = QApplication::clipboard();
            if (board) {
                QString clipBoardText = board->text();
                textCursor.insertText(clipBoardText);
            }
        }
    }
}

/**
 * @brief RightView::onCut
 * 剪切选择内容
 */
void RightView::onCut()
{
    m_strSelectText = "";
    m_strSelectText = getSelectText();
    selectText2Clipboard();
    onDltSelectContant();
}

/**
 * @brief RightView::onSelectAll
 * 全选文本内容
 */
void RightView::onSelectAll()
{
    m_strSelectText = "";
    m_selectTextItem.clear();
    selectAllItem();
}

void RightView::delWidget(DWidget *widget)
{
    if (widget == nullptr) {
        return;
    }
    widget->disconnect();
    int index = m_viewportLayout->indexOf(widget);
    QWidget *preWidget = nullptr;
    if (index > 0) {
        preWidget = m_viewportLayout->itemAt(index - 1)->widget();
    }
    QWidget *nextWidget = m_viewportLayout->itemAt(index + 1)->widget();
    if (m_curItemWidget == widget) {
        m_curItemWidget = nextWidget;
    }
    if (m_menuVoice == widget) {
        m_menuVoice = nullptr;
    }
    VNoteBlock *noteBlock = nullptr;
    QString objName = widget->objectName();
    if (objName == TextEditWidget) {
        noteBlock =  static_cast<TextNoteEdit *>(widget)->getNoteBlock();
    } else if (objName == VoiceWidget) {
        noteBlock =  static_cast<VoiceNoteItem *>(widget)->getNoteBlock();
    }
    if (noteBlock) {
        m_noteItemData->delBlock(noteBlock);
    }
    m_viewportLayout->removeWidget(widget);
    widget->deleteLater();
    if (preWidget && nextWidget //合并textedit窗口
            && preWidget->objectName() == TextEditWidget
            && nextWidget->objectName() == TextEditWidget) {
        TextNoteEdit *editPre = static_cast<TextNoteEdit *>(preWidget);
        TextNoteEdit *editNext = static_cast<TextNoteEdit *>(nextWidget);
        editPre->disconnect();
        noteBlock = editNext->getNoteBlock();
        noteBlock->blockText = editPre->toPlainText() + editNext->toPlainText();
        editNext->setPlainText(noteBlock->blockText);
        m_noteItemData->delBlock(editPre->getNoteBlock());
        m_viewportLayout->removeWidget(editPre);
        editPre->deleteLater();
        QTextCursor cursor = editNext->textCursor();
        cursor.movePosition(QTextCursor::End);
        editNext->setTextCursor(cursor);
        editNext->setFocus();
    }
    int height = 0;
    if (m_curItemWidget->objectName() == TextEditWidget) {
        TextNoteEdit *editCur = static_cast<TextNoteEdit *>(m_curItemWidget);
        QRect rc = editCur->cursorRect(editCur->textCursor());
        height = rc.bottom();
        editCur->setFocus();
    }
    adjustVerticalScrollBar(m_curItemWidget, height);
    m_fIsNoteModified = true;
    saveNote();
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

VoiceNoteItem *RightView::getVoiceItem(VNoteBlock *data)
{
    for (int i = 0; i < m_viewportLayout->count(); i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *tmpWidget = static_cast< VoiceNoteItem *>(widget);
            if (tmpWidget->getNoteBlock() == data) {
                return  tmpWidget;
            }
        }
    }
    return nullptr;
}

VoiceNoteItem *RightView::getMenuVoiceItem()
{
    return m_menuVoice;
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
    int tolHeight = defaultHeight;
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
    if (btn == Qt::RightButton) {
        qDebug() << getSelectText();
    } else if (btn == Qt::LeftButton) {
        clearAllSelection();
        m_strSelectText = "";
        m_curItemWidget = getWidgetByPos(event->pos());
    }
}

void RightView::mouseReleaseEvent(QMouseEvent *event)
{
    DWidget::mouseReleaseEvent(event);

    m_strSelectText = getSelectText();
}

QWidget *RightView::getWidgetByPos(const QPoint &pos)
{
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->geometry().contains(pos)) {
            return  widget;
        }
    }
    return  nullptr;
}

/**
 * @brief RightView::selectText2Clipboard
 * 将选择的text添加到系统剪切板
 */
void RightView::selectText2Clipboard()
{
//    QString strSelect = "";

//    m_strSelectText = getSelectText();
    if (m_strSelectText != "") {
        qInfo() << "    select text:" << m_strSelectText;
        QClipboard *board = QApplication::clipboard();
        if (board) {
            board->clear();
            board->setText(m_strSelectText);
            qInfo() << "    copy text:"  << board->text();
        }
    }
}

void RightView::mouseMoveSelect(QMouseEvent *event)
{
    QWidget *widget = getWidgetByPos(event->pos());
    if (widget) {
        int widgetIndex = m_viewportLayout->indexOf(widget);
        int curIndex = m_viewportLayout->indexOf(m_curItemWidget);
        QTextCursor::MoveOperation op = QTextCursor::NoMove;
        QLayoutItem *tmpItem = nullptr;
        if (widget != m_curItemWidget) {
            if (widgetIndex < curIndex) {
                op = QTextCursor::End;
                tmpItem = m_viewportLayout->itemAt(widgetIndex + 1);
            } else {
                op = QTextCursor::Start;
                tmpItem = m_viewportLayout->itemAt(widgetIndex - 1);
            }
        }
        if (widget->objectName() == TextEditWidget) {
            TextNoteEdit *editItem = static_cast<TextNoteEdit *>(widget);
            editItem->selectText(event->globalPos(), op);
        } else if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *voiceItem = static_cast<VoiceNoteItem *>(widget);
            if (!voiceItem->getNoteBlock()->blockText.isEmpty()) {
                TextNoteEdit *asrTextEdit = voiceItem->getAsrTextEdit();
                asrTextEdit->selectText(event->globalPos(), op);
            }
        }
        if (tmpItem != nullptr) {
            op = op == QTextCursor::Start ? QTextCursor::End : QTextCursor::Start;
            widget = tmpItem->widget();
            if (widget->objectName() == TextEditWidget) {
                TextNoteEdit *editItem = static_cast<TextNoteEdit *>(widget);
                if (widget != m_curItemWidget) {
                    QTextCursor cursor = editItem->textCursor();
                    editItem->selectAll();
                } else {
                    editItem->moveCursor(op, QTextCursor::KeepAnchor);
                }
            } else if (widget->objectName() == VoiceWidget) {
                VoiceNoteItem *voiceItem = static_cast<VoiceNoteItem *>(widget);
                if (!voiceItem->getNoteBlock()->blockText.isEmpty()) {
                    TextNoteEdit *asrTextEdit = voiceItem->getAsrTextEdit();
                    if (widget != m_curItemWidget) {
                        QTextCursor cursor = asrTextEdit->textCursor();
                        asrTextEdit->selectAll();
                    } else {
                        asrTextEdit->moveCursor(op, QTextCursor::KeepAnchor);
                    }
                }
            }
        }
    }
}

void RightView::clearAllSelection()
{
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->objectName() == TextEditWidget) {
            TextNoteEdit *editItem = static_cast<TextNoteEdit *>(widget);
            editItem->clearSelection();
        } else if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *voiceItem = static_cast<VoiceNoteItem *>(widget);
            if (!voiceItem->getNoteBlock()->blockText.isEmpty()) {
                TextNoteEdit *asrTextEdit = voiceItem->getAsrTextEdit();
                asrTextEdit->clearSelection();
            }
        }
    }
}

QString RightView::getSelectText()
{
    QString text = "";
    QTextCursor textCursor;
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->objectName() == TextEditWidget) {
            TextNoteEdit *editItem = static_cast<TextNoteEdit *>(widget);
            textCursor = editItem->textCursor();
            if (textCursor.hasSelection()) {
                text.append(textCursor.selectedText());
                m_selectTextItem.append(editItem);
            }
        } else if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *voiceItem = static_cast<VoiceNoteItem *>(widget);
            if (!voiceItem->getNoteBlock()->blockText.isEmpty()) {
                TextNoteEdit *asrTextEdit = voiceItem->getAsrTextEdit();
                textCursor = asrTextEdit->textCursor();
                if (textCursor.hasSelection()) {
                    text.append(textCursor.selectedText());
                    m_selectTextItem.append(asrTextEdit);
                }
            }
        }
    }
    return text;
}

void RightView::selectAllItem()
{
    for (int i = 0; i < m_viewportLayout->count() - 1 ; i++) {
        QWidget *widget = m_viewportLayout->itemAt(i)->widget();
        if (widget->objectName() == TextEditWidget) {
            TextNoteEdit *editItem = static_cast<TextNoteEdit *>(widget);
            editItem->selectAll();
            m_strSelectText += editItem->toPlainText();
            m_selectTextItem.append(editItem);
        } else if (widget->objectName() == VoiceWidget) {
            VoiceNoteItem *voiceItem = static_cast<VoiceNoteItem *>(widget);
            if (!voiceItem->getNoteBlock()->blockText.isEmpty()) {
                TextNoteEdit *asrTextEdit = voiceItem->getAsrTextEdit();
                asrTextEdit->selectAll();
                m_strSelectText += asrTextEdit->toPlainText();
                m_selectTextItem.append(asrTextEdit);
            }
        }
    }
}
