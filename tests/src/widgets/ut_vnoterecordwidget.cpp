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
#include "ut_vnoterecordwidget.h"
#include "vnoterecordwidget.h"
#include "utils.h"
#include "stub.h"

static bool stub_startRecord()
{
    return true;
}

ut_vnoterecordwidget_test::ut_vnoterecordwidget_test()
{
}

void ut_vnoterecordwidget_test::SetUp()
{
    m_vnoterecordwidget = new VNoteRecordWidget;
}

void ut_vnoterecordwidget_test::TearDown()
{
    delete m_vnoterecordwidget;
}

TEST_F(ut_vnoterecordwidget_test, startRecord)
{
    QAudioFormat audioformat;
    const QByteArray data;
    audioformat.setCodec("audio/pcm");
    //通道，采样率
    audioformat.setChannelCount(2);
    audioformat.setSampleRate(44100);
    //lamemp3enc 编码器插件格式为S16LE
    audioformat.setByteOrder(QAudioFormat::LittleEndian);
    audioformat.setSampleType(QAudioFormat::UnSignedInt);
    audioformat.setSampleSize(16);
    QAudioBuffer buffer(data, audioformat);
    Stub stub;
    stub.set(ADDR(GstreamRecorder, startRecord), stub_startRecord);
    m_vnoterecordwidget->onAudioBufferProbed(buffer);
    m_vnoterecordwidget->startRecord();
    m_vnoterecordwidget->onRecordDurationChange(4);
    m_vnoterecordwidget->stopRecord();
}

TEST_F(ut_vnoterecordwidget_test, getRecordPath)
{
    m_vnoterecordwidget->getRecordPath();
}

TEST_F(ut_vnoterecordwidget_test, setAudioDevice)
{
    m_vnoterecordwidget->setAudioDevice("test");
}
