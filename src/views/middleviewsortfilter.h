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

#ifndef MIDDLEVIEWSORTFILTER_H
#define MIDDLEVIEWSORTFILTER_H

#include <QSortFilterProxyModel>
//列表项排序
class MiddleViewSortFilter : public QSortFilterProxyModel
{
public:
    explicit MiddleViewSortFilter(QObject *parent = nullptr);

    enum sortFeild {
        title,
        createTime,
        modifyTime,
    };
    //执行排序
    void sortView(
        sortFeild feild = modifyTime,
        int column = 0,
        Qt::SortOrder order = Qt::DescendingOrder);

protected:
    //处理排序
    virtual bool lessThan(
        const QModelIndex &source_left,
        const QModelIndex &source_right) const override;

    sortFeild m_sortFeild {modifyTime};
};

#endif // MIDDLEVIEWSORTFILTER_H
