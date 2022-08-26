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

UT_GstreamRecorder::UT_GstreamRecorder()
{
}
/*
TEST_F(UT_GstreamRecorder, UT_GstreamRecorder_GetGstState_001)
{
    int state = -1;
    int pending = -1;
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.createPipe();
    gstreamrecorder.GetGstState(&state, &pending);
    EXPECT_NE(-1, state) << "state";
    EXPECT_NE(-1, pending) << "pending";
}

TEST_F(UT_GstreamRecorder, UT_GstreamRecorder_pauseRecord_001)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.createPipe();
    gstreamrecorder.pauseRecord();
}

TEST_F(UT_GstreamRecorder, UT_GstreamRecorder_stopRecord_001)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.createPipe();
    gstreamrecorder.stopRecord();
}

TEST_F(UT_GstreamRecorder, UT_GstreamRecorder_setDevice_001)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.createPipe();
    gstreamrecorder.m_currentDevice = "123";
    gstreamrecorder.setDevice(QString("test"));
    EXPECT_EQ("test", gstreamrecorder.m_currentDevice);
}

TEST_F(UT_GstreamRecorder, UT_GstreamRecorder_setStateToNull_001)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.createPipe();
    gstreamrecorder.setStateToNull();
}

TEST_F(UT_GstreamRecorder, UT_GstreamRecorder_initFormat)
{
    GstreamRecorder gstreamrecorder;
    gstreamrecorder.initFormat();
    EXPECT_EQ(44100, gstreamrecorder.m_format.sampleRate()) << "code";
    EXPECT_EQ(QAudioFormat::LittleEndian, gstreamrecorder.m_format.byteOrder()) << "byteOrder";
    EXPECT_EQ(QAudioFormat::SignedInt, gstreamrecorder.m_format.sampleType()) << "sampleType";
    EXPECT_EQ(16, gstreamrecorder.m_format.sampleSize()) << "sampleSize";
}
*/
