#include "foldertree.h"
#include "folderdelegate.h"
#include "foldertreecommon.h"

#include "common/vnoteforlder.h"
#include "common/actionmanager.h"

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
        pItem = FolderTreeCommon::createStandardItem(nullptr, FolderTreeCommon::NOTEPADROOT);
        m_pDataModel->insertRow(0, pItem);
    }
    m_notepadMenu = ActionManager::getNotepadRMenu(this);
    connect(m_notepadMenu, SIGNAL(triggered(QAction *)),
            this, SIGNAL(treeAction(QAction *)));
}

QStandardItem *FolderTree::getNotepadRoot()
{
    return m_pDataModel->item(0);
}

void FolderTree::mousePressEvent(QMouseEvent *event)
{
    DTreeView::mouseMoveEvent(event);
    if (event->button() == Qt::RightButton) {
        QPoint pos = mapToParent(event->pos());
        QModelIndex index = this->indexAt(pos);
        if (FolderTreeCommon::getStandardItemType(index) == FolderTreeCommon::NOTEPADITEM) {
            this->setCurrentIndex(index);
            m_notepadMenu->exec(event->globalPos());
        }
    }
}
