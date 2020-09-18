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

#include "ut_vnotedatamanager.h"

#define private public
#define protected public
#include "vnotedatamanager.h"
#undef private
#undef protected

ut_vnotedatamanager_test::ut_vnotedatamanager_test()
{

}

TEST_F(ut_vnotedatamanager_test, folderNotesCount)
{

}

TEST_F(ut_vnotedatamanager_test, isAllDatasReady)
{
    VNoteDataManager vnotedatamanager;
//    vnotedatamanager.reqNoteDefIcons();
//    vnotedatamanager.reqNoteFolders();
//    vnotedatamanager.reqNoteItems();
    vnotedatamanager.isAllDatasReady();
}

TEST_F(ut_vnotedatamanager_test, instance)
{
    VNoteDataManager* vnotedatamanager;
    vnotedatamanager = VNoteDataManager::instance();
}
