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

#include "ut_vndatasafertask.h"
#include "vndatasafertask.h"

#include <QThreadPool>

ut_vndatasafertask_test::ut_vndatasafertask_test()
{

}

TEST_F(ut_vndatasafertask_test, run)
{
    VDataSafer safer;
    safer.setSaferType(safer.Safe);
    VNDataSaferTask *vndatasafertask = new VNDataSaferTask(safer);
    vndatasafertask->setAutoDelete(true);
    QThreadPool::globalInstance()->start(vndatasafertask);

    safer.setSaferType(safer.Unsafe);
    VNDataSaferTask *vndatasafertask1 = new VNDataSaferTask(safer);
    vndatasafertask1->setAutoDelete(true);
    QThreadPool::globalInstance()->start(vndatasafertask1);

    safer.setSaferType(safer.ExceptionSafer);
    safer.path = "/home/zhangteng/test";
    VNDataSaferTask *vndatasafertask2 = new VNDataSaferTask(safer);
    vndatasafertask2->setAutoDelete(true);
    QThreadPool::globalInstance()->start(vndatasafertask2);
}
