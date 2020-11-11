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
#include <QDebug>
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
    setContextMenuPolicy(Qt::NoContextMenu);
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
    //触控屏手势
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        //记录触控起始位置与时间点
        m_touchPressPoint = event->pos();
        m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }

    if (!m_onlyCurItemMenuEnable) {
        event->setModifiers(Qt::NoModifier);
        DTreeView::mousePressEvent(event);
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
    m_isTouchSliding = false;
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
        if (!m_onlyCurItemMenuEnable) {
            //鼠标左键滑动
            if (event->source() == Qt::MouseEventSynthesizedByQt) {
                //计算上下、左右移动距离与时间间隔
                double distY = event->pos().y() - m_touchPressPoint.y();
                double distX = event->pos().x() - m_touchPressPoint.x();
                qint64 currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
                qint64 timeInterval = currentTime - m_touchPressStartMs;
                //正在滑动时移动距离超过5px，持续执行滚动操作
                if (m_isTouchSliding) {
                    if (qAbs(distY) > 5)
                        handleTouchSlideEvent(timeInterval, distY, event->pos());
                    return;
                }
                //初次判断，如果在250ms之内滑动距离超过5px，执行滚动操作
                else {
                    if (timeInterval < 250) {
                        if (qAbs(distY) > 10) {
                            m_isTouchSliding = true;
                            handleTouchSlideEvent(timeInterval, distY, event->pos());
                        }
                        //仅有左右滑动距离超过5px，更新为滚动状态但不执行滚动操作，用于区分拖拽事件
                        else if (qAbs(distX) > 5) {
                            m_isTouchSliding = true;
                        }
                        return;
                    }
                }
            }
            DTreeView::mouseMoveEvent(event);
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

/**
 * @brief LeftView::doNoteMove
 * @param src
 * @param dst
 * @return
 */
bool LeftView::doNoteMove(const QModelIndexList &src, const QModelIndex &dst)
{
    if (src.size() && StandardItemCommon::getStandardItemType(dst) == StandardItemCommon::NOTEPADITEM) {
        VNoteFolder *selectFolder = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(dst));
        VNoteItem *tmpData = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(src[0]));
        if (selectFolder && tmpData->folderId != selectFolder->id) {
            VNoteItemOper noteOper;
            VNOTE_ITEMS_MAP *srcNotes = noteOper.getFolderNotes(tmpData->folderId);
            VNOTE_ITEMS_MAP *destNotes = noteOper.getFolderNotes(selectFolder->id);
            for (auto it : src) {
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

/**
 * @brief LeftView::selectMoveFolder
 * @param src
 * @return
 */
QModelIndex LeftView::selectMoveFolder(const QModelIndexList &src)
{
    QModelIndex index;
    if (src.size()) {
        VNoteItem *data = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(src[0]));
        QString elideText = data->noteTitle;
        int textSize = fontMetrics().width(elideText);
        if (textSize > 147) {
            elideText = fontMetrics().elidedText(data->noteTitle, Qt::ElideRight, 147);
        }
        QString itemInfo = "";
        if(src.size() == 1){
           itemInfo = QString("Move the note \"%1\" to:").arg(elideText);
        }else {
           itemInfo = QString("Move %1 notes (%2, ...) to:").arg(elideText).arg(src.size());
        }

        if (m_folderSelectDialog == nullptr) {
            m_folderSelectDialog = new FolderSelectDialog(m_pDataModel, this);
            m_folderSelectDialog->resize(448, 372);
        }
        QList<VNoteFolder *> folders;
        folders.push_back(static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(currentIndex())));
        m_folderSelectDialog->setFolderBlack(folders);
        m_folderSelectDialog->setNoteContext(itemInfo);
        m_folderSelectDialog->clearSelection();
        m_folderSelectDialog->exec();
        if (m_folderSelectDialog->result() == FolderSelectDialog::Accepted) {
            index = m_folderSelectDialog->getSelectIndex();
        }
    }
    return index;
}

/**
 * @brief LeftView::getFolderSort
 * @return 当前所有记事本的排序编号字符串
 * 获取记事本顺序
 */
QString LeftView::getFolderSort()
{
    QString tmpQstr = "";
    VNoteFolder *data {nullptr};
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex currentIndex;
    for (int i = 0; i < folderCount(); i++) {
        currentIndex = m_pSortViewFilter->index(i, 0, rootIndex);
        if (!currentIndex.isValid()) {
            break;
        }
        data = reinterpret_cast<VNoteFolder *>(
                   StandardItemCommon::getStandardItemData(currentIndex));
        if (tmpQstr.isEmpty()) {
            tmpQstr = QString::number(data->id);
        } else {
            tmpQstr = tmpQstr + "," + QString::number(data->id);
        }
    }
    return tmpQstr;
}

/**
 * @brief LeftView::setFolderSort
 * @return true 排序成功，false 排序失败
 * 设置记事本默认顺序
 */
bool LeftView::setFolderSort()
{
    bool sortResult = false;
    VNoteFolder *data {nullptr};
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex currentIndex;
    int rowCount = folderCount();
    for (int i = 0; i < rowCount; i++) {
        currentIndex = m_pSortViewFilter->index(i, 0, rootIndex);
        if (!currentIndex.isValid()) {
            break;
        }
        data = reinterpret_cast<VNoteFolder *>(
                   StandardItemCommon::getStandardItemData(currentIndex));
        if (nullptr != data) {
            reinterpret_cast<VNoteFolder *>(
                StandardItemCommon::getStandardItemData(currentIndex))->sortNumber = rowCount - i;
        }
        sortResult = true;
    }
    return sortResult;
}

/**
 * @brief LeftView::getFirstFolder
 * @return 第一个记事本
 */
VNoteFolder* LeftView::getFirstFolder()
{
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex child = m_pSortViewFilter->index(0, 0, rootIndex);
    if (child.isValid()) {
        VNoteFolder *vnotefolder = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(child));
        return  vnotefolder;
    }
    return nullptr;
}

/**
 * @brief LeftView::handleTouchSlideEvent  处理触摸屏move事件，区分点击、滑动、拖拽、长按功能
 * @param timeParam 时间间隔
 * @param distY 纵向移动距离
 * @param point 当前时间发生位置
 */
void LeftView::handleTouchSlideEvent(qint64 timeParam, double distY, QPoint point)
{
    //根据移动距离与时间计算滑动速度，用于设置滚动步长
    double param = ((qAbs(distY)) / timeParam) + 0.3;
    verticalScrollBar()->setSingleStep(static_cast<int>(20 * param));
    verticalScrollBar()->triggerAction((distY > 0) ? QScrollBar::SliderSingleStepSub : QScrollBar::SliderSingleStepAdd);
    m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_touchPressPoint = point;
}

/**
 * @brief LeftView::startDrag
 * @param supportedActions
 * 开始拖拽事件
 */
void LeftView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);
    triggerDragFolder();
}

/**
 * @brief LeftView::dragEnterEvent
 * @param event
 * 拖拽进入视图事件
 */
void LeftView::dragEnterEvent(QDragEnterEvent *event)
{
    // 判断拖拽进入视图事件触发类型（笔记：NOTES_DRAG_KEY；记事本：NOTEPAD_DRAG_KEY）
    if (!event->mimeData()->hasFormat(NOTES_DRAG_KEY)
            && !event->mimeData()->hasFormat(NOTEPAD_DRAG_KEY)) {
        event->ignore();
        return DTreeView::dragEnterEvent(event);
    }

    if (m_folderDraing) {
        m_pItemDelegate->setDragState(true);
        this->update();
    }

    event->accept();
}

/**
 * @brief LeftView::dragMoveEvent
 * @param event
 * 拖拽移动事件
 */
void LeftView::dragMoveEvent(QDragMoveEvent *event)
{
    DTreeView::dragMoveEvent(event);
    this->update();
    event->accept();
}

/**
 * @brief LeftView::dragLeaveEvent
 * @param event
 * 拖拽离开视图事件
 */
void LeftView::dragLeaveEvent(QDragLeaveEvent *event)
{
    if (m_folderDraing) {
        m_pItemDelegate->setDragState(false);
        m_pItemDelegate->setDrawHover(false);
        this->update();
    }
    event->accept();
}

/**
 * @brief LeftView::doDragMove
 * @param src
 * @param dst
 * 拖拽移动
 */
void LeftView::doDragMove(const QModelIndex &src, const QModelIndex &dst)
{
    if (src.isValid() && dst.isValid() && src != dst) {
        VNoteFolder *tmpFolder = reinterpret_cast<VNoteFolder *>(
                                     StandardItemCommon::getStandardItemData(dst));
        if (nullptr == tmpFolder) {
            return;
        }

        // 判断当前是否需要进行重新排序
        VNoteFolder *firstFolder = getFirstFolder();
        if (firstFolder && -1 == firstFolder->sortNumber) {
            setFolderSort();
        }

        int tmpRow = qAbs(src.row() - dst.row());
        int dstNum = tmpFolder->sortNumber;
        QModelIndex tmpIndex;
        QModelIndex rootIndex = getNotepadRootIndex();

        // 根据拖拽放下的位置，给相应的记事本重新赋值排序编号
        for (int i = 0; i < tmpRow; i++) {
            // 目标位置所在的行数比被拖拽的记事本的行数大，则将目标位置记事本和被拖拽记事本之间的记事本的排序编号依次加1，反之依次减1
            if (dst.row() > src.row()) {
                tmpIndex = m_pSortViewFilter->index(dst.row() - i, 0, rootIndex);
                if (!tmpIndex.isValid()) {
                    break;
                }
                tmpFolder = reinterpret_cast<VNoteFolder *>(
                                StandardItemCommon::getStandardItemData(tmpIndex));
                tmpFolder->sortNumber += 1;

            } else {
                tmpIndex = m_pSortViewFilter->index(dst.row() + i, 0, rootIndex);
                if (!tmpIndex.isValid()) {
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

        // 获取重新排序后每个记事本的排序编号，写入配置文件中
        QString folderSortData = getFolderSort();
        setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);

    }
}

/**
 * @brief LeftView::triggerDragFolder
 * 触发拖动操作
 */
void LeftView::triggerDragFolder()
{
    VNoteFolder *folder = reinterpret_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(currentIndex()));
    // 判断当前拖拽的记事本是否可用，如果可用，则初始化拖拽操作的数据
    if (folder) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(NOTEPAD_DRAG_KEY, QByteArray());

        if (nullptr == m_MoveView) {
            m_MoveView = new MoveView(this);
        }
        m_MoveView->setFolder(folder);
        drag->setPixmap(m_MoveView->grab());
        drag->setMimeData(mimeData);
        m_folderDraing = true;
        drag->exec(Qt::MoveAction);
        drag->deleteLater();
        m_folderDraing = false;
        m_pItemDelegate->setDragState(false);
        m_pItemDelegate->setDrawHover(true);
    }
}

/**
 * @brief LeftView::dropEvent
 * @param event
 * 拖拽放下事件
 */
void LeftView::dropEvent(QDropEvent * event)
{
    // 判断拖拽放下事件触发类型（笔记：NOTES_DRAG_KEY；记事本：NOTEPAD_DRAG_KEY）
    if (event->mimeData()->hasFormat(NOTES_DRAG_KEY)) {
        emit dropNotesEnd();
    } else if (event->mimeData()->hasFormat(NOTEPAD_DRAG_KEY)) {
        doDragMove(currentIndex(), indexAt(mapFromGlobal(QCursor::pos())));
    }
}
