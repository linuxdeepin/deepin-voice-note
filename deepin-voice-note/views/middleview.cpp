#include "middleview.h"
#include "middleviewdelegate.h"
#include "middleviewsortfilter.h"
#include "common/actionmanager.h"
#include "common/standarditemcommon.h"

#include <QMouseEvent>
#include <QVBoxLayout>

#include <DApplication>
#include <DLog>

MiddleView::MiddleView(QWidget *parent)
    : DListView(parent)
{
    initModel();
    initDelegate();
    initMenu();
    initUI();
}

void MiddleView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);

    m_pSortViewFilter = new MiddleViewSortFilter(this);
    m_pSortViewFilter->setDynamicSortFilter(false);
    m_pSortViewFilter->setSourceModel(m_pDataModel);

    this->setModel(m_pSortViewFilter);
}

void MiddleView::initDelegate()
{
    m_pItemDelegate = new MiddleViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

void MiddleView::initMenu()
{
    m_noteMenu = ActionManager::Instance()->noteContextMenu();
}

void MiddleView::setSearchKey(const QRegExp &key)
{
    m_searchKey = key;
    m_pItemDelegate->setSearchKey(key);
}

void MiddleView::setCurrentId(qint64 id)
{
    m_currentId = id;
}

qint64 MiddleView::getCurrentId()
{
    return m_currentId;
}

void MiddleView::addRowAtHead(VNoteItem *note)
{
    if (nullptr != note) {
        QStandardItem *item = StandardItemCommon::createStandardItem(note, StandardItemCommon::NOTEITEM);
        m_pDataModel->insertRow(0, item);

        DListView::setCurrentIndex( m_pSortViewFilter->index(0, 0));
    }
}

void MiddleView::appendRow(VNoteItem *note)
{
    if (nullptr != note) {
        QStandardItem *item = StandardItemCommon::createStandardItem(note, StandardItemCommon::NOTEITEM);
        m_pDataModel->appendRow(item);
    }
}

void MiddleView::clearAll()
{
    m_pDataModel->clear();
}

VNoteItem *MiddleView::deleteCurrentRow()
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast< VNoteItem*>(
                StandardItemCommon::getStandardItemData(index)
                );

    m_pDataModel->removeRow(index.row());

    return noteData;
}

void MiddleView::onNoteChanged()
{
    m_pSortViewFilter->sortView();
}

qint32 MiddleView::rowCount() const
{
    return m_pDataModel->rowCount();
}

void MiddleView::setCurrentIndex(int index)
{
     DListView::setCurrentIndex(m_pSortViewFilter->index(index, 0));
}

void MiddleView::mousePressEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseMoveEvent(event);
    }

    if (event->button() == Qt::RightButton) {
        QModelIndex index = this->indexAt(event->pos());
        if (index.isValid()
                && (!m_onlyCurItemMenuEnable || index == this->currentIndex())) {
            DListView::setCurrentIndex(index);
            m_noteMenu->exec(event->globalPos());
        }
    }
}

void MiddleView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseReleaseEvent(event);
    }
}

void MiddleView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseDoubleClickEvent(event);
    }
}

void MiddleView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseMoveEvent(event);
    }
}

void MiddleView::initUI()
{
    m_emptySearch = new DLabel(this);
    m_emptySearch->setText(DApplication::translate("MiddleView", "No search results"));
    m_emptySearch->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_emptySearch, DFontSizeManager::T4);
    m_emptySearch->setForegroundRole(DPalette::TextTips);
    m_emptySearch->setVisible(false);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_emptySearch);
    this->setLayout(layout);
}

void MiddleView::setVisibleEmptySearch(bool visible)
{
    m_emptySearch->setVisible(visible);
}

void MiddleView::setOnlyCurItemMenuEnable(bool enable)
{
    m_onlyCurItemMenuEnable = enable;
    m_pItemDelegate->setEnableItem(!enable);
    this->update();
}
