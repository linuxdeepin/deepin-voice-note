#include "leftview.h"

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
    m_folderTree->initNotepadRoot();
    m_folderTree->setItemsExpandable(false);
    m_folderTree->setIndentation(0);
    QModelIndex index = m_folderTree->getNotepadRoot();
    if (index.isValid()) {
        m_folderTree->expand(index);
        QModelIndex child = index.child(0, 0);
        m_folderTree->setCurrentIndex(child);
    }
}
void LeftView::initUI()
{
    initFolderTree();
    m_addNotepadBtn = new DPushButton(DApplication::translate("LeftView", "Add Notepad"), this);
    m_addNotepadBtn->setFixedSize(QSize(180, 36));
    QVBoxLayout *viewLayout = new QVBoxLayout;
    viewLayout->addWidget(m_folderTree, Qt::AlignHCenter);
    viewLayout->addSpacing(5);
    viewLayout->addWidget(m_addNotepadBtn, Qt::AlignHCenter);
    viewLayout->setContentsMargins(10, 0, 0, 10);
    this->setLayout(viewLayout);
}

void LeftView::initConnection()
{
    connect(m_folderTree, &DTreeView::clicked, this, &LeftView::onTreeItemClicked);
    connect(m_addNotepadBtn, &DPushButton::clicked, this, &LeftView::onAddNotepad);
}

void LeftView::onAddNotepad()
{
    m_folderTree->addNotepad();
    emit sigNotepadAdd();
}

void LeftView::onTreeItemClicked(const QModelIndex &index)
{
    if (index != m_folderLastIndex) {
        m_folderLastIndex = index;
        emit sigModeIndexChange(m_folderLastIndex, m_folderTree->getItemType(index));
        qDebug() << "index change";
    }
}

void LeftView::clearTreeSelection()
{
    m_folderTree->clearSelection();
}
