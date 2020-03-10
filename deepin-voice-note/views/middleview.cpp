#include "middleview.h"
#include "middleviewdelegate.h"
#include "common/actionmanager.h"

#include <QMouseEvent>
#include <QVBoxLayout>

#include <DApplication>

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
    this->setModel(m_pDataModel);
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

QStandardItemModel *MiddleView::getStandardItemModel()
{
    return m_pDataModel;
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
            this->setCurrentIndex(index);
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
