#include "notelistview.h"
#include "common/vnoteforlder.h"
#include "db/vnotefolderoper.h"

#include <QDebug>

static VNoteFolderOper folderOper;

NoteListView::NoteListView(QWidget *parent)
    : DListView(parent)
{
    initModel();
    initDelegate();
    initConnection();
}

void NoteListView::initDelegate()
{
    m_pItemDelegate = new NoteItemDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

void NoteListView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);
    this->setModel(m_pDataModel);
}

void NoteListView::initConnection()
{
    ;
}

const QStandardItem *NoteListView::addNoteItem(VNoteItem *data)
{
    QStandardItem *pItem = new QStandardItem;
    pItem->setData(QVariant::fromValue(static_cast<void *>(data)), Qt::UserRole + 1);
    m_pDataModel->insertRow(0, pItem);
    return  pItem;
}

void NoteListView::removeNoteItem(VNoteItem *data)
{
    QModelIndex index = getItemIndex(data);
    m_pDataModel->removeRow(index.row());
}

void NoteListView::clearAllItems()
{
    m_pDataModel->removeRows(0, m_pDataModel->rowCount());
}

void NoteListView::initNoteData(VNOTE_ITEMS_MAP *data, const QRegExp &searchKey)
{
    clearAllItems();
    m_pItemDelegate->setSearchKey(searchKey);
    if (data) {
        data->lock.lockForRead();
        for (auto it : data->folderNotes) {
            QStandardItem *pItem = new QStandardItem;
            pItem->setData(QVariant::fromValue(static_cast<void *>(it)), Qt::UserRole + 1);
            m_pDataModel->appendRow(pItem);
        }
        data->lock.unlock();
        QModelIndex index = m_pDataModel->index(0, 0);
        this->setCurrentIndex(index);
    }

}

QModelIndex NoteListView::getItemIndex(const VNoteItem *data) const
{
    QModelIndex index;
    for (int i = 0; i < m_pDataModel->rowCount(); i++) {
        QModelIndex tmpIndex = m_pDataModel->item(i)->index();
        VNoteItem *value = getItemData(tmpIndex);
        if (value == data) {
            break;
        }
    }
    return  index;
}

VNoteItem *NoteListView::getItemData(const QModelIndex &index) const
{
    VNoteItem *value = nullptr;
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 1);
        if (var.isValid()) {
            value = static_cast<VNoteItem *>(var.value<void *>());
        }
    }
    return  value;
}

void NoteListView::removeCurItem()
{
    QModelIndex index = this->currentIndex();
    m_pDataModel->removeRow(index.row());
}

void NoteListView::itemEditFinsh(const QModelIndex &index,
                                 const QString &newValue)
{
    VNoteItem *value = getItemData(index);
    if(value){
        qDebug() << "finsh value:" << newValue;
    }
}

VNoteFolder *NoteListView::getFolderData(qint64 id)
{
    return folderOper.getFolder(id);
}
