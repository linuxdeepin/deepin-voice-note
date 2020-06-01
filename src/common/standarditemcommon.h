/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
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
#ifndef FOLDERTREECOMMON_H
#define FOLDERTREECOMMON_H

#include <QObject>
#include <QStandardItemModel>
class StandardItemCommon : public QObject
{
    Q_OBJECT
public:
    enum StandardItemType {
        Invalid = 0,
        NOTEPADROOT, //记事本一级目录
        NOTEPADITEM, //记事本项
        NOTEITEM    //笔记项
    };
    Q_ENUM(StandardItemType)
    StandardItemCommon();
    static QStandardItem *createStandardItem(void *data, StandardItemType type);
    static StandardItemType getStandardItemType(const QModelIndex &index);
    static void * getStandardItemData(const QModelIndex &index);

};

#endif // FOLDERTREECOMMON_H
