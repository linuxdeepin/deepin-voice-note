#include "leftview.h"
#include "common/vnoteforlder.h"
#include "common/vnotedatamanager.h"

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
    connect(m_folderTree, &DTreeView::clicked, this, &LeftView::onTreeItemClicked);
    connect(m_addNotepadBtn, &DPushButton::clicked, this, &LeftView::onAddNotepad);
}

void LeftView::onAddNotepad()
{
    VNoteFolderOper folderOper;
    VNoteFolder itemData;
    itemData.defaultIcon = folderOper.getDefaultIcon();
    itemData.UI.icon = folderOper.getDefaultIcon(itemData.defaultIcon);
    itemData.name = folderOper.getDefaultFolderName();
    VNoteFolder *newFolder = folderOper.addFolder(itemData);
    m_folderTree->addNotepad(newFolder);
    selectDefaultItem();
    m_folderTree->edit(m_folderLastIndex);
}

void LeftView::onTreeItemClicked(const QModelIndex &index)
{
    if (index != m_folderLastIndex) {
        m_folderLastIndex = index;
        emit sigFolderChange(m_folderLastIndex);
    }
}

void LeftView::clearTreeSelection()
{
    m_folderTree->clearSelection();
    m_folderLastIndex = QModelIndex();
}

void LeftView::updateCurFolder()
{
    m_folderTree->update(m_folderTree->currentIndex());
}

int LeftView::loadNoteFolder()
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    int ret = m_folderTree->addNotepads(folders);
    selectDefaultItem();
    return ret;
}

VNoteFolder *LeftView::getNotepadItemData(const QModelIndex &index) const
{
    return m_folderTree->getNotepadItemData(index);
}

void LeftView::selectDefaultItem()
{
    QStandardItem *item = m_folderTree->getNotepadRoot();
    if (item) {
        QStandardItem *itemChild = item->child(0);
        if (itemChild) {
            m_folderTree->setCurrentIndex(itemChild->index());
            m_folderLastIndex = m_folderTree->currentIndex();
            emit sigFolderChange(m_folderLastIndex);
        }
    }
}
