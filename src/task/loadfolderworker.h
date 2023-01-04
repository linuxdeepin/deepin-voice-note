/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
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
#ifndef LOADFOLDERWORKER_H
#define LOADFOLDERWORKER_H

#include "common/datatypedef.h"
#include "vntask.h"

#include <QObject>
#include <QRunnable>

//加载记事本数据线程
class LoadFolderWorker : public VNTask
{
    Q_OBJECT
public:
    explicit LoadFolderWorker(QObject *parent = nullptr);

protected:
    //加载数据
    virtual void run();
signals:
    //完成加载
    void onFoldersLoaded(VNOTE_FOLDERS_MAP *foldesMap);
public slots:
};

#endif // LOADFOLDERWORKER_H
