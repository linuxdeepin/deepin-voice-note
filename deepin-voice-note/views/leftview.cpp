#include "leftview.h"
#include "common/vnotedatamanager.h"
#include "common/utils.h"
#include "db/vnotefolderoper.h"

#include<QMouseEvent>

#include <DLog>

LeftView::LeftView(QWidget *parent)
 : DListView(parent)
{
    initMenuAction();
    initModel();
    initDelegate();
    initConnection();
}
void LeftView::initMenuAction()
{
    m_contextMenu = new DMenu(this);
    m_renameAction = new QAction(tr("Rename"), this);
    m_delAction = new QAction(tr("Delete"), this);
    m_contextMenu->addAction(m_renameAction);
    m_contextMenu->addAction(m_delAction);
}
void LeftView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);
    m_pSortFilterModel = new LeftViewSortFilterModel(this);
    m_pSortFilterModel->setSourceModel(m_pDataModel);
    this->setModel(m_pSortFilterModel);
}
void LeftView::initDelegate()
{
    m_pItemDelegate = new LeftViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}
void LeftView::initConnection()
{
    connect(m_renameAction, &QAction::triggered, this, &LeftView::handleRenameItem);
    connect(m_delAction, &QAction::triggered, this, &LeftView::handleDeleteItem);
    connect(m_pItemDelegate, &LeftViewDelegate::folderRename, this, &LeftView::handleFolderRename);
}
void LeftView::mousePressEvent(QMouseEvent *event)
{
    DListView::mouseMoveEvent(event);
    if ((event->button() == Qt::RightButton) && (this->count() > 0))
    {
        QPoint pos = mapToParent(event->pos());
        QModelIndex index = this->indexAt(pos);
        if (index.isValid())
        {
            this->setCurrentIndex(index);
            m_contextMenu->exec(event->globalPos());
        }
    }
}

void LeftView::handleRenameItem(bool)
{
    this->edit(this->currentIndex());
}
void LeftView::handleFolderRename(const VNoteFolder *data)
{
    emit sigFolderUpdate(data->id);
}
void LeftView::handleDeleteItem(bool)
{
    QModelIndex index = this->currentIndex();
    QVariant var = index.data(Qt::UserRole + 1);
    if (var.isValid())
    {
        VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
        m_pDataModel->removeRow(index.row());
        emit sigFolderDel(data->id);
    }
}
void LeftView::handleAddFolder()
{
    VNoteFolderOper folderOper;

    VNoteFolder itemData;
    itemData.defaultIcon = folderOper.getDefaultIcon();
    itemData.UI.icon = folderOper.getDefaultIcon(itemData.defaultIcon);
    itemData.name = folderOper.getDefaultFolderName();

    VNoteFolder* newFolder = folderOper.addFolder(itemData);

    if (nullptr != newFolder) {
        QStandardItem *pItem = new QStandardItem;
        pItem->setData(QVariant::fromValue(static_cast<void *>(newFolder)), Qt::UserRole + 1);
        m_pDataModel->insertRow(0, pItem);

        QModelIndex index = m_pSortFilterModel->index(0, 0);
        this->setCurrentIndex(index);
        this->edit(index);
        emit sigFolderAdd(newFolder->id);
    } else {
        qCritical() << __FUNCTION__ << "Add new folder failed:" << itemData.name;
    }
}
void LeftView::sortView(LeftViewSortFilterModel::OperaType Type, int column, Qt::SortOrder order)
{
    m_pSortFilterModel->sortView(Type, column, order);
    QModelIndex index = m_pSortFilterModel->index(0, 0);
    if (index.isValid())
    {
        this->setCurrentIndex(index);
    }
}
void LeftView::setCreateTimeFilter(const QDateTime &begin, const QDateTime &end)
{
    m_pSortFilterModel->setCreateTimeFilter(begin, end);
}
void LeftView::setUpdateTimeFilter(const QDateTime &begin, const QDateTime &end)
{
    m_pSortFilterModel->setModifyTimeFilter(begin, end);
}
void LeftView::setFolderNameFilter(QString key)
{
    m_pSortFilterModel->setFolderNameFilter(key);
}
void LeftView::clearFilter()
{
    m_pSortFilterModel->clearFilter();
    QModelIndex index = m_pSortFilterModel->index(0, 0);
    this->setCurrentIndex(index);
}
qint64 LeftView::getNewFolderId()
{
    qint64 id = 0;
    bool find = false;
    do {
        find = false;
//        for (auto &it : *m_mapFolderData)
//        {
//            if (id == it->id)
//            {
//                find = true;
//                id++;
//                break;
//            }
//        }
    } while (find);
    return id;
}

QString LeftView::getNewFolderIconPath()
{
    QString ImgPath;
    int value = QTime(0, 0, 0).secsTo(QTime::currentTime());
    qsrand(static_cast<uint>(value));
    int rand = qrand() % default_FolderData_imgpath.count();
    ImgPath = default_FolderData_imgpath.at(rand);
    return ImgPath;
}
void LeftView::loadNoteFolder()
{
    VNOTE_FOLDERS_MAP* folders = VNoteDataManager::instance()->getNoteFolders();
    if(folders != nullptr) {

        //Need lock readlock when read data
        folders->lock.lockForRead();

        for(auto &it : folders->folders) {
            QStandardItem *pItem = new QStandardItem;
            pItem->setData(QVariant::fromValue(static_cast<void *>(it)), Qt::UserRole + 1);
            m_pDataModel->appendRow(pItem);
        }

        folders->lock.unlock();

        QModelIndex index = m_pSortFilterModel->index(0, 0);
        this->setCurrentIndex(index);
    }
}
qint64 LeftView::getFolderId(const QModelIndex &index)
{
    if(index.isValid())
    {
        QVariant var = index.data(Qt::UserRole + 1);
        VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
        return data == nullptr ? -1 : data->id;
    }
    return  -1;
}
