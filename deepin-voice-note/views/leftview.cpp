#include "leftview.h"
#include "leftviewdelegate.h"
#include "common/actionmanager.h"
#include "common/standarditemcommon.h"

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
    if (m_isEnable) {
        DTreeView::mouseMoveEvent(event);
        if (event->button() == Qt::RightButton) {
            QPoint pos = mapToParent(event->pos());
            QModelIndex index = this->indexAt(pos);
            if (StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM) {
                this->setCurrentIndex(index);
                m_notepadMenu->exec(event->globalPos());
            }
        }
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

void  LeftView::initMenu()
{
    m_notepadMenu = ActionManager::Instance()->notebookContextMenu();
}
