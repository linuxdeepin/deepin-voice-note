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

#ifndef LEFTVIEWSORTFILTER_H
#define LEFTVIEWSORTFILTER_H
#include <QSortFilterProxyModel>
//记事本排序过滤
class LeftViewSortFilter : public QSortFilterProxyModel
{
public:
    LeftViewSortFilter(QObject *parent = nullptr);

protected:
    //处理排序
    virtual bool lessThan(
        const QModelIndex &source_left,
        const QModelIndex &source_right) const override;
};

#endif // LEFTVIEWSORTFILTER_H
