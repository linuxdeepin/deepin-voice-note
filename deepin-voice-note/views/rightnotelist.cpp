#include "rightnotelist.h"
#include "common/vnoteitem.h"
#include "textnoteitem.h"

#include <DFontSizeManager>
#include <QDebug>
#include <QLabel>

RightNoteList::RightNoteList(QWidget *parent)
    : DListWidget(parent)
{
    initUI();
    initConnection();
}

void RightNoteList::initUI()
{
    ;
}

void RightNoteList::initConnection()
{
    ;
}

void RightNoteList::initNoteItem(qint64 folderid, VNOTE_ITEMS_MAP *mapNoteData, QString serachKey)
{
    while (this->count()) {
        QListWidgetItem *item = this->takeItem(0);
        DWidget *widgetTemp = static_cast<DWidget *>(this->itemWidget(item));
        delete item;
        item = nullptr;
        widgetTemp->deleteLater();
    }
    m_forlderId = folderid;
    if (mapNoteData != nullptr) {
        mapNoteData->lock.lockForRead();
        for (auto &it : mapNoteData->folderNotes) {
            addNodeItem(it, serachKey);
        }
        mapNoteData->lock.unlock();
    }
}

void RightNoteList::addNodeItem(VNoteItem *item, QString key, bool isBtnAdd)
{
    if (item->noteType == VNoteItem::VNOTE_TYPE::VNT_Text) {
        TextNoteItem *textItem = new TextNoteItem(item, this);
        textItem->highSearchText(key, QColor(0x349ae8));
        QListWidgetItem *item = new QListWidgetItem();
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(this->width(),  textItem->height()));
        this->insertItem(this->count(), item);
        this->setItemWidget(item, textItem);
        connect(textItem, SIGNAL(sigDelNote(VNoteItem *)), this, SIGNAL(sigDelNote(VNoteItem *)));
        connect(textItem, SIGNAL(sigUpdateNote(VNoteItem *)), this, SIGNAL(sigUpdateNote(VNoteItem *)));
        connect(textItem, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QString &)), this, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QString &)));
        connect(textItem, SIGNAL(sigTextEditIsEmpty(VNoteItem *, bool)), this, SIGNAL(sigTextEditIsEmpty(VNoteItem *, bool)));
        if (isBtnAdd) {
            textItem->changeToEdit();
        }
    }
    adjustWidgetItemWidth(this->count() - 1);
    this->scrollToBottom();
}

void RightNoteList::adjustWidgetItemWidth(int index)
{
    QListWidgetItem *ptmp = nullptr;
    int listWidth = this->width();
    int newWidth = this->width() - 20;
    for (int i = index; i < this->count(); i++) {
        ptmp = this->item(i);
        QWidget *ptmpWidget = this->itemWidget(ptmp);
        ptmpWidget->setFixedSize(QSize(newWidth, ptmpWidget->height()));
        this->item(i)->setSizeHint(QSize(listWidth, ptmpWidget->height()));
    }
}

void RightNoteList::delNodeItem(VNoteItem *item)
{
    for (int i = 0; i < this->count(); i++) {
        if (item->noteType == VNoteItem::VNOTE_TYPE::VNT_Text) {
            TextNoteItem *itemWidget = static_cast<TextNoteItem *>(this->itemWidget(this->item(i)));
            if (itemWidget->getNoteItem() == item) {
                QListWidgetItem *tmpItem = this->takeItem(i);
                delete tmpItem;
                itemWidget->deleteLater();
                return;
            }
        }
    }
}

void RightNoteList::resizeEvent(QResizeEvent *event)
{
    DListWidget::resizeEvent(event);
    adjustWidgetItemWidth(0);
}

int RightNoteList::getListHeight()
{
    int height = 0;
    for (int i = 0; i < this->count(); i++) {
        height += this->item(i)->sizeHint().height();
    }
    return height;
}

qint64 RightNoteList::getFolderId()
{
    return m_forlderId;
}

void RightNoteList::onAddHeight(int height)
{
    QWidget *senderWidget = static_cast<QWidget *>(sender());
    for (int i = 0; i < this->count(); i++) {
        QListWidgetItem *pTmpItem = this->item(i);
        QWidget *pTmpwidget = this->itemWidget(pTmpItem);
        if (senderWidget == pTmpwidget) {
            this->item(i)->setSizeHint(QSize(this->width(), pTmpwidget->height() + height));
            return;
        }
    }
}

void RightNoteList::updateNodeItem(VNoteItem *item)
{
    for (int i = 0; i < this->count(); i++) {
        TextNoteItem *itemWidget = static_cast<TextNoteItem *>(this->itemWidget(this->item(i)));
        if (itemWidget->getNoteItem() == item) {
            itemWidget->updateData();
        }
    }
}
