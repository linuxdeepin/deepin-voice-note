/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "leftview.h"
#include "leftviewdelegate.h"
#include "leftviewsortfilter.h"

#include "globaldef.h"

#include "common/actionmanager.h"
#include "common/standarditemcommon.h"
#include "common/vnoteforlder.h"
#include "common/setting.h"

#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDrag>
#include <QMimeData>
#include <QScrollBar>
#include <QApplication>

/**
 * @brief LeftView::LeftView
 * @param parent
 */
LeftView::LeftView(QWidget *parent)
    : DTreeView(parent)
{
    initModel();
    initDelegate();
    initNotepadRoot();
    initMenu();
    this->setDragEnabled(true);
    this->setDragDropMode(QAbstractItemView::DragOnly);
    this->setDropIndicatorShown(true);
    this->setAcceptDrops(true);
}

/**
 * @brief LeftView::initModel
 */
void LeftView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);
    m_pSortViewFilter = new LeftViewSortFilter(this);
    m_pSortViewFilter->setDynamicSortFilter(false);
    m_pSortViewFilter->setSourceModel(m_pDataModel);
    this->setModel(m_pSortViewFilter);
}

/**
 * @brief LeftView::initDelegate
 */
void LeftView::initDelegate()
{
    m_pItemDelegate = new LeftViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

/**
 * @brief LeftView::initNotepadRoot
 */
void LeftView::initNotepadRoot()
{
    QStandardItem *pItem = m_pDataModel->item(0);
    if (pItem == nullptr) {
        pItem = StandardItemCommon::createStandardItem(nullptr, StandardItemCommon::NOTEPADROOT);
        m_pDataModel->insertRow(0, pItem);
    }
}

/**
 * @brief LeftView::getNotepadRoot
 * @return 记事本项父节点
 */
QStandardItem *LeftView::getNotepadRoot()
{
    return m_pDataModel->item(0);
}

/**
 * @brief LeftView::getNotepadRootIndex
 * @return 记事本父节点索引
 */
QModelIndex LeftView::getNotepadRootIndex()
{
    return m_pSortViewFilter->mapFromSource(getNotepadRoot()->index());
}

/**
 * @brief LeftView::mousePressEvent
 * @param event
 */
void LeftView::mousePressEvent(QMouseEvent *event)
{
    this->setFocus();

    if (!m_onlyCurItemMenuEnable) {
        event->setModifiers(Qt::NoModifier);
        DTreeView::mouseMoveEvent(event);
    }

    if (event->button() == Qt::RightButton) {
        QModelIndex index = this->indexAt(event->pos());
        if (StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM
            && (!m_onlyCurItemMenuEnable || index == this->currentIndex())) {
            this->setCurrentIndex(index);
            m_notepadMenu->popup(event->globalPos());
        }
    }

    if (event->button() == Qt::LeftButton) {
        startPos = event->pos();
        m_index = this->indexAt(startPos);
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        drag->setMimeData(mimeData);
        drag->exec(Qt::MoveAction);
    }
}

/**
 * @brief LeftView::mouseReleaseEvent
 * @param event
 */
void LeftView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DTreeView::mouseReleaseEvent(event);
    }
}

/**
 * @brief LeftView::mouseDoubleClickEvent
 * @param event
 */
void LeftView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DTreeView::mouseDoubleClickEvent(event);
    }
}

/**
 * @brief LeftView::mouseMoveEvent
 * @param event
 */
void LeftView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if ((event->pos() - startPos).manhattanLength() < QApplication::startDragDistance()) {
            DTreeView::mouseMoveEvent(event);
            return;
        }
    }
}

/**
 * @brief LeftView::keyPressEvent
 * @param e
 */
void LeftView::keyPressEvent(QKeyEvent *e)
{
    if (m_onlyCurItemMenuEnable || e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
        e->ignore();
    } else {
        if (0 == this->currentIndex().row() && e->key() == Qt::Key_Up) {
            e->ignore();
        } else {
            DTreeView::keyPressEvent(e);
        }
    }
}

/**
 * @brief LeftView::restoreNotepadItem
 * @return 当前选中项
 */
QModelIndex LeftView::restoreNotepadItem()
{
    QModelIndex index = this->currentIndex();
    QItemSelectionModel *model = this->selectionModel();

    if (index.isValid() && StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM) {
        if (!model->isSelected(index)) {
            this->setCurrentIndex(index);
        }
    } else {
        index = setDefaultNotepadItem();
    }

    return index;
}

/**
 * @brief LeftView::setDefaultNotepadItem
 * @return 当前选中项
 */
QModelIndex LeftView::setDefaultNotepadItem()
{
    QModelIndex index = m_pSortViewFilter->index(0, 0, getNotepadRootIndex());
    this->setCurrentIndex(index);
    return index;
}

/**
 * @brief LeftView::addFolder
 * @param folder
 */
void LeftView::addFolder(VNoteFolder *folder)
{
    if (nullptr != folder) {
        QStandardItem *pItem = StandardItemCommon::createStandardItem(
            folder, StandardItemCommon::NOTEPADITEM);

        QStandardItem *root = getNotepadRoot();
        root->appendRow(pItem);
        QModelIndex index = m_pSortViewFilter->mapFromSource(pItem->index());
        setCurrentIndex(index);
    }
    this->scrollToTop();
}

/**
 * @brief LeftView::appendFolder
 * @param folder
 */
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

/**
 * @brief LeftView::editFolder
 */
void LeftView::editFolder()
{
    edit(currentIndex());
}

/**
 * @brief LeftView::removeFolder
 * @return 删除项绑定的数据
 */
VNoteFolder *LeftView::removeFolder()
{
    QModelIndex index = currentIndex();

    if (StandardItemCommon::getStandardItemType(index) != StandardItemCommon::NOTEPADITEM) {
        return nullptr;
    }

    VNoteFolder *data = reinterpret_cast<VNoteFolder *>(
        StandardItemCommon::getStandardItemData(index));

    m_pSortViewFilter->removeRow(index.row(), index.parent());

    return data;
}

/**
 * @brief LeftView::getFolderSortId
 * @return 获取当前顺序所有记事本编号
 */
QString LeftView::getFolderSortId()
{
    QString tmpQstr = "";
    VNoteFolder *data {nullptr};
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex currentIndex;
    for (int i = 0; i < folderCount(); i++) {
        currentIndex = m_pSortViewFilter->index(i, 0, rootIndex);
        if(!currentIndex.isValid()) {
            break;
        }
        data = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(currentIndex));
        if(tmpQstr.isEmpty()) {
            tmpQstr = QString::number(data->id);
        }else {
            tmpQstr = tmpQstr + "," + QString::number(data->id);
        }
    }
    return tmpQstr;
}

/**
 * @brief LeftView::setFolderSortNum
 * @return 给当前记事本列表绑定排序编号
 */
bool LeftView::setFolderSortNum()
{
    bool sortResult = false;
    VNoteFolder *data {nullptr};
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex currentIndex;
    int rowCount = folderCount();
    for (int i = 0; i < rowCount; i++) {
        currentIndex = m_pSortViewFilter->index(i, 0, rootIndex);
        if(!currentIndex.isValid()) {
            break;
        }
        data = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(currentIndex));
        if(nullptr != data) {
            reinterpret_cast<VNoteFolder *>(
                                StandardItemCommon::getStandardItemData(currentIndex))->sort_number = rowCount - i;
        }
        sortResult = true;
    }
    return sortResult;
}

/**
 * @brief LeftView::folderCount
 * @return 记事本个数
 */
int LeftView::folderCount()
{
    int count = 0;

    QStandardItem *root = getNotepadRoot();

    if (nullptr != root) {
        count = root->rowCount();
    }

    return count;
}

/**
 * @brief LeftView::initMenu
 */
void LeftView::initMenu()
{
    m_notepadMenu = ActionManager::Instance()->notebookContextMenu();
}

/**
 * @brief LeftView::setOnlyCurItemMenuEnable
 * @param enable
 */
void LeftView::setOnlyCurItemMenuEnable(bool enable)
{
    m_onlyCurItemMenuEnable = enable;
    m_pItemDelegate->setEnableItem(!enable);
    this->update();
}

/**
 * @brief LeftView::sort
 */
void LeftView::sort()
{
    return m_pSortViewFilter->sort(0, Qt::DescendingOrder);
}

/**
 * @brief LeftView::closeEditor
 * @param editor
 * @param hint
 */
void LeftView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    Q_UNUSED(hint);
    DTreeView::closeEditor(editor, QAbstractItemDelegate::NoHint);
}

void LeftView::dropEvent(QDropEvent *event)
{
    if (StandardItemCommon::getStandardItemType(this->indexAt(event->pos())) != StandardItemCommon::NOTEPADITEM ||
            this->indexAt(event->pos()) == m_index) {
        DTreeView::dropEvent(event);
    } else {
        QModelIndex rootIndex = getNotepadRootIndex();
        QModelIndex currentIndex;
        QModelIndex currentSorceIndex;
        QModelIndex currentTargetIndex;
        int sorceRow = m_index.row();
        int targetRow = this->indexAt(event->pos()).row();
        int tmpRow = qAbs(targetRow - sorceRow);
        int sorceSortNum = 0;
        int targetSortNum = 0;
        currentSorceIndex = m_pSortViewFilter->index(sorceRow, 0, rootIndex);
        currentTargetIndex = m_pSortViewFilter->index(targetRow, 0, rootIndex);
        if(!currentSorceIndex.isValid() || !currentTargetIndex.isValid()) {
            DTreeView::dropEvent(event);
        }

        sorceSortNum = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(currentSorceIndex))->sort_number;
        if (-1 == sorceSortNum) {
            if (!setFolderSortNum()) {
                DTreeView::dropEvent(event);
            }
        }

        sorceSortNum = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(currentSorceIndex))->sort_number;
        targetSortNum = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(currentTargetIndex))->sort_number;

        for (int i = 0; i < tmpRow; i++) {
            if(targetRow > sorceRow) {
                currentIndex = m_pSortViewFilter->index(targetRow - i, 0, rootIndex);
                if(!currentIndex.isValid()) {
                    break;
                }
                reinterpret_cast<VNoteFolder *>(
                                    StandardItemCommon::getStandardItemData(currentIndex))->sort_number += 1;
            } else if (targetRow < sorceRow) {
                currentIndex = m_pSortViewFilter->index(targetRow + i, 0, rootIndex);
                if(!currentIndex.isValid()) {
                    break;
                }
                reinterpret_cast<VNoteFolder *>(
                                    StandardItemCommon::getStandardItemData(currentIndex))->sort_number -= 1;
            }
        }

        reinterpret_cast<VNoteFolder *>(
                            StandardItemCommon::getStandardItemData(currentSorceIndex))->sort_number = targetSortNum;
        sort();

        QString folderSortData = getFolderSortId();
        setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);
    }
    DTreeView::dropEvent(event);

}

void LeftView::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void LeftView::dragMoveEvent(QDragMoveEvent *event)
{
    DTreeView::dragMoveEvent(event);
    event->accept();
}

void LeftView::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
}

/**
 * @brief LeftView::closeMenu
 */
void LeftView::closeMenu()
{
    m_notepadMenu->close();
}
