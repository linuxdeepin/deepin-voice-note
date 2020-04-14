#include "middleviewsortfilter.h"
#include "common/vnoteitem.h"
#include "common/standarditemcommon.h"

MiddleViewSortFilter::MiddleViewSortFilter(QObject *parent)
    :QSortFilterProxyModel (parent)
{

}

void MiddleViewSortFilter::sortView(MiddleViewSortFilter::sortFeild feild,
                                    int column,
                                    Qt::SortOrder order)
{
    m_sortFeild = feild;

    sort(column, order);
}

bool MiddleViewSortFilter::lessThan(
        const QModelIndex &source_left,
        const QModelIndex &source_right) const
{

    VNoteItem *leftNote = reinterpret_cast<VNoteItem*>(
                StandardItemCommon::getStandardItemData(source_left)
                );

    VNoteItem *rightNote = reinterpret_cast<VNoteItem*>(
                StandardItemCommon::getStandardItemData(source_right)
                );

    if (nullptr != leftNote && nullptr != rightNote) {
        switch (m_sortFeild) {
        case modifyTime:
            return (leftNote->modifyTime < rightNote->modifyTime);
        case createTime:
            return (leftNote->createTime < rightNote->createTime);
        case title:
            return (leftNote->noteTitle < rightNote->noteTitle);
        }
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);

}
