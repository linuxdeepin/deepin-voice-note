#include "rightnotelist.h"
#include "textnoteitem.h"
#include "common/vnoteitem.h"

#include<QDebug>
#include<QLabel>
#include <DFontSizeManager>

RightNoteList::RightNoteList(qint64 folderId,QWidget *parent)
    : DListWidget(parent)
    ,m_folderId(folderId)
{
    initUI();
    initConnection();
    //loadNoteItem();
}
qint64 RightNoteList::getFolderId()
{
    return m_folderId;
}
void RightNoteList::initUI()
{
   ;
}
void RightNoteList::initConnection()
{
   ;
}
void RightNoteList::loadNoteItem()
{
    m_mapNoteData = new VNOTE_ITEMS_MAP();
}
void RightNoteList::addTextNodeItem()
{
    QSharedPointer<VNoteItem> item (new  VNoteItem());
    item->noteId = 0;
    item->noteType = VNoteItem::VNOTE_TYPE::VNT_Text;
    item->folderId = m_folderId;
    item->createTime = QDateTime::currentDateTime();
    addNodeItem(item.get());
    data.push_back(item);
}
void RightNoteList::addNodeItem(VNoteItem *item)
{
    if(item->noteType == VNoteItem::VNOTE_TYPE::VNT_Text)
    {
        TextNoteItem *textItem = new TextNoteItem(item,this);
        QListWidgetItem *item=new QListWidgetItem();
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(this->width(),170));
        this->insertItem(this->count(),item);
        this->setItemWidget(item,textItem);
        adjustWidgetItemWidth();
        m_height += 170;
    }
}
void RightNoteList::adjustWidgetItemWidth()
{
    QListWidgetItem *ptmp = nullptr;
    int listWidth = this->width();
    int newWidth = this->width() - 18;
    for(int i = 0; i < this->count(); i++)
    {
        ptmp = this->item(i);
        QWidget* ptmpWidget = this->itemWidget(ptmp);
        ptmpWidget->setFixedSize(QSize(newWidth,ptmpWidget->height()));
        this->item(i)->setSizeHint(QSize(listWidth,ptmpWidget->height()));
    }
}
void RightNoteList::resizeEvent(QResizeEvent *event)
{
    DListWidget::resizeEvent(event);
    adjustWidgetItemWidth();
}
qint64 RightNoteList::getHeight()
{
    return  m_height;
}
