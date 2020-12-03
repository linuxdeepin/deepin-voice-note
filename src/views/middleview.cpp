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

#include "middleview.h"
#include "vnoteapplication.h"
#include "globaldef.h"
#include "middleviewdelegate.h"
#include "middleviewsortfilter.h"
#include "common/actionmanager.h"
#include "common/standarditemcommon.h"
#include "common/vnoteitem.h"
#include "task/exportnoteworker.h"
#include "common/setting.h"
#include "db/vnoteitemoper.h"
#include "moveview.h"

#include <QMouseEvent>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QDrag>
#include <QMimeData>

#include <DApplication>
#include <DFileDialog>
#include <DLog>

/**
 * @brief MiddleView::MiddleView
 * @param parent
 */
MiddleView::MiddleView(QWidget *parent)
    : DListView(parent)
{
    initModel();
    initDelegate();
    initMenu();
    initUI();
    initConnections();

    setContextMenuPolicy(Qt::NoContextMenu);
    this->setDragEnabled(true);
    this->setDragDropMode(QAbstractItemView::DragOnly);
    this->setAcceptDrops(false);
    //dx-选择
    this->setSelectionMode(QAbstractItemView::MultiSelection);
}

/**
 * @brief MiddleView::initModel
 */
void MiddleView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);

    m_pSortViewFilter = new MiddleViewSortFilter(this);
    m_pSortViewFilter->setDynamicSortFilter(false);
    m_pSortViewFilter->setSourceModel(m_pDataModel);

    this->setModel(m_pSortViewFilter);
}

/**
 * @brief MiddleView::initDelegate
 */
void MiddleView::initDelegate()
{
    m_pItemDelegate = new MiddleViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

/**
 * @brief LeftView::setTouchState 更新触摸屏一指状态
 * @param touchState
 */
void MiddleView::setTouchState(const TouchState &touchState)
{
    m_touchState = touchState;
}

/**
 * @brief MiddleView::initMenu
 */
void MiddleView::initMenu()
{
    m_noteMenu = ActionManager::Instance()->noteContextMenu();
}

/**
 * @brief MiddleView::setSearchKey
 * @param key 搜索关键字
 */
void MiddleView::setSearchKey(const QString &key)
{
    m_searchKey = key;
    m_pItemDelegate->setSearchKey(key);
}

/**
 * @brief MiddleView::setCurrentId
 * @param id
 */
void MiddleView::setCurrentId(qint64 id)
{
    m_currentId = id;
}

/**
 * @brief MiddleView::getCurrentId
 * @return 绑定的id
 */
qint64 MiddleView::getCurrentId()
{
    return m_currentId;
}

/**
 * @brief MiddleView::addRowAtHead
 * @param note
 */
void MiddleView::addRowAtHead(VNoteItem *note)
{
    if (nullptr != note) {
        QStandardItem *item = StandardItemCommon::createStandardItem(note, StandardItemCommon::NOTEITEM);
        m_pDataModel->insertRow(0, item);
        sortView(false);
        QModelIndex index = m_pDataModel->index(item->row(), 0);
        DListView::setCurrentIndex(m_pSortViewFilter->mapFromSource(index));
        this->scrollTo(currentIndex());
        //dx-添加后选中
        m_currentRow = currentIndex().row();
    }
}

/**
 * @brief MiddleView::appendRow
 * @param note
 */
void MiddleView::appendRow(VNoteItem *note)
{
    if (nullptr != note) {
        QStandardItem *item = StandardItemCommon::createStandardItem(note, StandardItemCommon::NOTEITEM);
        m_pDataModel->appendRow(item);
    }
}

/**
 * @brief MiddleView::clearAll
 */
void MiddleView::clearAll()
{
    m_pDataModel->clear();
}

/**
 * @brief MiddleView::deleteCurrentRow
 * @return 移除的记事项绑定的数据
 *///dx-右键删除
QList<VNoteItem *>MiddleView::deleteCurrentRow()
{
    QModelIndexList indexList = selectedIndexes();
    QList<VNoteItem *> noteItemList;
    qSort(indexList);
    for(int i = indexList.size()-1;i>-1;i--){
        VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                                  StandardItemCommon::getStandardItemData(indexList[i]));
        noteItemList.append(noteData);
        m_pSortViewFilter->removeRow(indexList[i].row());
    }
    return noteItemList;
}

/**
 * @brief MiddleView::getCurrVNotedata
 * @return 当前选中的记事项数据
 */
VNoteItem *MiddleView::getCurrVNotedata() const
{
    //dx-选择
    VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                              StandardItemCommon::getStandardItemData(m_index));

    return noteData;
}

//dx-拖拽移动
QList<VNoteItem *>MiddleView::getCurrVNotedataList() const
{
    QModelIndexList modelList = selectedIndexes();
    QList<VNoteItem *> noteDataList;
    for(auto index:modelList){
        VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                                  StandardItemCommon::getStandardItemData(index));
        noteDataList.append(noteData);
    }

    return noteDataList;
}

/**
 * @brief MiddleView::onNoteChanged
 */
void MiddleView::onNoteChanged()
{
    sortView();
    //dx-选择
    m_currentRow = -1;
}

/**
 * @brief MiddleView::rowCount
 * @return 记事项数目
 */
qint32 MiddleView::rowCount() const
{
    return m_pDataModel->rowCount();
}

/**
 * @brief MiddleView::setCurrentIndex
 * @param index
 */
void MiddleView::setCurrentIndex(int index)
{
    DListView::setCurrentIndex(m_pSortViewFilter->index(index, 0));
}

/**
 * @brief MiddleView::editNote
 */
void MiddleView::editNote()
{
    edit(currentIndex());
}

/**
 * @brief MiddleView::saveAsText
 *///dx-导出文本
void MiddleView::saveAsText()
{
    QModelIndexList indexList = selectedIndexes();
    qSort(indexList);
    QList<VNoteItem *> noteDataList;
    for(auto index : indexList){
        VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                                  StandardItemCommon::getStandardItemData(index));
        noteDataList.append(noteData);
    }
    if (indexList.size()) {
        //TODO:
        //    Should check if this note is doing save action

        DFileDialog dialog;
        dialog.setFileMode(DFileDialog::DirectoryOnly);
        dialog.setLabelText(DFileDialog::Accept, DApplication::translate("MiddleView", "Save"));
        dialog.setNameFilter("TXT(*.txt)");

        QString historyDir = setting::instance()->getOption(VNOTE_EXPORT_TEXT_PATH_KEY).toString();
        if (historyDir.isEmpty()) {
            historyDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        }
        dialog.setDirectory(historyDir);

        if (QDialog::Accepted == dialog.exec()) {
            // save the directory string to config file.
            setting::instance()->setOption(VNOTE_EXPORT_TEXT_PATH_KEY, dialog.directoryUrl().toLocalFile());

            QString exportDir = dialog.directoryUrl().toLocalFile();
            ExportNoteWorker *exportWorker = new ExportNoteWorker(
                exportDir, ExportNoteWorker::ExportText, noteDataList);
            exportWorker->setAutoDelete(true);

            QThreadPool::globalInstance()->start(exportWorker);
        }
    }
}

/**
 * @brief MiddleView::saveRecords
 *///dx-保存语音
void MiddleView::saveRecords()
{
    QModelIndexList indexList = selectedIndexes();
    qSort(indexList);
    QList<VNoteItem *>noteItemList;
    for(auto index : indexList){
        VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                                  StandardItemCommon::getStandardItemData(index));
        noteItemList.append(noteData);
    }
    if (noteItemList.size()) {
        //TODO:
        //    Should check if this note is doing save action

        DFileDialog dialog;
        dialog.setFileMode(DFileDialog::DirectoryOnly);
        dialog.setLabelText(DFileDialog::Accept, DApplication::translate("MiddleView", "Save"));
        dialog.setNameFilter("MP3(*.mp3)");

        QString historyDir = setting::instance()->getOption(VNOTE_EXPORT_VOICE_PATH_KEY).toString();
        if (historyDir.isEmpty()) {
            historyDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        }
        dialog.setDirectory(historyDir);

        if (QDialog::Accepted == dialog.exec()) {
            // save the directory string to config file.
            setting::instance()->setOption(VNOTE_EXPORT_VOICE_PATH_KEY, dialog.directoryUrl().toLocalFile());

            QString exportDir = dialog.directoryUrl().toLocalFile();

            ExportNoteWorker *exportWorker = new ExportNoteWorker(
                exportDir, ExportNoteWorker::ExportAllVoice, noteItemList);
            exportWorker->setAutoDelete(true);

            QThreadPool::globalInstance()->start(exportWorker);
        }
    }
}

/**
 * @brief MiddleView::mousePressEvent
 * @param event
 */
void MiddleView::mousePressEvent(QMouseEvent *event)
{
    this->setFocus();

    if (!m_onlyCurItemMenuEnable) {
        //触控屏手势
        if (event->source() == Qt::MouseEventSynthesizedByQt) {
            //dx-触屏多选
            if(Qt::NoModifier == event->modifiers()){
                //记录此时触控点的位置，用于move事件中滑动距离与速度
                m_touchPressPoint = event->pos();
                m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
                //更新触摸状态
                setTouchState(TouchPressing);
                m_index = indexAt(event->pos());
                m_noteMenu->setPressPoint(QCursor::pos());
                //dx-选择
                setMouseState(MouseState::pressing);

                //检查是否选中
                m_selectCurrentTimer->start(250);
                //是否弹出右键菜单
                m_popMenuTimer->start(1000);
                return;
            }
        }
        //dx-选择
        QModelIndex modelIndex = indexAt(event->pos());
        if (!modelIndex.isValid())
            return;
        m_index = indexAt(event->pos());
        m_currentRow = m_currentRow==-1?0:m_currentRow;
        if(Qt::LeftButton == event->button() || Qt::MidButton == event->button()){
            //ctrl+左/中键
            if(Qt::ControlModifier == event->modifiers()){
                //ctrl+mousepress，如果当前是shift+key/shift+mousepress,清空多余选中
                //当前点击位置为最后一个选择，不做处理
                if(selectedIndexes().count()==1 && selectedIndexes().last().row() == indexAt(event->pos()).row()){
                    return;
                }
                //不延续当前状态则清空当前选中
                if(getModifierState()!=ModifierState::noModifier && getModifierState()!= ModifierState::ctrlModifier){
                    clearSelection();
                    //dx-多选详情页
                    setCurrentIndex(modelIndex.row());

                    selectionModel()->select(m_pSortViewFilter->index(modelIndex.row(), 0), QItemSelectionModel::Select);
                    //普通详情页
                    //从shift切换到ctrl/ctrl切换到shift状态时，未刷新详情页状态
                    changeRightView(false);
                }else {

                    if(selectedIndexes().contains(modelIndex)){
                        selectionModel()->select(m_pSortViewFilter->index(modelIndex.row(), 0), QItemSelectionModel::Deselect);
                        //dx-刷新详情页
                        if(selectedIndexes().count()==1){
                            //普通详情页
                            changeRightView(false);
                        }else {
                            //多选详情页
                            changeRightView();
                        }
                    }else {
                        selectionModel()->select(m_pSortViewFilter->index(modelIndex.row(), 0), QItemSelectionModel::Select);
                        //多选详情页
                        changeRightView();
                    }
                }
                //更新当前状态
                setModifierState(ModifierState::ctrlModifier);
                m_currentRow = modelIndex.row();
                m_shiftSelection = -1;
                return;
            }
            //shift+左/中键
            else if (Qt::ShiftModifier == event->modifiers()) {
                handleShiftAndPress(modelIndex);
                selectionModel()->select(m_pSortViewFilter->index(modelIndex.row(), 0), QItemSelectionModel::Select);
                //dx-刷新详情页
                if(modelIndex.row()!=m_currentRow){
                    //多选详情页
                    changeRightView();
                }else {
                    //普通详情页
                    changeRightView(false);
                }
                return;
            }
            //仅左/中键
            else {
                setTouchState(TouchPressing);
                setModifierState(ModifierState::noModifier);
                setMouseState(MouseState::pressing);
                //普通详情页
//                emit requestReFreshRightView(false);
                return;
            }
        }
        //右键press
        else {
            if(MenuStatus::ReleaseFromMenu == m_menuState){
                m_menuState = MenuStatus::Normal;
                return;
            }
            if(Qt::ShiftModifier == event->modifiers() || Qt::CTRL == event->modifiers()){
                //shift+右键
                if(Qt::ShiftModifier == event->modifiers()){
                    handleShiftAndPress(modelIndex);
                }
                //ctrl+右键
                else {
                    //dx-刷新详情页
                    if(modelIndex.row()!=currentIndex().row()){
                        //多选详情页
                        changeRightView();
                    }
                    selectionModel()->select(m_pSortViewFilter->index(modelIndex.row(), 0), QItemSelectionModel::Select);
                }
                //dx-右键菜单
                m_noteMenu->setWindowOpacity(1);
                //                m_noteMenu->popup(event->globalPos());
                //dx-右键菜单
                onMenuShow(event->globalPos());
            }
            //仅右键
            else if(!m_onlyCurItemMenuEnable || modelIndex == this->currentIndex()){
                //不在选中范围内
                if(!selectedIndexes().contains(modelIndex)){
                    clearSelection();
                    DListView::setCurrentIndex(modelIndex);
                    m_currentRow = modelIndex.row();
                    m_shiftSelection = -1;
                    //多选时，右击未选中，刷新详情页状态
                    changeRightView(false);
                }
                //dx-右键菜单
                m_noteMenu->setWindowOpacity(1);
                //                m_noteMenu->popup(event->globalPos());
                //dx-右键菜单
                onMenuShow(event->globalPos());
            }

            event->setModifiers(Qt::NoModifier);
            setTouchState(TouchState::TouchPressing);
            //        DListView::mousePressEvent(event);
        }
    }else {
        //触控屏手势
        if (event->source() == Qt::MouseEventSynthesizedByQt) {
            //是否弹出右键菜单
            m_popMenuTimer->start(1000);
            return;
        }else {
            if(Qt::RightButton == event->button()){
                m_noteMenu->setWindowOpacity(1);
                //dx-右键菜单
                onMenuShow(event->globalPos());
            }
        }
    }
}

/**
 * @brief MiddleView::mouseReleaseEvent
 * @param event
 */
void MiddleView::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDraging = false;
    //停止计时器
    m_selectCurrentTimer->stop();
    m_popMenuTimer->stop();
    m_menuState = MenuStatus::Normal;
    //dx-选择
    if(TouchState::TouchMoving == m_touchState){
        return;
    }
    QModelIndex index = indexAt(event->pos());
    if(MouseState::pressing == m_mouseState && Qt::NoModifier == event->modifiers()){
        m_currentRow = indexAt(event->pos()).row();
        m_shiftSelection = -1;
        if (index.isValid()){
            clearSelection();
            setCurrentIndex(index.row());
            //dx-刷新详情页
            changeRightView(false);
            setTouchState(TouchState::TouchNormal);
            setMouseState(MouseState::normal);
            return;
        }
    }
    //处理拖拽事件，由于与drop操作参数不同，暂未封装
    if (m_touchState == TouchState::TouchDraging) {
        setTouchState(TouchState::TouchNormal);
        return;
    }
    //正常点击状态，选择当前点击选项 // dx-选择
    if (index.row() != currentIndex().row() && m_touchState == TouchState::TouchPressing && Qt::NoModifier == event->modifiers()) {
        if (index.isValid()){
            setCurrentIndex(index.row());
        }
        setTouchState(TouchState::TouchNormal);
        return;
    }
    setTouchState(TouchState::TouchNormal);
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseReleaseEvent(event);
    }
}

/**
 * @brief MiddleView::mouseDoubleClickEvent
 * @param event
 */
void MiddleView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseDoubleClickEvent(event);
    }
}

//dx-选择
void MiddleView::handleShiftAndPress(QModelIndex &index){
    if(getModifierState()!=ModifierState::noModifier && getModifierState()!= ModifierState::shiftAndMouseModifier){
        clearSelection();
        selectionModel()->select(m_pSortViewFilter->index(m_currentRow, 0), QItemSelectionModel::Select);
    }
    int begin = m_currentRow;
    int end = m_currentRow;

    if (-1 != m_currentRow) {
        if (m_currentRow < index.row()) {
            end = index.row();
        } else if (m_currentRow > index.row()) {
            begin = index.row();
        }
    } else {
        begin = 0;
        end = index.row();
    }

    if (begin < 0)
        return;

    selectionModel()->clear();
    //选中起始位置至点击位置范围内的index
    for (int i = begin; i <= end; i++) {
        selectionModel()->select(m_pSortViewFilter->index(i, 0), QItemSelectionModel::Select);
    }
    //dx-刷新详情页
    if(end>begin ){
        //多选详情页
        changeRightView();
    }

    setModifierState(ModifierState::shiftAndMouseModifier);
}

//dx-选择
void MiddleView::setModifierState(const ModifierState &modifierState)
{
    m_modifierState = modifierState;
}

//dx-选择
MiddleView::ModifierState MiddleView::getModifierState() const
{
    return m_modifierState;
}

//dx-拖拽
void MiddleView::setMouseState(const MouseState &mouseState)
{
    m_mouseState = mouseState;
}

//dx-右键菜单
bool MiddleView::isMultipleSelected()
{
    return (selectedIndexes().count()>1);
}
//dx-右键菜单
bool MiddleView::haveText()
{
    for(auto index:selectedIndexes()){
        VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                                  StandardItemCommon::getStandardItemData(index));
        if(noteData->haveText()){
            return true;
        }
    }
    return false;
}

//dx-右键菜单
bool MiddleView::haveVoice()
{
    for(auto index:selectedIndexes()){
        VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                                  StandardItemCommon::getStandardItemData(index));
        if(noteData->haveVoice()){
            return true;
        }
    }
    return false;
}

//dx-右键菜单
void MiddleView::onMenuShow(QPoint point)
{
    if(isMultipleSelected()){
        ActionManager::Instance()->visibleAction(ActionManager::NoteTop, false);
        ActionManager::Instance()->visibleAction(ActionManager::NoteRename, false);
        ActionManager::Instance()->visibleAction(ActionManager::NoteAddNew, false);
        ActionManager::Instance()->visibleAction(ActionManager::NoteMenuBase, false);
    }else {
        ActionManager::Instance()->visibleAction(ActionManager::NoteTop, true);
        ActionManager::Instance()->visibleAction(ActionManager::NoteRename, true);
        ActionManager::Instance()->visibleAction(ActionManager::NoteAddNew, true);
        ActionManager::Instance()->visibleAction(ActionManager::NoteMenuBase, true);
        m_noteMenu = ActionManager::Instance()->noteContextMenu();
    }
    m_noteMenu->popup(point);
}

//dx-刷新详情页
int MiddleView::getSelectedCount()
{
    return selectedIndexes().count();
}

/**
 * @brief MiddleView::mouseMoveEvent
 * @param event
 */
void MiddleView::mouseMoveEvent(QMouseEvent *event)
{
    if(m_onlyCurItemMenuEnable){
        return;
    }
    //处理触摸屏一指操作
    if ((event->source() == Qt::MouseEventSynthesizedByQt && event->buttons() & Qt::LeftButton)) {
        if(TouchState::TouchOutVisibleRegion !=  m_touchState){
            doTouchMoveEvent(event);
        }
        return;
    } else if ((event->buttons() & Qt::LeftButton) && m_touchState == TouchState::TouchPressing) {
        if (!m_isDraging) {
            //dx-选择
            m_shiftSelection = -1;
            if(!selectedIndexes().contains(m_index)){
                //解决点击多选问题
                if(!selectedIndexes().contains(m_index)){
                    clearSelection();
                    selectionModel()->select(m_pSortViewFilter->index(m_index.row(),0), QItemSelectionModel::Select);
                    //dx-刷新详情页
                    m_currentRow = m_index.row();
                    changeRightView(false);
                }
            }

            //需判断移动距离
            if(qAbs(event->pos().x() - m_touchPressPoint.x()) > 3
                    || qAbs(event->pos().y() - m_touchPressPoint.y()) > 3){
                handleDragEvent(false);
            }
        }
    } else {
        return;
//        DListView::mouseMoveEvent(event);
    }
}

/**
 * @brief LeftView::handleTouchSlideEvent  处理触摸屏move事件，区分点击、滑动、拖拽、长按功能
 * @param timeParam 时间间隔
 * @param distY 纵向移动距离
 * @param point 当前时间发生位置
 */
void MiddleView::handleTouchSlideEvent(qint64 timeParam, double distY, QPoint point)
{
    //根据移动距离和时间计算滑动速度，设置滚动步长
    double slideSpeed = ((qAbs(distY)) / timeParam) + 0.3;
    verticalScrollBar()->setSingleStep(static_cast<int>(20 * slideSpeed));
    verticalScrollBar()->triggerAction((distY > 0) ? QScrollBar::SliderSingleStepSub : QScrollBar::SliderSingleStepAdd);
    m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_touchPressPoint = point;
}

/**
 * @brief MiddleView::eventFilter
 * @param o
 * @param e
 * @return false 不过滤，事件正常处理
 */
bool MiddleView::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);
    if (e->type() == QEvent::FocusIn) {
        m_pItemDelegate->setEditIsVisible(true);
        this->update(currentIndex());
    } else if (e->type() == QEvent::Destroy) {
        m_pItemDelegate->setEditIsVisible(false);
        this->update(currentIndex());
    }
    return false;
}

/**
 * @brief MiddleView::keyPressEvent
 * @param e
 *///dx-选择
void MiddleView::keyPressEvent(QKeyEvent *e)
{
    if (m_onlyCurItemMenuEnable || e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
        e->ignore();
    } else if(Qt::Key_Up == e->key() || Qt::Key_Down == e->key()){
        if(Qt::ShiftModifier == e->modifiers() && Qt::Key_Up == e->key()){
            int nextIndex = m_shiftSelection==-1?m_currentRow-1:m_shiftSelection-1;
            if(-1 !=nextIndex)
            {
                if(getModifierState() != ModifierState::shiftAndKeyModifier){
                    clearSelection();
                }
                //多选在下，shift+上
                if (nextIndex>=m_currentRow){
                    scrollTo(m_pSortViewFilter->index(nextIndex,0));
                    selectionModel()->select(m_pSortViewFilter->index(m_shiftSelection, 0), QItemSelectionModel::Deselect);
                    //普通详情页
                    if(nextIndex == m_currentRow){
                        changeRightView(false);
                    }
                    selectionModel()->select(m_pSortViewFilter->index(m_currentRow, 0), QItemSelectionModel::Select);
                }
                //多选在上，shift+上
                else {
                    scrollTo(m_pSortViewFilter->index(nextIndex,0));
                    selectionModel()->select(m_pSortViewFilter->index(nextIndex, 0), QItemSelectionModel::Select);
                    selectionModel()->select(m_pSortViewFilter->index(m_currentRow, 0), QItemSelectionModel::Select);
                    //多选详情页
                    changeRightView();
                }
                m_shiftSelection = nextIndex;
                setModifierState(ModifierState::shiftAndKeyModifier);
            }
        }else if (Qt::ShiftModifier == e->modifiers() && Qt::Key_Down == e->key()) {
            int nextIndex = m_shiftSelection==-1?m_currentRow+1: m_shiftSelection +1;
            if(count() !=nextIndex)
            {
                if(getModifierState() != ModifierState::shiftAndKeyModifier){
                    clearSelection();
                }
                //多选在上，shift+下
                if(nextIndex<=m_currentRow){
                    scrollTo(m_pSortViewFilter->index(nextIndex,0));
                    selectionModel()->select(m_pSortViewFilter->index(m_shiftSelection, 0), QItemSelectionModel::Deselect);
                    //普通详情页
                    if(nextIndex == m_currentRow){
                        changeRightView(false);
                    }
                    selectionModel()->select(m_pSortViewFilter->index(m_currentRow, 0), QItemSelectionModel::Select);
                }
                //多选在下，shift+下
                else {
                    scrollTo(m_pSortViewFilter->index(nextIndex,0));
                    selectionModel()->select(m_pSortViewFilter->index(nextIndex, 0), QItemSelectionModel::Select);
                    //多选详情页
                    selectionModel()->select(m_pSortViewFilter->index(m_currentRow, 0), QItemSelectionModel::Select);
                    changeRightView();
                }
                m_shiftSelection = nextIndex;
                setModifierState(ModifierState::shiftAndKeyModifier);
            }
        }else {
            setModifierState(ModifierState::noModifier);
            DListView::keyPressEvent(e);
            //dx
            changeRightView(false);
            //由于启用多选，导致键盘操作不会清空和实现选中效果，在此处实现
            clearSelection();
            m_shiftSelection = -1;
            m_currentRow = currentIndex().row();
            selectionModel()->select(m_pSortViewFilter->index(currentIndex().row(), 0), QItemSelectionModel::Select);
        }
    }else {
        //dx-全选
        setModifierState(ModifierState::noModifier);
        if(Qt::CTRL == e->modifiers() && Qt::Key_A == e->key()){
            for(int i = 0;i<count();i++){
                selectionModel()->select(m_pSortViewFilter->index(i, 0), QItemSelectionModel::Select);
            }
            if(count()>1)
                changeRightView();
        }
    }
}

/**
 * @brief MiddleView::initUI
 */
void MiddleView::initUI()
{
    //TODO:
    //    HQ & scaler > 1 have one line at
    //the footer of DListView,so add footer
    //to solve this bug
    QWidget *footer = new QWidget(this);
    footer->setFixedHeight(1);
    addFooterWidget(footer);
    //    this->installEventFilter(this);
}

/**
 * @brief MiddleView::initConnections
 */
void MiddleView::initConnections()
{
    //右键菜单滑动
    connect(m_noteMenu, &VNoteRightMenu::menuTouchMoved, this, &MiddleView::handleDragEvent);
    //右键菜单释放
    connect(m_noteMenu, &VNoteRightMenu::menuTouchReleased, this, [ = ] {
        m_touchState = TouchState::TouchNormal;
        m_menuState = MenuStatus::ReleaseFromMenu;
    });
    //定时器用于判断是否选中当前
    m_selectCurrentTimer = new QTimer();
    connect(m_selectCurrentTimer,&QTimer::timeout,[=]{
        if (m_touchState == TouchState::TouchPressing && m_index.isValid())
            //dx-选择
            if(!selectedIndexes().contains(m_index)){
                //dx-刷新详情页
                this->clearSelection();
                m_currentRow = m_index.row();
                changeRightView(false);
                this->setCurrentIndex(m_index.row());
            }

        m_selectCurrentTimer->stop();
    });
     //定时器用于判断是否弹出菜单
    m_popMenuTimer = new QTimer();
    connect(m_popMenuTimer,&QTimer::timeout,[=]{
        if (m_touchState == TouchState::TouchPressing && m_index.isValid()){
            m_noteMenu->setWindowOpacity(1);
            onMenuShow(QCursor::pos());
        }
        m_popMenuTimer->stop();
    });
}

/**
 * @brief MiddleView::setVisibleEmptySearch
 * @param visible true 显示搜索无结果界面
 */
void MiddleView::setVisibleEmptySearch(bool visible)
{
    if (visible && m_emptySearch == nullptr) {
        m_emptySearch = new DLabel(this);
        m_emptySearch->setText(DApplication::translate("MiddleView", "No search results"));
        m_emptySearch->setAlignment(Qt::AlignCenter);
        DFontSizeManager::instance()->bind(m_emptySearch, DFontSizeManager::T6);
        m_emptySearch->setForegroundRole(DPalette::PlaceholderText);
        m_emptySearch->setVisible(visible);
        QVBoxLayout *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_emptySearch);
        this->setLayout(layout);
    } else if (m_emptySearch) {
        m_emptySearch->setVisible(visible);
    }
}

/**
 * @brief MiddleView::setOnlyCurItemMenuEnable
 * @param enable true 只有选中项右键菜单可弹出
 */
void MiddleView::setOnlyCurItemMenuEnable(bool enable)
{
    m_onlyCurItemMenuEnable = enable;
    m_pItemDelegate->setEnableItem(!enable);
    this->update();
}

/**
 * @brief MiddleView::closeEditor
 * @param editor
 * @param hint
 */
void MiddleView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    Q_UNUSED(hint);
    DListView::closeEditor(editor, QAbstractItemDelegate::NoHint);
}

/**
 * @brief MiddleView::closeMenu
 */
void MiddleView::closeMenu()
{
    m_noteMenu->close();
}

/**
 * @brief MiddleView::noteStickOnTop
 */
void MiddleView::noteStickOnTop()
{
    QModelIndex index = this->currentIndex();
    VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index));
    if (noteData) {
        VNoteItemOper noteOper(noteData);
        if (noteOper.updateTop(!noteData->isTop)) {
            sortView();
        }
    }
}

/**
 * @brief MiddleView::sortView
 * @param adjustCurrentItemBar true 调整当前滚动条
 */
void MiddleView::sortView(bool adjustCurrentItemBar)
{
    m_pSortViewFilter->sortView();
    if (adjustCurrentItemBar) {
        this->scrollTo(currentIndex(), DListView::PositionAtBottom);
    }
}

/**
 * @brief MiddleView::getAllSelectNote
 * @return 选中的笔记列表
 *///dx-右键移动
QModelIndexList MiddleView::getAllSelectNote()
{
    if(selectedIndexes().count()){
        return  selectedIndexes();
    }else {
        return QModelIndexList();
    }
}

/**
 * @brief MiddleView::deleteModelIndexs
 * @param indexs　需要删除的笔记列表
 */
//dx-右键移动
void MiddleView::deleteModelIndexs(const QModelIndexList &indexs)
{
    QModelIndexList indexList = indexs;
    qSort(indexList);
    for(int i = indexList.size()-1;i>-1;i--){
        m_pSortViewFilter->removeRow(indexList[i].row());
    }
}

/**
 * @brief MiddleView::triggerDragNote
 * 触发拖动操作
 */
void MiddleView::triggerDragNote()
{
    //dx-选择
    if(!m_index.isValid()){
        m_isDraging = true;
        return;
    }
    //dx-拖拽移动
    QList<VNoteItem *>noteDataList = getCurrVNotedataList();
    //dx-拖拽移动
    if (noteDataList.size()) {
        if (m_MoveView == nullptr) {
            m_MoveView = new MoveView(this);
        }
        //dx-拖拽取消后选中
        QModelIndexList modelList = selectedIndexes();
        m_MoveView->setNotesNumber(modelList.count());
        //dx-拖拽移动
        m_MoveView->setNoteList(noteDataList);
        //重设视图大小
        m_MoveView->setFixedSize(282,91);
        QPixmap pixmap = m_MoveView->grab();
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(NOTES_DRAG_KEY, QByteArray());
        drag->setMimeData(mimeData);
        drag->setPixmap(pixmap);
        //解决高分屏显示与鼠标位置不对应问题，使用固定位置
        drag->setHotSpot(QPoint(18,25));
        drag->exec(Qt::MoveAction);
        drag->deleteLater();
        //隐藏菜单
        m_noteMenu->hide();
        //dx-拖拽取消后选中
        if(m_dragSuccess){
        //dx-移除后选中
            selectAfterRemoved();
        }else {
            for(auto index : modelList){
                selectionModel()->select(m_pSortViewFilter->index(index.row(), 0), QItemSelectionModel::Select);
            }
        }
        setDragSuccess(false);
    }
}

/**
 * @brief LeftView::doTouchMoveEvent  处理触摸屏move事件，区分点击、滑动、拖拽、长按功能
 * @param event
 */
void MiddleView::doTouchMoveEvent(QMouseEvent *event)
{
    //处理触摸屏单指move事件，区分滑动、拖拽事件
//    m_pItemDelegate->setDraging(false);
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
            setTouchState(TouchState::TouchOutVisibleRegion);
        } else if (qAbs(distY) > 5) {
            handleTouchSlideEvent(timeParam, distY, event->pos());
        }
        break;
    default:
        break;
    }
}

/**
 * @brief LeftView::handleDragEvent
 * @param isTouch 是否触屏
 */
void MiddleView::handleDragEvent(bool isTouch)
{
    //dx-移除后选中
    setNextSelection();

    if(m_onlyCurItemMenuEnable)
        return;
    if(isTouch){
        setTouchState(TouchState::TouchDraging);
    }
    m_popMenuTimer->stop();
    m_noteMenu->setWindowOpacity(0.0);
    triggerDragNote();
}

//dx-刷新详情页
void MiddleView::changeRightView(bool isMultipleDetailPage)
{
    emit requestChangeRightView(isMultipleDetailPage);
}

/**
 * @brief MiddleView::setDragSuccess
 * @param dragSuccess
 *///dx-拖拽取消后选中
void MiddleView::setDragSuccess(bool dragSuccess)
{
    m_dragSuccess = dragSuccess;
}

/**
 * @brief MiddleView::setNextSelection
 * @param
 *///dx-移除后选中
void MiddleView::setNextSelection()
{
    QModelIndexList indexList = selectedIndexes();
    qSort(indexList);
    if(indexList.count()){
        m_nextSelection = indexList.first().row();
    }
}

/**
 * @brief MiddleView::selectAfterRemoved
 * @param
 *///dx-移除后选中
void MiddleView::selectAfterRemoved(){
    clearSelection();
    if(m_pSortViewFilter->index(m_nextSelection,0).isValid()){
        setCurrentIndex(m_nextSelection);
        m_currentRow = m_nextSelection;
    }else if(m_pSortViewFilter->index(m_nextSelection -1 ,0).isValid()){
        setCurrentIndex(m_nextSelection-1);
        m_currentRow = m_nextSelection-1;
    }
    //dx-滚动到当前选中
    scrollTo(currentIndex());
    //dx-移除后选中
    changeRightView(false);
    return;
}
