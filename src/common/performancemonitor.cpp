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
#include "performancemonitor.h"
#include <QDateTime>
#include <QDebug>

static qint64 initializeAppStartMs = 0;
static qint64 initializeAppFinishMs = 0;

/**
 * @brief PerformanceMonitor::initializeAppStart
 * 记录程序初始化开始时间
 */
void PerformanceMonitor::initializeAppStart()
{
    QDateTime current = QDateTime::currentDateTime();
    initializeAppStartMs = current.toMSecsSinceEpoch();
}

/**
 * @brief PerformanceMonitor::initializeAppFinish
 *  记录程序初始化结束时间
 */
void PerformanceMonitor::initializeAppFinish()
{
    QDateTime current = QDateTime::currentDateTime();
    initializeAppFinishMs = current.toMSecsSinceEpoch();
    qint64 time = initializeAppFinishMs - initializeAppStartMs;
    qInfo() << QString("[GRABPOINT] POINT-01 startduration=%1ms").arg(time);
}
