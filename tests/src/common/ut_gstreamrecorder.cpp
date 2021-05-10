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

#include "ut_gstreamrecorder.h"
#include "gstreamrecorder.h"
#include "vnoterecordbar.h"
#include "stub.h"
#include <QDebug>

static void stub_gstreamrecorde_common()
{
    qInfo() << "I am gstreamrecorder common stub";
}

ut_gstreamrecorder_test::ut_gstreamrecorder_test()
{
}

TEST_F(ut_gstreamrecorder_test, createPipe)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, createPipe), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.createPipe();
}

TEST_F(ut_gstreamrecorder_test, GetGstState)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, GetGstState), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.GetGstState(nullptr, nullptr);
}

TEST_F(ut_gstreamrecorder_test, startRecord)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, startRecord), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.startRecord();
}

TEST_F(ut_gstreamrecorder_test, pauseRecord)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, pauseRecord), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.pauseRecord();
}

TEST_F(ut_gstreamrecorder_test, stopRecord)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, stopRecord), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.stopRecord();
}

TEST_F(ut_gstreamrecorder_test, setDevice)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, setDevice), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.setDevice("");
}

TEST_F(ut_gstreamrecorder_test, setStateToNull)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, setStateToNull), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.setStateToNull();
}

TEST_F(ut_gstreamrecorder_test, initFormat)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.initFormat();
}

TEST_F(ut_gstreamrecorder_test, doBusMessage)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, doBusMessage), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.doBusMessage(nullptr);
}

TEST_F(ut_gstreamrecorder_test, doBufferProbe)
{
    Stub stub;
    stub.set(ADDR(GstreamRecorder, doBufferProbe), stub_gstreamrecorde_common);
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.doBufferProbe(nullptr);
}
