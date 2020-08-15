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
#include "loadfolderworker.h"
#include "common/vnoteforlder.h"
#include "db/vnotefolderoper.h"
#include "globaldef.h"

#include <DLog>

/**
 * @brief LoadFolderWorker::LoadFolderWorker
 * @param parent
 */
LoadFolderWorker::LoadFolderWorker(QObject *parent)
    : VNTask(parent)
{
}

/**
 * @brief LoadFolderWorker::run
 */
void LoadFolderWorker::run()
{
    static struct timeval start, backups, end;

    gettimeofday(&start, nullptr);
    backups = start;

    VNoteFolderOper folderOper;
    VNOTE_FOLDERS_MAP *foldersMap = folderOper.loadVNoteFolders();

    gettimeofday(&end, nullptr);

    qDebug() << "LoadFolderWorker(ms):" << TM(start, end);

    //TODO:
    //    Add load folder code here

    qDebug() << __FUNCTION__ << " load folders ok:" << foldersMap << " thread id:" << QThread::currentThreadId();

    emit onFoldersLoaded(foldersMap);
}
