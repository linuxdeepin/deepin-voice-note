/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_loadfolderworker.h"

#define protected public
#include "loadfolderworker.h"
#undef protected

#include <QThreadPool>

ut_loadfolderworker_test::ut_loadfolderworker_test()
{

}

TEST_F(ut_loadfolderworker_test, run)
{
    LoadFolderWorker *loadfolderworker = new LoadFolderWorker();
    loadfolderworker->setAutoDelete(true);

    QThreadPool::globalInstance()->start(loadfolderworker);
}
