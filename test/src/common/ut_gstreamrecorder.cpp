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

#define private public
#include "gstreamrecorder.h"
#undef private

#define protected public
#include "vnoterecordbar.h"
#undef protected

ut_gstreamrecorder_test::ut_gstreamrecorder_test()
{

}

TEST_F(ut_gstreamrecorder_test, createPipe)
{
    GstreamRecorder gstreamrecorder;
    ASSERT_TRUE(gstreamrecorder.createPipe());
}

TEST_F(ut_gstreamrecorder_test, deinit)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.deinit();
}

TEST_F(ut_gstreamrecorder_test, GetGstState)
{
    GstreamRecorder gstreamrecorder;
    int state = -1;
    int pending = -1;
    gstreamrecorder.GetGstState(&state, &pending);
}

TEST_F(ut_gstreamrecorder_test, startRecord)
{
    GstreamRecorder gstreamrecorder;
    QString tmpstr = "alsa_output.pci-0000_00_1f.3.analog-stereo.monitor";
    QString outpath = "/home/zhangteng";
    ASSERT_FALSE(gstreamrecorder.startRecord());
}

TEST_F(ut_gstreamrecorder_test, pauseRecord)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.createPipe();
    gstreamrecorder.pauseRecord();
}

TEST_F(ut_gstreamrecorder_test, stopRecord)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.stopRecord();
}

TEST_F(ut_gstreamrecorder_test, setDevice)
{
    GstreamRecorder gstreamrecorder;
    QString tmpstr = "alsa_output.pci-0000_00_1f.3.analog-stereo.monitortest";
    gstreamrecorder.setDevice(tmpstr);
    ASSERT_TRUE(tmpstr == gstreamrecorder.m_currentDevice);
}

TEST_F(ut_gstreamrecorder_test, objectUnref)
{
    GstreamRecorder gstreamrecorder;
}

TEST_F(ut_gstreamrecorder_test, setStateToNull)
{
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
    GstreamRecorder gstreamrecorder;
}

TEST_F(ut_gstreamrecorder_test, doBufferProbe)
{
    GstreamRecorder gstreamrecorder;
}
