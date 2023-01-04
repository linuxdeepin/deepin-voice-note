/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     leilong <leilong@uniontech.com>
*
* Maintainer: leilong <leilong@uniontech.com>
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
#include "ut_vnmainwnddelayinittask.h"
#include "vnmainwnddelayinittask.h"
#include "views/vnotemainwindow.h"

UT_VNMainWndDelayInitTask::UT_VNMainWndDelayInitTask()
{
}

TEST_F(UT_VNMainWndDelayInitTask, UT_VNMainWndDelayInitTask_run_001)
{
    VNMainWndDelayInitTask *work = new VNMainWndDelayInitTask(nullptr);
    work->run();

    delete work;
}
