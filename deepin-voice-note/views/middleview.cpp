#include "middleview.h"
#include "common/utils.h"
#include "common/vnotedatamanager.h"

#include <QMouseEvent>

#include <DLog>

MiddleView::MiddleView(QWidget *parent)
    : DListView(parent)
{
    initMenuAction();
    initModel();
    initDelegate();
    initConnection();
}

void MiddleView::initMenuAction()
{
    m_contextMenu = new DMenu(this);
    m_renameAction = new QAction(DApplication::translate("MiddleView","Rename"), this);
    m_delAction = new QAction(DApplication::translate("MiddleView","Delete"), this);
    m_contextMenu->addAction(m_renameAction);
    m_contextMenu->addAction(m_delAction);
    m_delDialog = new VNoteMessageDialog(VNoteMessageDialog::DeleteFolder,this);
}

void MiddleView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);
    m_pSortFilterModel = new LeftViewSortFilterModel(this);
    m_pSortFilterModel->setSourceModel(m_pDataModel);
    this->setModel(m_pSortFilterModel);
}

void MiddleView::initDelegate()
{
    m_pItemDelegate = new LeftViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

void MiddleView::initConnection()
{
    connect(m_renameAction, &QAction::triggered, this, &MiddleView::handleRenameItem);
    connect(m_delAction, &QAction::triggered, this, &MiddleView::handleDeleteItem);
    connect(m_pItemDelegate, &LeftViewDelegate::sigFolderRename, this,
            &MiddleView::handleFolderRename);
}

void MiddleView::mousePressEvent(QMouseEvent *event)
{
    DListView::mouseMoveEvent(event);
    if ((event->button() == Qt::RightButton) && (this->count() > 0)) {
        QPoint pos = mapToParent(event->pos());
        QModelIndex index = this->indexAt(pos);
        if (index.isValid()) {
            this->setCurrentIndex(index);
            m_contextMenu->exec(event->globalPos());
        }
    }
}

void MiddleView::handleRenameItem(bool)
{
    this->edit(this->currentIndex());
}

void MiddleView::handleFolderRename(VNoteFolder *data)
{
    VNoteFolderOper folderOper(data);
    folderOper.renameVNoteFolder(data->name);
    this->update(this->currentIndex());
    emit sigFolderUpdate(data);
}

void MiddleView::handleDeleteItem(bool)
{
    if(m_delDialog->exec() == QDialog::Accepted){
        QModelIndex index = this->currentIndex();
        QVariant var = index.data(Qt::UserRole + 1);
        if (var.isValid()) {
            VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
            m_pSortFilterModel->removeRow(index.row());
            emit sigFolderDel(data);
        }
    }
}

void MiddleView::handleAddFolder()
{
    VNoteFolderOper folderOper;

    VNoteFolder itemData;
    itemData.defaultIcon = folderOper.getDefaultIcon();
    itemData.UI.icon = folderOper.getDefaultIcon(itemData.defaultIcon);
    itemData.name = folderOper.getDefaultFolderName();

    VNoteFolder *newFolder = folderOper.addFolder(itemData);

    if (nullptr != newFolder) {
        QStandardItem *pItem = new QStandardItem;
        pItem->setData(QVariant::fromValue(static_cast<void *>(newFolder)), Qt::UserRole + 1);
        m_pDataModel->insertRow(0, pItem);

        QModelIndex index = m_pSortFilterModel->index(0, 0);
        this->setCurrentIndex(index);
        this->edit(index);
//        emit sigFolderAdd(newFolder);
    } else {
        qCritical() << __FUNCTION__ << "Add new folder failed:" << itemData.name;
    }
}

void MiddleView::sortView(LeftViewSortFilterModel::OperaType Type, Qt::SortOrder order)
{
    m_pSortFilterModel->sortView(Type, 0, order);
    QModelIndex index = m_pSortFilterModel->index(0, 0);
    if (index.isValid()) {
        this->setCurrentIndex(index);
    }
}

void MiddleView::setCreateTimeFilter(const QDateTime &begin, const QDateTime &end,
                                   QList<qint64> *whilteList)
{
    m_pSortFilterModel->setCreateTimeFilter(begin, end, whilteList);
    QModelIndex nowIndex = m_pSortFilterModel->index(0, 0);
    this->reset();
    this->setCurrentIndex(nowIndex);
}

void MiddleView::setUpdateTimeFilter(const QDateTime &begin, const QDateTime &end,
                                   QList<qint64> *whilteList)
{
    m_pSortFilterModel->setModifyTimeFilter(begin, end, whilteList);
    QModelIndex nowIndex = m_pSortFilterModel->index(0, 0);
    this->reset();
    this->setCurrentIndex(nowIndex);
}

void MiddleView::setFolderNameFilter(const QRegExp &searchKey, QList<qint64> *whilteList)
{
    if(!searchKey.isEmpty() && m_searchKey != searchKey){
        m_pSortFilterModel->setFolderNameFilter(searchKey, whilteList);
        m_pItemDelegate->updateSearchKeyword(searchKey);
        QModelIndex nowIndex = m_pSortFilterModel->index(0, 0);
        this->reset();
        this->setCurrentIndex(nowIndex);
        m_searchKey = searchKey;
    }
}

void MiddleView::clearFilter()
{
    if(!m_searchKey.isEmpty()){
        m_pSortFilterModel->clearFilter();
        m_pItemDelegate->resetKeyword();
        this->reset();
        QModelIndex index = m_pSortFilterModel->index(0, 0);
        this->setCurrentIndex(index);
        m_searchKey = QRegExp();
    }
}

int MiddleView::loadNoteFolder()
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders != nullptr) {
        // Need lock readlock when read data
        folders->lock.lockForRead();

        for (auto &it : folders->folders) {
            QStandardItem *pItem = new QStandardItem;
            pItem->setData(QVariant::fromValue(static_cast<void *>(it)), Qt::UserRole + 1);
            m_pDataModel->appendRow(pItem);
        }
        int count = folders->folders.size();
        folders->lock.unlock();

        sortView(LeftViewSortFilterModel::OperaType::createTime, Qt::DescendingOrder);

        QModelIndex index = m_pSortFilterModel->index(0, 0);
        this->setCurrentIndex(index);
        return count;
    }
    return 0;
}

qint64 MiddleView::getFolderId(const QModelIndex &index)
{
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 1);
        VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
        return data == nullptr ? -1 : data->id;
    }
    return -1;
}

int MiddleView::getFolderCount()
{
    int count = 0;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        folders->lock.lockForRead();
        count = folders->folders.size();
        folders->lock.unlock();
    }
    return count;
}

void MiddleView::removeFromWhiteList(qint64 id)
{
    if(m_pSortFilterModel->removeFromWhiteList(id)){
        QModelIndex nowIndex = m_pSortFilterModel->index(0, 0);
        this->reset();
        this->setCurrentIndex(nowIndex);
    }
}

void MiddleView::setFolderEnable(bool enable)
{
    m_pItemDelegate->setItemEnable(enable);
    this->setEnabled(enable);
}
