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

#include "ut_vnotesaferoper.h"
#include "vnotesaferoper.h"

ut_vnotesaferoper_test::ut_vnotesaferoper_test()
{

}

void ut_vnotesaferoper_test::SetUp()
{
    m_vnotesaferoper = new VNoteSaferOper;
}

void ut_vnotesaferoper_test::TearDown()
{
    delete m_vnotesaferoper;
}

TEST_F(ut_vnotesaferoper_test, loadSafers)
{
    VDataSafer vdatasafer;
    VNoteSaferOper vnotesaferoper(vdatasafer);
    m_vnotesaferoper->loadSafers();
}

TEST_F(ut_vnotesaferoper_test, addSafer)
{
    VDataSafer vdatasafer;
    m_vnotesaferoper->addSafer(vdatasafer);
    m_vnotesaferoper->rmSafer(vdatasafer);

    vdatasafer.path = "test";
    vdatasafer.folder_id = 1;
    vdatasafer.note_id = 2;
    m_vnotesaferoper->addSafer(vdatasafer);
    m_vnotesaferoper->rmSafer(vdatasafer);
}
