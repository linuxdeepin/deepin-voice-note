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

UT_LoadFolderWorker::UT_LoadFolderWorker(QObject *parent)
    : QObject(parent)
{
}

void UT_LoadFolderWorker::onFolderLoad(VNOTE_FOLDERS_MAP *folders)
{
    EXPECT_TRUE(nullptr != folders);
    if (folders) {
        delete folders;
        folders = nullptr;
    }
}

TEST_F(UT_LoadFolderWorker, UT_LoadFolderWorker_run_001)
{
    LoadFolderWorker work;
    connect(&work, &LoadFolderWorker::onFoldersLoaded, this, &UT_LoadFolderWorker::onFolderLoad);
    work.run();
}
