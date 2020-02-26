#include "foldertreecommon.h"

FolderTreeCommon::FolderTreeCommon()
{

}
QStandardItem *FolderTreeCommon::createStandardItem(void *data, StandardItemType type)
{
    QStandardItem *pItem = new QStandardItem;
    pItem->setData(QVariant::fromValue(type), Qt::UserRole + 1);
    pItem->setData(QVariant::fromValue(data), Qt::UserRole + 2);
    return  pItem;
}

FolderTreeCommon::StandardItemType FolderTreeCommon::getStandardItemType(const QModelIndex &index)
{
    StandardItemType type = Invalid;
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 1);
        if (var.isValid()) {
            return  var.value<StandardItemType>();
        }
    }
    return  type;
}

void * FolderTreeCommon::getStandardItemData(const QModelIndex &index)
{
    if(index.isValid()){
        QVariant var = index.data(Qt::UserRole + 2);
        if (var.isValid()) {
            return  var.value<void *>();
        }
    }
    return nullptr;
}
