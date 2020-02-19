#include "foldertree.h"

FolderTree::FolderTree(QWidget *parent)
    : DTreeView(parent)
{
    initModel();
    initDelegate();
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
    QStandardItem *pItem = m_pDataModel->item(FolderTree::Notepad);
    if(pItem == nullptr){
        pItem = new QStandardItem;
        pItem->setData(QVariant(FolderDelegate::NOTEROOT), Qt::UserRole + 1);
        pItem->setFlags(Qt::NoItemFlags);
        m_pDataModel->insertRow(FolderTree::Notepad, pItem);
    }
}
void FolderTree::addNotepad()
{
    QStandardItem *pItem = new QStandardItem;
    pItem->setData(QVariant(FolderDelegate::NOTEITEM), Qt::UserRole + 1);
    m_pDataModel->item(FolderTree::Notepad)->insertRow(0,pItem);
    QModelIndex rootIndex = getNotepadRoot();
    this->setCurrentIndex(rootIndex.child(0,0));
}

QModelIndex FolderTree::getNotepadRoot()
{
    return m_pDataModel->index(FolderTree::Notepad, 0);
}

FolderDelegate::ItemType FolderTree::getItemType(const QModelIndex &index) const
{
    return  m_pItemDelegate->getItemType(index);
}
