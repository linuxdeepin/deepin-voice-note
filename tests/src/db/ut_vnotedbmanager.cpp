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

#include "ut_vnotedbmanager.h"
#include "vnotedbmanager.h"
#include "vnotefolderoper.h"
#include "vnoteforlder.h"
#include "vnoteitemoper.h"
#include "vnoteitem.h"

UT_VNoteDbManager::UT_VNoteDbManager()
{
}

TEST_F(UT_VNoteDbManager, UT_VNoteDbManager_initVNoteDb_001)
{
    VNoteDbManager::instance()->initVNoteDb();
    EXPECT_TRUE(VNoteDbManager::instance()->m_vnoteDB.isValid());
}

TEST_F(UT_VNoteDbManager, UT_VNoteDbManager_getVNoteDb_001)
{
    VNoteDbManager::instance()->getVNoteDb();
    EXPECT_TRUE(VNoteDbManager::instance()->m_vnoteDB.isValid());
}
