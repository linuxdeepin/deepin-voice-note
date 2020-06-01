/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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

