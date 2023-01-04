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
#include <QList>

struct VNoteFolder;
//记事本排序过滤
class LeftViewSortFilter : public QSortFilterProxyModel
{
public:
    explicit LeftViewSortFilter(QObject *parent = nullptr);
    //设置不显示项
    void setBlackFolders(const QList<VNoteFolder *> &folders);

protected:
    //处理排序
    virtual bool lessThan(
        const QModelIndex &source_left,
        const QModelIndex &source_right) const override;
    //数据过滤
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    QList<VNoteFolder *> m_blackFolders;
};

#endif // LEFTVIEWSORTFILTER_H
