#include "foldertree.h"
#include "folderdelegate.h"
#include "common/vnoteforlder.h"
#include "db/vnotefolderoper.h"

#include <QList>
#include <QDebug>

FolderTree::FolderTree(QWidget *parent)
    : DTreeView(parent)
{
    initModel();
    initDelegate();
    initNotepadRoot();
}

void FolderTree::initModel()
{
    m_pDataModel = new QStandardItemModel(this);
    this->setModel(m_pDataModel);
}

void FolderTree::initDelegate()
{
    m_pItemDelegate = new FolderDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

void FolderTree::initNotepadRoot()
{
    QStandardItem *pItem = m_pDataModel->item(0);
    if (pItem == nullptr) {
        pItem = new QStandardItem;
        setItemType(pItem, NOTEPADROOT);
        pItem->setFlags(Qt::NoItemFlags);
        m_pDataModel->insertRow(0, pItem);
    }
}
void FolderTree::addNotepad(VNoteFolder *notepad)
{
    if (notepad) {
        QStandardItem *pItem = new QStandardItem;
        setItemType(pItem, NOTEPADITEM);
        setItemData(pItem, notepad);
        QStandardItem *pItemRoot = getNotepadRoot();
        if (pItemRoot) {
            pItemRoot->insertRow(0, pItem);
        }
    }
}

int FolderTree::addNotepads(VNOTE_FOLDERS_MAP *notepads)
{
    if (notepads != nullptr) {
        QList<QStandardItem *> items;
        notepads->lock.lockForRead();
        for (auto it : notepads->folders) {
            QStandardItem *pItem = new QStandardItem;
            setItemType(pItem, NOTEPADITEM);
            setItemData(pItem, it);
            items.push_back(pItem);
        }
        notepads->lock.unlock();
        QStandardItem *pItemRoot = getNotepadRoot();
        if (pItemRoot) {
            pItemRoot->appendRows(items);
            return  items.size();
        }
    }
    return  0;
}
QStandardItem *FolderTree::getNotepadRoot()
{
    return m_pDataModel->item(0);
}

VNoteFolder *FolderTree::getNotepadItemData(const QModelIndex &index) const
{
    VNoteFolder *data = nullptr;
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 2);
        if (var.isValid()) {
            data = static_cast<VNoteFolder *>(var.value<void *>());
        }
    }
    return  data;
}

FolderTree::ItemType FolderTree::getItemType(const QModelIndex &index) const
{
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 1);
        int value = var.toInt();
        if (value < ERRORTYPE) {
            return static_cast<ItemType>(value);
        }
    }
    return ERRORTYPE;
}

void FolderTree::setItemType(QStandardItem *pItem, ItemType type)
{
    if (pItem) {
        pItem->setData(QVariant(type), Qt::UserRole + 1);
    }
}

void FolderTree::setItemData(QStandardItem *pItem, void *data)
{
    if (pItem) {
        pItem->setData(QVariant::fromValue(data), Qt::UserRole + 2);
    }
}

void FolderTree::itemEditFinsh(const QModelIndex &index, const QString &newValue)
{
    FolderTree::ItemType type = getItemType(index);
    if (type == NOTEPADITEM) {
        VNoteFolder *data = getNotepadItemData(index);
        if (!newValue.isEmpty() && data && data->name != newValue) {
            VNoteFolderOper folderOper(data);
            if (!folderOper.renameVNoteFolder(newValue)) {
                qDebug() << "rename error";
            }
        }
    }
    this->update(this->currentIndex());
}
