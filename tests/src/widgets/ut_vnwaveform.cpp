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

#include "ut_vnwaveform.h"
#include "vnwaveform.h"

UT_VNWaveform::UT_VNWaveform()
{
}

TEST_F(UT_VNWaveform, UT_VNWaveform_onAudioBufferProbed_001)
{
    VNWaveform vnwaveform;
    QAudioBuffer buffer;
    buffer.format().setSampleSize(16);
    vnwaveform.onAudioBufferProbed(buffer);
    EXPECT_TRUE(vnwaveform.m_audioScaleSamples.isEmpty());
}

TEST_F(UT_VNWaveform, UT_VNWaveform_paintEvent_001)
{
    VNWaveform vnwaveform;
    vnwaveform.m_audioScaleSamples.push_back(1.1);
    vnwaveform.m_audioScaleSamples.push_back(1.2);
    vnwaveform.m_audioScaleSamples.push_back(1.3);
    vnwaveform.m_audioScaleSamples.push_back(1.4);
    vnwaveform.m_maxShowedSamples = 4;
    EXPECT_FALSE(vnwaveform.grab().isNull());
}

TEST_F(UT_VNWaveform, UT_VNWaveform_resizeEvent_001)
{
    VNWaveform vnwaveform;
    QSize size1(10, 12);
    QSize size2(5, 6);
    QResizeEvent *event = new QResizeEvent(size1, size2);
    int maxShowedSamples = vnwaveform.m_maxShowedSamples;
    vnwaveform.resizeEvent(event);
    EXPECT_TRUE(maxShowedSamples != vnwaveform.m_maxShowedSamples);

    delete event;
}

TEST_F(UT_VNWaveform, UT_VNWaveform_getBufferLevels_001)
{
    VNWaveform vnwaveform;
    QAudioFormat audioformat;
    QVector<qreal> scaleSamples;
    const QByteArray data;
    qreal frameGain = 0;
    audioformat.setCodec("audio/pcm");
    //通道，采样率
    audioformat.setChannelCount(2);
    audioformat.setSampleRate(44100);
    //lamemp3enc 编码器插件格式为S16LE
    audioformat.setByteOrder(QAudioFormat::LittleEndian);
    audioformat.setSampleType(QAudioFormat::UnSignedInt);
    audioformat.setSampleSize(16);
    QAudioBuffer buffer(data, audioformat);
    vnwaveform.getBufferLevels(buffer, scaleSamples, frameGain);

    audioformat.setSampleSize(32);
    QAudioBuffer buffer1(6, audioformat);
    vnwaveform.getBufferLevels(buffer1, scaleSamples, frameGain);

    audioformat.setSampleSize(8);
    QAudioBuffer buffer2(6, audioformat);
    vnwaveform.getBufferLevels(buffer2, scaleSamples, frameGain);

    audioformat.setSampleType(QAudioFormat::Float);
    audioformat.setSampleSize(32);
    QAudioBuffer buffer3(6, audioformat);
    vnwaveform.getBufferLevels(buffer3, scaleSamples, frameGain);

    audioformat.setSampleType(QAudioFormat::SignedInt);
    audioformat.setSampleSize(32);
    QAudioBuffer buffer4(6, audioformat);
    vnwaveform.getBufferLevels(buffer4, scaleSamples, frameGain);

    audioformat.setSampleSize(16);
    QAudioBuffer buffer5(6, audioformat);
    vnwaveform.getBufferLevels(buffer5, scaleSamples, frameGain);

    audioformat.setSampleSize(8);
    QAudioBuffer buffer6(6, audioformat);
    vnwaveform.getBufferLevels(buffer6, scaleSamples, frameGain);
}

TEST_F(UT_VNWaveform, UT_VNWaveform_getPeakValue_001)
{
    VNWaveform vnwaveform;
    QAudioFormat audioformat;
    audioformat.setCodec("audio/pcm");
    //通道，采样率
    audioformat.setChannelCount(2);
    audioformat.setSampleRate(44100);
    //lamemp3enc 编码器插件格式为S16LE
    audioformat.setByteOrder(QAudioFormat::LittleEndian);
    audioformat.setSampleType(QAudioFormat::SignedInt);
    audioformat.setSampleSize(16);
    EXPECT_EQ(SHRT_MAX, vnwaveform.getPeakValue(audioformat));
    audioformat.setSampleSize(8);
    EXPECT_EQ(CHAR_MAX, vnwaveform.getPeakValue(audioformat));
}
