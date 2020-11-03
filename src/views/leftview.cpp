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
#include "moveview.h"

#include "dialog/folderselectdialog.h"

#include "common/actionmanager.h"
#include "common/standarditemcommon.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "common/setting.h"

#include "db/vnoteitemoper.h"

#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QDebug>
#include <QTimer>
#include <QScrollBar>

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
    this->setDragDropMode(QAbstractItemView::DragDrop);
    this->setDropIndicatorShown(true);
    this->setAcceptDrops(true);
    this->setContextMenuPolicy(Qt::NoContextMenu);
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
    Q_UNUSED(event);

    if (event->buttons() & Qt::LeftButton) {
        triggerDragFolder();
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

/**
 * @brief LeftView::closeMenu
 */
void LeftView::closeMenu()
{
    m_notepadMenu->close();
}

bool LeftView::doNoteMove(const QModelIndexList &src, const QModelIndex &dst)
{
    if(src.size() && StandardItemCommon::getStandardItemType(dst) == StandardItemCommon::NOTEPADITEM){
        VNoteFolder *selectFolder = static_cast<VNoteFolder*>(StandardItemCommon::getStandardItemData(dst));
        VNoteItem *tmpData = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(src[0]));
        if(selectFolder && tmpData->folderId != selectFolder->id){
            VNoteItemOper noteOper;
            VNOTE_ITEMS_MAP *srcNotes = noteOper.getFolderNotes(tmpData->folderId);
            VNOTE_ITEMS_MAP *destNotes = noteOper.getFolderNotes(selectFolder->id);
            for(auto it : src){
                tmpData = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(it));
                //更新内存数据
                srcNotes->lock.lockForWrite();
                srcNotes->folderNotes.remove(tmpData->noteId);
                srcNotes->lock.unlock();

                destNotes->lock.lockForWrite();
                tmpData->folderId = selectFolder->id;
                destNotes->folderNotes.insert(tmpData->noteId, tmpData);
                destNotes->lock.unlock();
                //更新数据库
                noteOper.updateFolderId(tmpData);
            }
            return true;
        }
    }
    return false;
}

QModelIndex LeftView::selectMoveFolder(const QModelIndexList &src)
{
    QModelIndex index;
    if(src.size()){
        VNoteItem *data = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(src[0]));
        QString itemInfo = QString("移动 %1 等%2个笔记到：").arg(data->noteTitle).arg(src.size());
        if(m_folderSelectDialog == nullptr){
            m_folderSelectDialog = new FolderSelectDialog(m_pDataModel, this);
            m_folderSelectDialog->resize(448, 372);
        }
        QList<VNoteFolder *> folders;
        folders.push_back(static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(currentIndex())));
        m_folderSelectDialog->setFolderBlack(folders);
        m_folderSelectDialog->setNoteContext(itemInfo);
        m_folderSelectDialog->clearSelection();
        m_folderSelectDialog->exec();
        if(m_folderSelectDialog->result() == FolderSelectDialog::Accepted){
            index = m_folderSelectDialog->getSelectIndex();
        }
    }
    return index;
}

 QString LeftView::getFolderSort()
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

 bool LeftView::setFolderSort()
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
                         StandardItemCommon::getStandardItemData(currentIndex))->sortNumber = rowCount - i;
         }
         sortResult = true;
     }
     return sortResult;
 }

 bool LeftView::needFolderSort()
 {
     if (getNotepadRoot()->hasChildren()){
         QModelIndex child = getNotepadRoot()->child(0)->index();
         if (child.isValid()) {
             VNoteFolder *vnotefolder = reinterpret_cast<VNoteFolder *>(
                         StandardItemCommon::getStandardItemData(child));
             return  vnotefolder && (-1 != vnotefolder->sortNumber);
         }
     }
     return false;
 }

 void LeftView::dragEnterEvent(QDragEnterEvent *event)
 {
     if(m_folderDraing){
         m_pItemDelegate->setDragState(true);
         this->update();
     }
     event->accept();
 }

 void LeftView::dragMoveEvent(QDragMoveEvent *event)
 {
     DTreeView::dragMoveEvent(event);
     this->update();
     event->accept();
 }

 void LeftView::dragLeaveEvent(QDragLeaveEvent *event)
 {
     Q_UNUSED(event)
     if(m_folderDraing){
         m_pItemDelegate->setDragState(false);
         m_pItemDelegate->setDrawHover(false);
         this->update();
     }
     event->accept();
 }

 void LeftView::doDragMove(const QModelIndex &src, const QModelIndex &dst)
 {
     if(src.isValid() && dst.isValid() && src != dst){
         VNoteFolder *tmpFolder = reinterpret_cast<VNoteFolder *>(
                     StandardItemCommon::getStandardItemData(dst));
         if(tmpFolder == nullptr){
             return;
         }
         if(!needFolderSort()){
             setFolderSort();
         }
         int tmpRow = qAbs(src.row() - dst.row());
         int dstNum = tmpFolder->sortNumber;
         QModelIndex tmpIndex;
         QModelIndex rootIndex = getNotepadRootIndex();
         for (int i = 0; i < tmpRow; i++) {
             if(dst.row() > src.row()) {
                 tmpIndex = m_pSortViewFilter->index(dst.row() - i, 0, rootIndex);
                 if(!tmpIndex.isValid()) {
                     break;
                 }
                 tmpFolder = reinterpret_cast<VNoteFolder *>(
                             StandardItemCommon::getStandardItemData(tmpIndex));
                 tmpFolder->sortNumber += 1;

             }else{
                 tmpIndex = m_pSortViewFilter->index(dst.row() + i, 0, rootIndex);
                 if(!tmpIndex.isValid()) {
                     break;
                 }
                 tmpFolder = reinterpret_cast<VNoteFolder *>(
                             StandardItemCommon::getStandardItemData(tmpIndex));
                 tmpFolder->sortNumber -= 1;
             }
         }
         tmpFolder = reinterpret_cast<VNoteFolder *>(
                     StandardItemCommon::getStandardItemData(src));
         tmpFolder->sortNumber = dstNum;

         sort();

         QString folderSortData = getFolderSort();
         setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);

     }
 }

 void LeftView::triggerDragFolder()
 {
     VNoteFolder *folder = reinterpret_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(currentIndex()));
     if(folder){
         QDrag *drag = new QDrag(this);
         QMimeData *mimeData = new QMimeData;

         if(m_MoveView == nullptr){
             m_MoveView = new MoveView(this);
         }
         m_MoveView->setFolder(folder);
         drag->setPixmap(m_MoveView->grab());
         drag->setMimeData(mimeData);
         m_folderDraing = true;
         drag->exec(Qt::MoveAction);
         drag->deleteLater();
         doDragMove(currentIndex(), indexAt(mapFromGlobal(QCursor::pos())));
         m_folderDraing = false;
         m_pItemDelegate->setDragState(false);
         m_pItemDelegate->setDrawHover(true);
     }
 }
