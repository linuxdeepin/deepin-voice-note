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

void RightNoteList::initNoteItem(qint64 folderid, VNOTE_ITEMS_MAP *mapNoteData, const QRegExp &searchKey)
{
    while (this->count()) {
        QListWidgetItem *item = this->takeItem(0);
        VNoteItemWidget *widgetTemp = static_cast<VNoteItemWidget *>(this->itemWidget(item));
        delete item;
        item = nullptr;
        widgetTemp->deleteLater();
    }
    m_forlderId = folderid;
    if (mapNoteData != nullptr) {
        mapNoteData->lock.lockForRead();
        for (auto &it : mapNoteData->folderNotes) {
            addNodeItem(it, searchKey);
        }
        mapNoteData->lock.unlock();
    }
}

void RightNoteList::addNodeItem(VNoteItem *item, const QRegExp &searchKey, bool isBtnAdd)
{
    if (item->noteType == VNoteItem::VNOTE_TYPE::VNT_Text) {
        TextNoteItem *textItem = new TextNoteItem(item, this);
        textItem->setFixedHeight(170);
        textItem->highSearchText(searchKey, QColor(0x349ae8));
        QListWidgetItem *item = new QListWidgetItem();
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(this->width(),  textItem->height()));
        this->insertItem(this->count(), item);
        this->setItemWidget(item, textItem);
        connect(textItem, SIGNAL(sigMenuPopup(VNoteItem *)), this, SIGNAL(sigMenuPopup(VNoteItem *)));
        connect(textItem, SIGNAL(sigDelNote(VNoteItem *)), this, SIGNAL(sigDelNote(VNoteItem *)));
        connect(textItem, SIGNAL(sigUpdateNote(VNoteItem *)), this, SIGNAL(sigUpdateNote(VNoteItem *)));
        connect(textItem, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)), this, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)));
        connect(textItem, SIGNAL(sigTextEditIsEmpty(VNoteItem *, bool)), this, SIGNAL(sigTextEditIsEmpty(VNoteItem *, bool)));
        if (isBtnAdd) {
            textItem->changeToEdit();
        }
    } else {
        VoiceNoteItem *voiceItem = new VoiceNoteItem(item, nullptr, this);
        voiceItem->setFixedHeight(95);
        QListWidgetItem *item = new QListWidgetItem();
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(this->width(),  voiceItem->height()));
        this->insertItem(this->count(), item);
        this->setItemWidget(item, voiceItem);
        connect(voiceItem, SIGNAL(sigMenuPopup(VNoteItem *)), this, SIGNAL(sigMenuPopup(VNoteItem *)));
        connect(voiceItem, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)), this,
                SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)));
        connect(voiceItem, SIGNAL(sigVoicePlayBtnClicked(VoiceNoteItem *)),
                this, SIGNAL(sigVoicePlayBtnClicked(VoiceNoteItem *)));
        connect(voiceItem, SIGNAL(sigVoicePauseBtnClicked(VoiceNoteItem *)),
                this, SIGNAL(sigVoicePauseBtnClicked(VoiceNoteItem *)));
        //connect(voiceItem, SIGNAL(sigItemAddHeight(int)), this, SLOT(onItemAddHeight(int)));
        //connect(voiceItem, SIGNAL(sigVoicePlayPosChange(int)), this, SIGNAL(sigVoicePlayPosChange(int)));
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
        VNoteItemWidget *ptmpWidget = static_cast<VNoteItemWidget *>(this->itemWidget(ptmp));
        ptmpWidget->setFixedSize(QSize(newWidth, ptmpWidget->height()));
        this->item(i)->setSizeHint(QSize(listWidth, ptmpWidget->height()));
    }
}

void RightNoteList::delNodeItem(VNoteItem *item)
{
    for (int i = 0; i < this->count(); i++) {
        VNoteItemWidget *itemWidget = static_cast<VNoteItemWidget *>(this->itemWidget(this->item(i)));
        if (itemWidget->getNoteItem() == item) {
            QListWidgetItem *tmpItem = this->takeItem(i);
            delete tmpItem;
            itemWidget->deleteLater();
            return;
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

void RightNoteList::onItemAddHeight(int height)
{
    VNoteItemWidget *senderWidget = static_cast<VNoteItemWidget *>(sender());
    for (int i = 0; i < this->count(); i++) {
        QListWidgetItem *pTmpItem = this->item(i);
        VNoteItemWidget *pTmpwidget = static_cast<VNoteItemWidget *>(this->itemWidget(pTmpItem));
        if (senderWidget == pTmpwidget) {
            pTmpwidget->setFixedHeight(pTmpwidget->height() + height);
            this->item(i)->setSizeHint(QSize(this->width(), pTmpwidget->height()));
            emit sigListHeightChange();
            if(i == this->count() - 1){
                this->scrollToBottom();
            }
            return;
        }
    }
}

void RightNoteList::updateNodeItem(VNoteItem *item)
{
    for (int i = 0; i < this->count(); i++) {
        VNoteItemWidget *itemWidget = static_cast<VNoteItemWidget *>(this->itemWidget(this->item(i)));
        if (itemWidget->getNoteItem() == item) {
            itemWidget->updateData();
            break;
        }
    }
}

VoiceNoteItem *RightNoteList::getVoiceItem(VNoteItem *item)
{
    return nullptr;
}

void RightNoteList::setVoicePlayEnable(bool enable)
{

}

bool RightNoteList::hasSearchNote(QRegExp &searchKey)
{
    for (int i = 0; i < this->count(); i++) {
        VNoteItemWidget *itemWidget = static_cast<VNoteItemWidget *>(this->itemWidget(this->item(i)));
        VNoteItem *itemdata = itemWidget->getNoteItem();
        if(itemdata && itemdata->noteType == VNoteItem::VNT_Text){
//            if(itemdata->metaData.contains(searchKey)){
//                return true;
//            }
        }
    }
    return false;
}
