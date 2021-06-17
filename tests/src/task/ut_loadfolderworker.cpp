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
#include "ut_loadfolderworker.h"
#include "task/loadfolderworker.h"

ut_loadfolderworker_test::ut_loadfolderworker_test(QObject *parent)
    : QObject(parent)
{
}

void ut_loadfolderworker_test::onFolderLoad(VNOTE_FOLDERS_MAP *folders)
{
    if (folders) {
        delete folders;
        folders = nullptr;
    }
}

TEST_F(ut_loadfolderworker_test, run)
{
    LoadFolderWorker work;
    connect(&work, &LoadFolderWorker::onFoldersLoaded, this, &ut_loadfolderworker_test::onFolderLoad);
    work.run();
}
