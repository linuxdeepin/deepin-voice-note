#include "leftviewsortfilter.h"
#include "common/vnoteforlder.h"
#include "common/standarditemcommon.h"

LeftViewSortFilter::LeftViewSortFilter(QObject *parent)
    :QSortFilterProxyModel (parent)
{

}

bool LeftViewSortFilter::lessThan(
        const QModelIndex &source_left,
        const QModelIndex &source_right) const
{

    StandardItemCommon::StandardItemType leftSourceType = StandardItemCommon::getStandardItemType(source_left);
    StandardItemCommon::StandardItemType rightSourceType = StandardItemCommon::getStandardItemType(source_left);

    if(leftSourceType == StandardItemCommon::NOTEPADITEM &&
            rightSourceType == StandardItemCommon::NOTEPADITEM){
        VNoteFolder *leftSource = reinterpret_cast<VNoteFolder*>(
                    StandardItemCommon::getStandardItemData(source_left)
                    );

        VNoteFolder *rightSource = reinterpret_cast<VNoteFolder*>(
                    StandardItemCommon::getStandardItemData(source_right)
                    );
        return  leftSource->createTime < rightSource->createTime;
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

