#include "standarditemcommon.h"

StandardItemCommon::StandardItemCommon()
{

}
QStandardItem *StandardItemCommon::createStandardItem(void *data, StandardItemType type)
{
    QStandardItem *pItem = new QStandardItem;
    pItem->setData(QVariant::fromValue(type), Qt::UserRole + 1);
    pItem->setData(QVariant::fromValue(data), Qt::UserRole + 2);
    return  pItem;
}

StandardItemCommon::StandardItemType StandardItemCommon::getStandardItemType(const QModelIndex &index)
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

void * StandardItemCommon::getStandardItemData(const QModelIndex &index)
{
    if(index.isValid()){
        QVariant var = index.data(Qt::UserRole + 2);
        if (var.isValid()) {
            return  var.value<void *>();
        }
    }
    return nullptr;
}
