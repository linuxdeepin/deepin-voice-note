#include "leftview.h"
#include "leftviewdelegate.h"
#include "common/actionmanager.h"
#include "common/standarditemcommon.h"
#include "common/vnoteforlder.h"

#include <QMouseEvent>

LeftView::LeftView(QWidget *parent)
    : DTreeView(parent)
{
    initModel();
    initDelegate();
    initNotepadRoot();
    initMenu();
}

void LeftView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);
    this->setModel(m_pDataModel);
}

void LeftView::initDelegate()
{
    m_pItemDelegate = new LeftViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

void LeftView::initNotepadRoot()
{
    QStandardItem *pItem = m_pDataModel->item(0);
    if (pItem == nullptr) {
        pItem = StandardItemCommon::createStandardItem(nullptr, StandardItemCommon::NOTEPADROOT);
        m_pDataModel->insertRow(0, pItem);
    }
}

QStandardItem *LeftView::getNotepadRoot()
{
    return m_pDataModel->item(0);
}

void LeftView::mousePressEvent(QMouseEvent *event)
{
    this->setFocus();

    if (!m_onlyCurItemMenuEnable) {
        DTreeView::mouseMoveEvent(event);
    }

    if (event->button() == Qt::RightButton) {
        QModelIndex index = this->indexAt(event->pos());
        if (StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM
                && (!m_onlyCurItemMenuEnable || index == this->currentIndex())) {
            this->setCurrentIndex(index);
            m_notepadMenu->exec(event->globalPos());
        }
    }
}
void LeftView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DTreeView::mouseReleaseEvent(event);
    }
}

void LeftView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DTreeView::mouseDoubleClickEvent(event);
    }
}

void LeftView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DTreeView::mouseMoveEvent(event);
    }
}

QModelIndex LeftView::setDefaultNotepadItem()
{
    QModelIndex index = this->currentIndex();
    QItemSelectionModel *model = this->selectionModel();
    if (index.isValid() && StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM) {
        if (!model->isSelected(index)) {
            this->setCurrentIndex(index);
        }
    } else {
        QStandardItem *item = getNotepadRoot();
        QStandardItem *itemChild = item->child(0);
        if (itemChild) {
            index = itemChild->index();
            this->setCurrentIndex(index);
        }
    }
    return index;
}

void LeftView::addFolder(VNoteFolder *folder)
{
    if (nullptr != folder) {
        QStandardItem *pItem = StandardItemCommon::createStandardItem(
                    folder, StandardItemCommon::NOTEPADITEM);

        QStandardItem *root = getNotepadRoot();
        root->insertRow(0, pItem);
        setCurrentIndex(pItem->index());
    }
}

void LeftView::appendFolder(VNoteFolder *folder)
{
    if (nullptr != folder) {
        QStandardItem *pItem = StandardItemCommon::createStandardItem(
                    folder, StandardItemCommon::NOTEPADITEM);

        QStandardItem *root = getNotepadRoot();

        if (nullptr != root) {
            root->appendRow(pItem);
        }
    }
}

void LeftView::editFolder()
{
    edit(currentIndex());
}

VNoteFolder* LeftView::removeFolder()
{

    QModelIndex index = currentIndex();

    VNoteFolder *data = reinterpret_cast<VNoteFolder*>(
                StandardItemCommon::getStandardItemData(index));

    m_pDataModel->removeRow(index.row(), index.parent());

    return data;
}

int LeftView::folderCount()
{
    int count = 0;

    QStandardItem *root = getNotepadRoot();

    if (nullptr != root) {
        count = root->rowCount();
    }

    return count;
}

void  LeftView::initMenu()
{
    m_notepadMenu = ActionManager::Instance()->notebookContextMenu();
}

void LeftView::setOnlyCurItemMenuEnable(bool enable)
{
    m_onlyCurItemMenuEnable = enable;
    m_pItemDelegate->setEnableItem(!enable);
    m_pDataModel->layoutChanged();
}
