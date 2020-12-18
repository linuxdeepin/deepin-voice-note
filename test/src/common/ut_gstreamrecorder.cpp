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

ut_gstreamrecorder_test::ut_gstreamrecorder_test()
{
}

void ut_gstreamrecorder_test::SetUp()
{
    m_GstreamRecorder = new GstreamRecorder;
}

void ut_gstreamrecorder_test::TearDown()
{
    delete m_GstreamRecorder;
}

TEST_F(ut_gstreamrecorder_test, createPipe)
{
    m_GstreamRecorder->createPipe();
}

TEST_F(ut_gstreamrecorder_test, deinit)
{
    m_GstreamRecorder->deinit();
}

TEST_F(ut_gstreamrecorder_test, GetGstState)
{
    int state = -1;
    int pending = -1;
    m_GstreamRecorder->GetGstState(&state, &pending);
}

TEST_F(ut_gstreamrecorder_test, startRecord)
{
    QString tmpstr = "alsa_output.pci-0000_00_1f.3.analog-stereo.monitor";
    QString outpath = "/home/zhangteng";
    m_GstreamRecorder->startRecord();
}

TEST_F(ut_gstreamrecorder_test, pauseRecord)
{
    m_GstreamRecorder->createPipe();
    m_GstreamRecorder->pauseRecord();
}

TEST_F(ut_gstreamrecorder_test, stopRecord)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.stopRecord();
}

TEST_F(ut_gstreamrecorder_test, setDevice)
{
    QString tmpstr = "alsa_output.pci-0000_00_1f.3.analog-stereo.monitortest";
    m_GstreamRecorder->setDevice(tmpstr);
}

TEST_F(ut_gstreamrecorder_test, objectUnref)
{
//    GstreamRecorder gstreamrecorder;
}

TEST_F(ut_gstreamrecorder_test, setStateToNull)
{
    m_GstreamRecorder->setStateToNull();
}

TEST_F(ut_gstreamrecorder_test, initFormat)
{
    m_GstreamRecorder->initFormat();
}

TEST_F(ut_gstreamrecorder_test, doBusMessage)
{
//    GstreamRecorder gstreamrecorder;
}

TEST_F(ut_gstreamrecorder_test, doBufferProbe)
{
//    GstreamRecorder gstreamrecorder;
}
