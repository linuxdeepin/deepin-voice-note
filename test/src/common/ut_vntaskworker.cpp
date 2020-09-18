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

#include "ut_vntaskworker.h"
#include "vntaskworker.h"
#include "vndatasafertask.h"


ut_vntaskworker_test::ut_vntaskworker_test()
{

}

void ut_vntaskworker_test::SetUp()
{
    m_vntaskworker = new VNTaskWorker;
}

void ut_vntaskworker_test::TearDown()
{
    delete m_vntaskworker;
}

TEST_F(ut_vntaskworker_test, addTask)
{
    VDataSafer safer;
    VNDataSaferTask *pSafeTask = new VNDataSaferTask(safer);
    m_vntaskworker->addTask(pSafeTask);
}
