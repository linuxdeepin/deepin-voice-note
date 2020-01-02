#include "leftviewsortfiltermodel.h"
#include "common/vnoteforlder.h"

#include<QDebug>

LeftViewSortFilterModel::LeftViewSortFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    ;
}

void LeftViewSortFilterModel::setCreateTimeFilter(const QDateTime &begin, const QDateTime &end,
                                                  QList<qint64> *whilteList)
{
    m_beginTime = begin;
    m_endTime = end;
    m_filterType = createTime;
    if (whilteList != nullptr) {
        m_whilteList = *whilteList;
    }
    invalidateFilter();
}

void LeftViewSortFilterModel::setModifyTimeFilter(const QDateTime &begin, const QDateTime &end,
                                                  QList<qint64> *whilteList)
{
    m_beginTime = begin;
    m_endTime = end;
    m_filterType = modifyTime;
    if (whilteList != nullptr) {
        m_whilteList = *whilteList;
    }
    invalidateFilter();
}

void LeftViewSortFilterModel::setFolderNameFilter(const QRegExp &searchKey, QList<qint64> *whilteList)
{
    m_filterType = folderName;
    if (whilteList != nullptr) {
        m_whilteList = *whilteList;
    }
    if (!searchKey.isEmpty()) {
        m_folderNameKey = searchKey;
    }
    invalidateFilter();
}

void LeftViewSortFilterModel::clearFilter()
{
    m_filterType = none;
    m_whilteList.clear();
    invalidateFilter();
}

void LeftViewSortFilterModel::sortView(OperaType Type, int column, Qt::SortOrder order)
{
    m_sortType = Type;
    return sort(column, order);
}

bool LeftViewSortFilterModel::lessThan(const QModelIndex &source_left,
                                       const QModelIndex &source_right) const
{
    if (m_sortType != none) {
        QVariant var_left = source_left.data(Qt::UserRole + 1);
        VNoteFolder *data_left = static_cast<VNoteFolder *>(var_left.value<void *>());
        QVariant var_right = source_right.data(Qt::UserRole + 1);
        VNoteFolder *data_right = static_cast<VNoteFolder *>(var_right.value<void *>());
        switch (m_sortType) {
        case folderName:
            return data_left->name < data_right->name;
        case createTime:
            return data_left->createTime < data_right->createTime;
        case modifyTime:
            return data_left->modifyTime < data_right->modifyTime;
        default:
            break;
        }
    }
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

bool LeftViewSortFilterModel::filterAcceptsRow(int source_row,
                                               const QModelIndex &source_parent) const
{
    if (m_filterType != none) {
        QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
        QVariant var = index.data(Qt::UserRole + 1);
        VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
        if (!m_whilteList.isEmpty()) {
            for (auto it : m_whilteList) {
                if (it == data->id) {
                    qDebug() << "id while:" << data->id;
                    return true;
                }
            }
        }
        switch (m_filterType) {
        case folderName:
            return data->name.contains(m_folderNameKey);
        case createTime:
            return data->createTime >= m_beginTime && data->createTime <= m_endTime;
        case modifyTime:
            return data->modifyTime >= m_beginTime && data->modifyTime <= m_endTime;
        default:
            break;
        }
    }
    return true;
}
