// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_vnmainwnddelayinittask.h"
#include "vnmainwnddelayinittask.h"

UT_VNMainWndDelayInitTask::UT_VNMainWndDelayInitTask()
{
}

TEST_F(UT_VNMainWndDelayInitTask, Run_WithNullMainWnd_DoesNotCrash)
{
    VNMainWndDelayInitTask task(nullptr);
    task.run();
    SUCCEED();
}

TEST_F(UT_VNMainWndDelayInitTask, Constructor_DoesNotCrash)
{
    VNMainWndDelayInitTask task(nullptr);
    SUCCEED();
}
