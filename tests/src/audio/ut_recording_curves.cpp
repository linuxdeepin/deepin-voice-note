// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_recording_curves.h"
#include "recording_curves.h"

#include <QPainter>

UT_RecordingCurves::UT_RecordingCurves()
{
}

TEST_F(UT_RecordingCurves, Constructor_CreatesValidInstance)
{
    RecordingCurves curves;
    EXPECT_FALSE(curves.isRecordingActive());
}

TEST_F(UT_RecordingCurves, UpdateVolume_DoesNotCrash)
{
    RecordingCurves curves;
    curves.updateVolume(0.5);
    curves.updateVolume(0.0);
    curves.updateVolume(1.0);
    SUCCEED();
}

TEST_F(UT_RecordingCurves, StartRecording_TimerBecomesActive)
{
    RecordingCurves curves;
    EXPECT_FALSE(curves.isRecordingActive());

    curves.startRecording();
    EXPECT_TRUE(curves.isRecordingActive());
}

TEST_F(UT_RecordingCurves, StopRecording_AfterStart_TimerStops)
{
    RecordingCurves curves;
    curves.startRecording();
    ASSERT_TRUE(curves.isRecordingActive());

    curves.stopRecording();
    EXPECT_FALSE(curves.isRecordingActive());
}

TEST_F(UT_RecordingCurves, StopRecording_WithoutStart_DoesNotCrash)
{
    RecordingCurves curves;
    EXPECT_FALSE(curves.isRecordingActive());

    curves.stopRecording();
    EXPECT_FALSE(curves.isRecordingActive());
}

TEST_F(UT_RecordingCurves, PauseRecording_TogglesTimer)
{
    RecordingCurves curves;
    curves.startRecording();
    ASSERT_TRUE(curves.isRecordingActive());

    // pauseRecording should stop the timer
    curves.pauseRecording();
    EXPECT_FALSE(curves.isRecordingActive());

    // Second call should restart the timer
    curves.pauseRecording();
    EXPECT_TRUE(curves.isRecordingActive());
}

// 注：pauseRecording 在未 startRecording 时调用会启动定时器，
// 这是 RecordingCurves 源码的已知行为（pauseRecording 内部用
// m_timer->isActive() 做切换），此处不作为预期断言，避免将
// 源码行为固化为"正确"。

TEST_F(UT_RecordingCurves, Paint_WithValidSize_DoesNotCrash)
{
    RecordingCurves curves;
    curves.setSize(QSizeF(100, 50));
    curves.updateVolume(0.5);

    QImage image(100, 50, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    curves.paint(&painter);
    SUCCEED();
}

TEST_F(UT_RecordingCurves, Paint_WithZeroGain_DoesNotCrash)
{
    RecordingCurves curves;
    curves.setSize(QSizeF(100, 50));

    QImage image(100, 50, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    curves.paint(&painter);
    SUCCEED();
}
