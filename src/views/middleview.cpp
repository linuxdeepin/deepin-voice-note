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
 */
VNoteItem *MiddleView::deleteCurrentRow()
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index));

    m_pSortViewFilter->removeRow(index.row());

    return noteData;
}

/**
 * @brief MiddleView::getCurrVNotedata
 * @return 当前选中的记事项数据
 */
VNoteItem *MiddleView::getCurrVNotedata() const
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index));

    return noteData;
}

/**
 * @brief MiddleView::onNoteChanged
 */
void MiddleView::onNoteChanged()
{
    sortView();
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
 */
void MiddleView::saveAsText()
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index));
    if (nullptr != noteData) {
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
                exportDir, ExportNoteWorker::ExportText, noteData);
            exportWorker->setAutoDelete(true);

            QThreadPool::globalInstance()->start(exportWorker);
        }
    }
}

/**
 * @brief MiddleView::saveRecords
 */
void MiddleView::saveRecords()
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast<VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index));
    if (nullptr != noteData) {
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
                exportDir, ExportNoteWorker::ExportAllVoice, noteData);
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
    //触控屏手势
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        //记录此时触控点的位置，用于move事件中滑动距离与速度
        m_touchPressPoint = event->pos();
        m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
        //更新触摸状态
        setTouchState(TouchPressing);
        m_index = indexAt(event->pos());
        m_noteMenu->setPressPoint(QCursor::pos());
        //检查是否选中
        m_selectCurrentTimer->start(250);
        //是否弹出右键菜单
        m_popMenuTimer->start(1000);
        return;
    }

    if (!m_onlyCurItemMenuEnable) {
        event->setModifiers(Qt::NoModifier);
        setTouchState(TouchState::TouchPressing);
//        DListView::mousePressEvent(event);
    }

    if (event->button() == Qt::RightButton) {
        if(MenuStatus::ReleaseFromMenu == m_menuState){
            m_menuState = MenuStatus::Normal;
            return;
        }
        QModelIndex index = this->indexAt(event->pos());
        if (index.isValid()
                && (!m_onlyCurItemMenuEnable || index == this->currentIndex())) {
            setCurrentIndex(index.row());
            m_noteMenu->popup(event->globalPos());
            //通过此方法隐藏菜单，在处理拖拽事件结束后hide
            m_noteMenu->setWindowOpacity(1);
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
    //处理拖拽事件，由于与drop操作参数不同，暂未封装
    if (m_touchState == TouchState::TouchDraging) {
        setTouchState(TouchState::TouchNormal);
        return;
    }
    //正常点击状态，选择当前点击选项
    QModelIndex index = indexAt(event->pos());
    if (index.row() != currentIndex().row() && m_touchState == TouchState::TouchPressing) {
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

/**
 * @brief MiddleView::mouseMoveEvent
 * @param event
 */
void MiddleView::mouseMoveEvent(QMouseEvent *event)
{
    //处理触摸屏一指操作
    if ((event->source() == Qt::MouseEventSynthesizedByQt && event->buttons() & Qt::LeftButton)) {
        if(TouchState::TouchOutVisibleRegion !=  m_touchState){
            doTouchMoveEvent(event);
        }
        return;
    } else if ((event->buttons() & Qt::LeftButton) && m_touchState == TouchState::TouchPressing) {
        if (!m_isDraging) {
            setCurrentIndex(indexAt(event->pos()).row());
            //需判断移动距离
            if(qAbs(event->pos().x() - m_touchPressPoint.x()) > 3
                    || qAbs(event->pos().y() - m_touchPressPoint.y()) > 3){
                handleDragEvent(false);
            }
        }
    } else {
        DListView::mouseMoveEvent(event);
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
 */
void MiddleView::keyPressEvent(QKeyEvent *e)
{
    if (m_onlyCurItemMenuEnable || e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
        e->ignore();
    } else {
        DListView::keyPressEvent(e);
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
            this->setCurrentIndex(m_index.row());
        m_selectCurrentTimer->stop();
    });
     //定时器用于判断是否弹出菜单
    m_popMenuTimer = new QTimer();
    connect(m_popMenuTimer,&QTimer::timeout,[=]{
        if (m_touchState == TouchState::TouchPressing && m_index.isValid()){
            m_noteMenu->setWindowOpacity(1);
            m_noteMenu->exec(QCursor::pos());
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
 */
QModelIndexList MiddleView::getAllSelectNote()
{
    QModelIndexList indexList;
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        indexList.append(index);
    }
    return  indexList;
}

/**
 * @brief MiddleView::deleteModelIndexs
 * @param indexs　需要删除的笔记列表
 */
void MiddleView::deleteModelIndexs(const QModelIndexList &indexs)
{
    for (auto &it : indexs) {
        m_pSortViewFilter->removeRow(it.row());
    }
}

/**
 * @brief MiddleView::triggerDragNote
 * 触发拖动操作
 */
void MiddleView::triggerDragNote()
{
    QModelIndex dragIndex = this->indexAt(mapFromGlobal(QCursor::pos()));
    if(!dragIndex.isValid()){
        m_isDraging = true;
        return;
    }
    VNoteItem *noteData = getCurrVNotedata();
    // 判断当前拖拽的笔记是否可用，如果可用，则初始化拖拽操作的数据
    if (noteData) {
        if (m_MoveView == nullptr) {
            m_MoveView = new MoveView(this);
        }
        m_MoveView->setNote(noteData);
        //重设视图大小
        m_MoveView->setFixedSize(260,56);
        QPixmap pixmap = m_MoveView->grab();
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(NOTES_DRAG_KEY, QByteArray());
        drag->setMimeData(mimeData);
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
        drag->exec(Qt::MoveAction);
        drag->deleteLater();
        //隐藏菜单
        m_noteMenu->hide();
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
    if(isTouch){
        setTouchState(TouchState::TouchDraging);
    }
    m_popMenuTimer->stop();
    m_noteMenu->setWindowOpacity(0.0);
    triggerDragNote();
}
