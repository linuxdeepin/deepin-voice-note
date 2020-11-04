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
#include "common/vnoteitem.h"
#include "common/thumbnail.h"
#include "db/vnoteitemoper.h"
#include "dialog/leftviewdialog.h"

#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDrag>
#include <QMimeData>
#include <QScrollBar>
#include <QApplication>
#include <QMouseEvent>
#include <DLog>

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
    //禁止系统右键菜单弹出
    this->setContextMenuPolicy(Qt::NoContextMenu);
    connect(m_notepadMenu, &VNoteRightMenu::menuTouchMoved, this, &LeftView::handleDragEvent);
    connect(m_notepadMenu, &VNoteRightMenu::menuTouchReleased, this, [ = ] {
        updateDragingState();
        m_touchState = TouchState::TouchNormal;
    });
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
 * @brief LeftView::selectCurrentOnTouch 设置当前点击位置选中
 * @return null
 * @author
 */
void LeftView::selectCurrentOnTouch()
{
    QTimer::singleShot(250, this, [ = ] {
        if (m_touchState == TouchState::TouchPressing && m_index.isValid())
            this->setCurrentIndex(m_index);
    });
}

/**
 * @brief LeftView::updateDragingState //延时更新代理中拖拽状态
 * @return null
 * @author
 */
void LeftView::updateDragingState()
{
    QTimer::singleShot(300, [ = ] {
        m_pItemDelegate->setDraging(false);
    });
}

/**
 * @brief LeftView::mousePressEvent
 * @param event
 */
void LeftView::mousePressEvent(QMouseEvent *event)
{
    this->setFocus();
    //处理触摸屏单指press事件
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        if (viewport()->visibleRegion().contains(event->pos())) {
            QDateTime current = QDateTime::currentDateTime();
            m_touchPressStartMs = current.toMSecsSinceEpoch();
            m_touchPressPoint = event->pos();
            m_index = this->indexAt(event->pos());
            setTouchState(TouchState::TouchPressing);
            m_notepadMenu->setPressPointY(QCursor::pos());
            selectCurrentOnTouch();
            return;
        }
    }
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
            m_notepadMenu->setWindowOpacity(1);
        }
    }
}

/**
 * @brief LeftView::mouseReleaseEvent
 * @param event
 */
void LeftView::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDraging = false;
    //处理拖拽事件，由于与drop操作参数不同，暂未封装
    if (m_touchState == TouchState::TouchDraging) {
        updateDragingState();
        setTouchState(TouchState::TouchNormal);
        return;
    }
    //正常点击状态，选择当前点击选项
    QModelIndex index = indexAt(event->pos());
    if (index.row() != currentIndex().row() && m_touchState == TouchState::TouchPressing) {
        if (index.isValid())
            setCurrentIndex(index);
        setTouchState(TouchState::TouchNormal);
        return;
    }
    setTouchState(TouchState::TouchNormal);

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
    if ((event->source() == Qt::MouseEventSynthesizedByQt && event->buttons() & Qt::LeftButton)) {
        doTouchMoveEvent(event);
        return;
    } else if ((event->buttons() & Qt::LeftButton) && m_touchState == TouchState::TouchNormal) {
        if (!m_isDraging) {
            handleDragEvent();
        }
    } else {
        DTreeView::mouseMoveEvent(event);
    }
}

/**
 * @brief LeftView::doTouchMoveEvent  处理触摸屏move事件，区分点击、滑动、拖拽、长按功能
 * @param event
 */
void LeftView::doTouchMoveEvent(QMouseEvent *event)
{
    //处理触摸屏单指move事件，区分滑动、拖拽事件
    m_pItemDelegate->setDraging(false);
    double distX = event->pos().x() - m_touchPressPoint.x();
    double distY = event->pos().y() - m_touchPressPoint.y();
    //获取时间间隔
    QDateTime current = QDateTime::currentDateTime();
    qint64 timeParam = current.toMSecsSinceEpoch() - m_touchPressStartMs;

    switch (m_touchState) {
    //首次判断
    case TouchState::TouchPressing:
        //250ms-1000ms之间移动超过10px，判断为拖拽
        if ((timeParam > 250 && timeParam < 1000) && (qAbs(distY) > 10 || qAbs(distX) > 10)) {
            if (!m_isDraging)
                handleDragEvent();
        }
        //250ms之内，上下移动距离超过10px或左右移动距离超过5px，判断为滑动
        else if (timeParam <= 250 && (qAbs(distY) > 10 || qAbs(distX) > 5)) {
            setTouchState(TouchState::TouchMoving);
            handleTouchSlideEvent(timeParam, distY, event->pos());
        }
        break;
    //如果正在拖拽
    case TouchState::TouchDraging:
        if (!m_isDraging)
            handleDragEvent();
        break;
    //如果正在滑动
    case TouchState::TouchMoving:
        if (!viewport()->visibleRegion().contains(event->pos())) {
            setTouchState(TouchState::TouchNormal);
        } else if (qAbs(distY) > 5) {
            handleTouchSlideEvent(timeParam, distY, event->pos());
        }
        break;
    default:
        break;
    }
}

/**
 * @brief LeftView::handleTouchSlideEvent  处理触摸屏move事件，区分点击、滑动、拖拽、长按功能
 * @param timeParam 时间间隔
 * @param distY 纵向移动距离
 * @param point 当前时间发生位置
 */
void LeftView::handleTouchSlideEvent(qint64 timeParam, double distY, QPoint point)
{
    if (m_touchInterval == 0)
        m_touchInterval = m_touchPressStartMs;
    qint64 timerDis = timeParam - m_touchInterval;
    double param = ((qAbs(distY)) / timerDis) + 0.3;
    verticalScrollBar()->setSingleStep(static_cast<int>(20 * param));
    verticalScrollBar()->triggerAction((distY > 0) ? QScrollBar::SliderSingleStepSub : QScrollBar::SliderSingleStepAdd);
    m_touchInterval = timeParam;
    m_touchPressPoint = point;
}

/**
 * @brief LeftView::handleDragEvent 处理拖拽事件
 * @param
 */
void LeftView::handleDragEvent()
{
    m_notepadMenu->setWindowOpacity(0.0);
    setTouchState(TouchState::TouchDraging);
    m_pItemDelegate->setDraging(true);


    QModelIndex mouseMoveCurrentIndex = this->currentIndex();
    QPoint dragPoint = this->mapFromGlobal(QCursor::pos());
    QModelIndex dragIndex = this->indexAt(dragPoint);

    if (!dragIndex.isValid()) {
        m_isDraging = true;
        return;
    }
    m_pItemDelegate->setDragState(true);
    this->update();
    QString vnoteName = "";
    qint64 vnotesCount = 0;
    QIcon icon;
    VNoteFolder *data = reinterpret_cast<VNoteFolder *>(
                            StandardItemCommon::getStandardItemData(mouseMoveCurrentIndex));
    if (nullptr != data) {
        vnoteName = data->name;
        vnotesCount = data->getNotesCount();
        icon = QIcon(data->UI.icon);
    }

    thumbnail *dragImage = new thumbnail(this);
    dragImage->setupthumbnail(icon, vnoteName, QString::number(vnotesCount));
    QPixmap pixmap = dragImage->grab();
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    drag->exec(Qt::MoveAction);
    drag->deleteLater();
    QPoint point = this->mapFromGlobal(QCursor::pos());
    QModelIndex selectIndex = this->indexAt(point);
    updateFolderSortNum(mouseMoveCurrentIndex, selectIndex);
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
        if (!currentIndex.isValid()) {
            break;
        }
        data = reinterpret_cast<VNoteFolder *>(
                   StandardItemCommon::getStandardItemData(currentIndex));
        if (nullptr != data) {
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
 * @brief LeftView::setTouchState 更新触摸屏一指状态
 * @param touchState
 */
void LeftView::setTouchState(const TouchState &touchState)
{
    m_touchState = touchState;
}

/**
 * @brief LeftView::setMoveList 更新移动笔记列表
 */
void LeftView::setMoveList(const QModelIndexList &moveList)
{
    m_moveList = moveList;
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

void LeftView::dragEnterEvent(QDragEnterEvent *event)
{
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
}

/**
 * @brief LeftView::closeMenu
 */
void LeftView::closeMenu()
{
    m_notepadMenu->close();
}

bool LeftView::popupMoveDlg()
{
    bool ret = false;
    if (m_moveList.size()) {
        if (m_folderSelectDlg == nullptr) {
            m_folderSelectDlg = new LeftviewDialog(m_pSortViewFilter, this);
            m_folderSelectDlg->resize(448, 372);
            connect(m_folderSelectDlg, &LeftviewDialog::accepted, this, [&ret, this]() {
                ret = doNoteMove(m_moveList, m_folderSelectDlg->getSelectIndex());
                m_moveList.clear();
            });
        }
        m_folderSelectDlg->setEnabled(true);
        VNoteItem *data = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(m_moveList[0]));
        QString itemInfo = QString("移动 %1 等%2个笔记到：").arg(data->noteTitle).arg(m_moveList.size());
        m_folderSelectDlg->setNoteContext(itemInfo);
        m_folderSelectDlg->clearSelection();
        m_folderSelectDlg->exec();
    }
    return ret;
}

bool LeftView::doNoteMove(const QModelIndexList &src, const QModelIndex &dst)
{
    VNoteFolder *selectFolder = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(dst));
    VNoteItem *srcData = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(src[0]));

    if (selectFolder && srcData->folderId != selectFolder->id) {
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *srcNotes = noteOper.getFolderNotes(srcData->folderId);
        VNOTE_ITEMS_MAP *destNotes = noteOper.getFolderNotes(selectFolder->id);
        for (auto it : src) {
            srcData = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(it));
            //更新内存数据
            srcNotes->lock.lockForWrite();
            srcNotes->folderNotes.remove(srcData->noteId);
            srcNotes->lock.unlock();

            destNotes->lock.lockForWrite();
            srcData->folderId = selectFolder->id;
            destNotes->folderNotes.insert(srcData->noteId, srcData);
            destNotes->lock.unlock();
            //更新数据库
            noteOper.updateFolderId(srcData);
        }
        return true;
    }
    return false;
}

void LeftView::updateFolderSortNum(const QModelIndex &sorceIndex, const QModelIndex &targetIndex)
{
    this->update();
    m_pItemDelegate->setDragState(false);
    if (StandardItemCommon::getStandardItemType(targetIndex) != StandardItemCommon::NOTEPADITEM ||
            targetIndex == sorceIndex) {
        m_notepadMenu->hide();
        return;
    } else {
        QModelIndex rootIndex = getNotepadRootIndex();
        QModelIndex currentIndex;
        int sorceRow = sorceIndex.row();
        int targetRow = targetIndex.row();
        int tmpRow = qAbs(targetRow - sorceRow);
        int sorceSortNum = 0;
        int targetSortNum = 0;
        if (!sorceIndex.isValid() || !targetIndex.isValid()) {
            return;
        }

        sorceSortNum = reinterpret_cast<VNoteFolder *>(
                           StandardItemCommon::getStandardItemData(sorceIndex))->sort_number;
        if (-1 == sorceSortNum) {
            if (!setFolderSortNum()) {
                return;
            }
        }

        sorceSortNum = reinterpret_cast<VNoteFolder *>(
                           StandardItemCommon::getStandardItemData(sorceIndex))->sort_number;
        targetSortNum = reinterpret_cast<VNoteFolder *>(
                            StandardItemCommon::getStandardItemData(targetIndex))->sort_number;

        for (int i = 0; i < tmpRow; i++) {
            if (targetRow > sorceRow) {
                currentIndex = m_pSortViewFilter->index(targetRow - i, 0, rootIndex);
                if (!currentIndex.isValid()) {
                    break;
                }
                reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(currentIndex))->sort_number += 1;
            } else if (targetRow < sorceRow) {
                currentIndex = m_pSortViewFilter->index(targetRow + i, 0, rootIndex);
                if (!currentIndex.isValid()) {
                    break;
                }
                reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(currentIndex))->sort_number -= 1;
            }
        }

        reinterpret_cast<VNoteFolder *>(
            StandardItemCommon::getStandardItemData(sorceIndex))->sort_number = targetSortNum;
        sort();

        QString folderSortData = getFolderSortId();
        setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);
    }
    m_notepadMenu->hide();
}
