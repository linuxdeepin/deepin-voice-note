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

#include "ut_vnoteforlder.h"
#include "vnoteforlder.h"
#include "vnotedatamanager.h"
#include <QtDebug>

ut_vnoteforlder_test::ut_vnoteforlder_test()
{
}

void ut_vnoteforlder_test::SetUp()
{
    m_vnoteforlder = VNoteDataManager::instance()->getNoteFolders()->folders[0];
}

void ut_vnoteforlder_test::TearDown()
{
    ;
}

TEST_F(ut_vnoteforlder_test, isValid)
{
    m_vnoteforlder->isValid();
}

TEST_F(ut_vnoteforlder_test, maxNoteIdRef)
{
    m_vnoteforlder->maxNoteIdRef();
}

TEST_F(ut_vnoteforlder_test, getNotesCount)
{
    m_vnoteforlder->getNotesCount();
}
