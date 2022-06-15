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
#include "filecleanupworker.h"

UT_VNTaskWorker::UT_VNTaskWorker()
{
}

void UT_VNTaskWorker::SetUp()
{
    m_vntaskworker = new VNTaskWorker;
}

void UT_VNTaskWorker::TearDown()
{
    delete m_vntaskworker;
}

TEST_F(UT_VNTaskWorker, UT_VNTaskWorker_addTask_001)
{
    m_vntaskworker->addTask(nullptr);
}

TEST_F(UT_VNTaskWorker, UT_VNTaskWorker_setWorkerName_001)
{
    QString worke = "aaa";
    m_vntaskworker->setWorkerName(worke);
    EXPECT_EQ("aaa", m_vntaskworker->m_workerName);
}

TEST_F(UT_VNTaskWorker, UT_VNTaskWorker_quitWorker_001)
{
    m_vntaskworker->quitWorker();
    EXPECT_TRUE(m_vntaskworker->m_fQuit);
}

TEST_F(UT_VNTaskWorker, UT_VNTaskWorker_run_001)
{
    m_vntaskworker->m_fQuit = true;
    m_vntaskworker->run();
}
