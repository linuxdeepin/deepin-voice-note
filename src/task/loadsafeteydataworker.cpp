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
#include "loadsafeteydataworker.h"
#include "db/vnotesaferoper.h"
#include "globaldef.h"

#include <DLog>

/**
 * @brief LoadSafeteyDataWorker::LoadSafeteyDataWorker
 * @param parent
 */
LoadSafeteyDataWorker::LoadSafeteyDataWorker(QObject *parent)
    : VNTask(parent)
{
}

/**
 * @brief LoadSafeteyDataWorker::run
 */
void LoadSafeteyDataWorker::run()
{
    struct timeval start, end;
    gettimeofday(&start, nullptr);

    //TODO:
    //    Add load folder code here
    VNoteSaferOper saferOper;
    SafetyDatas *safers = saferOper.loadSafers();

    gettimeofday(&end, nullptr);

    qDebug() << "Load safers ok:" << safers->size()
             << " cost time(ms):" << TM(start, end)
             << " thread id:" << QThread::currentThreadId();

    emit saferLoaded(safers);
}
