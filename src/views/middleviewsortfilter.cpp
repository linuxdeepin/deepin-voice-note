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
