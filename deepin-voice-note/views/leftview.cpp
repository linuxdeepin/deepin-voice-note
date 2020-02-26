#include "leftview.h"
#include "foldertreecommon.h"

#include "common/vnoteforlder.h"
#include "common/vnotedatamanager.h"
#include "common/actionmanager.h"
#include "db/vnotefolderoper.h"

#include<QVBoxLayout>
#include<QDebug>

#include<DApplication>

LeftView::LeftView(QWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnection();
}

void LeftView::initFolderTree()
{
    m_folderTree = new FolderTree(this);
    m_folderTree->setHeaderHidden(true);
    m_folderTree->setFrameShape(QFrame::NoFrame);
    m_folderTree->setItemsExpandable(false);
    m_folderTree->setIndentation(0);
    QStandardItem *notepadRoot = m_folderTree->getNotepadRoot();
    if (notepadRoot) {
        m_folderTree->expand(notepadRoot->index());
        QStandardItem *firstChild = notepadRoot->child(0);
        if (firstChild) {
            m_folderTree->setCurrentIndex(firstChild->index());
        }
    }
}
void LeftView::initUI()
{
    initFolderTree();
    m_addNotepadBtn = new DPushButton(DApplication::translate("LeftView", "Add Notepad"), this);
    m_addNotepadBtn->setFixedSize(QSize(180, 36));
    QVBoxLayout *viewLayout = new QVBoxLayout;
    viewLayout->addWidget(m_folderTree, Qt::AlignHCenter);
    QVBoxLayout *btnLayout = new QVBoxLayout;
    btnLayout->addWidget(m_addNotepadBtn);
    btnLayout->setContentsMargins(10, 0, 0, 10);
    viewLayout->addLayout(btnLayout, Qt::AlignHCenter);
    viewLayout->setContentsMargins(0, 5, 0, 0);
    this->setLayout(viewLayout);
}

void LeftView::initConnection()
{
    connect(m_folderTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &LeftView::onTreeSelectChange);
    connect(m_folderTree, &FolderTree::treeAction, this, &LeftView::onTreeAction);
    connect(m_addNotepadBtn, &DPushButton::clicked, this, &LeftView::onAddNotepad);
}

void LeftView::onAddNotepad()
{
    VNoteFolder itemData;
    VNoteFolderOper  folderOper;
    itemData.defaultIcon = folderOper.getDefaultIcon();
    itemData.UI.icon = folderOper.getDefaultIcon(itemData.defaultIcon);
    itemData.name = folderOper.getDefaultFolderName();
    VNoteFolder *newFolder = folderOper.addFolder(itemData);
    if (newFolder) {
        QStandardItem *item = FolderTreeCommon::createStandardItem(newFolder, FolderTreeCommon::NOTEPADITEM);
        m_folderTree->getNotepadRoot()->insertRow(0, item);
        m_folderTree->setCurrentIndex(item->index());
        m_folderTree->edit(item->index());
    }
}

void LeftView::onTreeSelectChange(const QModelIndex &current, const QModelIndex &previous)
{
    if (m_folderTree->selectionMode() != QAbstractItemView::NoSelection) {
        m_folderLastIndex = current;
        emit sigFolderChange(current);
    } else {
        emit sigFolderChange(QModelIndex());
    }
}

void LeftView::clearSelection()
{
    if (m_folderLastIndex == m_folderTree->currentIndex()) {
        if (m_folderTree->selectionModel()->isSelected(m_folderLastIndex)) {
            m_folderTree->clearSelection();
        }
    }
}

void LeftView::updateCurFolder()
{
    m_folderTree->update(m_folderTree->currentIndex());
}

int LeftView::loadNoteFolder()
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        QList<QStandardItem *> items;
        folders->lock.lockForRead();
        for (auto it : folders->folders) {
            QStandardItem *pItem = FolderTreeCommon::createStandardItem(it, FolderTreeCommon::NOTEPADITEM);
            items.push_back(pItem);
        }
        folders->lock.unlock();
        QStandardItem *pItemRoot = m_folderTree->getNotepadRoot();
        if (pItemRoot) {
            pItemRoot->appendRows(items);
            selectDefaultItem();
            return  items.size();
        }
    }
    return 0;
}

void LeftView::selectDefaultItem()
{
    if (m_folderLastIndex.isValid() && m_folderTree->currentIndex() == m_folderLastIndex) {
        QItemSelectionModel *model = m_folderTree->selectionModel();
        if(!model->isSelected(m_folderLastIndex)){
            model->select(m_folderLastIndex,QItemSelectionModel::SelectCurrent);
            emit sigFolderChange(m_folderLastIndex);
        }
        return;
    }
    QStandardItem *item = m_folderTree->getNotepadRoot();
    if (item) {
        QStandardItem *itemChild = item->child(0);
        if (itemChild) {
            m_folderTree->setCurrentIndex(itemChild->index());
            emit sigFolderChange(itemChild->index());
        }
    }
}

void LeftView::delFolderItem(const QModelIndex &index)
{
    if (index == m_folderLastIndex) {
        m_folderLastIndex = QModelIndex();
    }
    m_folderTree->model()->removeRow(index.row(), index.parent());
}

void LeftView::onTreeAction(QAction *action)
{
    ActionManager::ActionKind kind = ActionManager::getActionKind(action);
    switch (kind) {
    case ActionManager::NotepadRename:
        m_folderTree->edit(m_folderTree->currentIndex());
        break;
    default:
        emit sigFolderAction(m_folderTree->currentIndex(), action);
        break;
    }
}
