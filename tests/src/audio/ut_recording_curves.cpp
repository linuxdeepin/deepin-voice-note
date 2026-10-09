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

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================

// BUG 277547: 录音动画除体现声音大小外需向右流动（91fca315 重写 paint，
// 引入 m_phase 相位推进：sin((x - m_phase)...) 使波形随时间右移）。
// PMS: https://pms.uniontech.com/bug-view-277547.html  commit: 91fca315
TEST(RecordingCurvesFlowUT, BUG277547_updateCurvesAdvancesPhase)
{
    RecordingCurves curves;
    curves.startRecording();
    const double p0 = curves.m_phase;
    curves.updateCurves();
    curves.updateCurves();
    // 相位每次 updateCurves 推进 π，驱动波形向右流动
    EXPECT_DOUBLE_EQ(p0 + 2 * M_PI, curves.m_phase);

    // 超过 170π 后回卷，避免长时间录制相位无界增长
    curves.m_phase = 170 * M_PI;
    curves.updateCurves();
    EXPECT_DOUBLE_EQ(0.0, curves.m_phase);
    curves.stopRecording();
}

// 相位差应导致渲染结果不同（波形确实移动了，而非静态曲线）
// PMS: https://pms.uniontech.com/bug-view-277547.html  commit: 91fca315
TEST(RecordingCurvesFlowUT, BUG277547_paintFlowShiftsWaveform)
{
    RecordingCurves c1, c2;
    c1.setSize(QSizeF(100, 50));
    c2.setSize(QSizeF(100, 50));
    c1.updateVolume(1.0);
    c2.updateVolume(1.0);
    c2.m_phase = M_PI;   // 半周期相位差

    QImage img1(200, 100, QImage::Format_ARGB32);
    QImage img2(200, 100, QImage::Format_ARGB32);
    img1.fill(Qt::transparent);
    img2.fill(Qt::transparent);
    QPainter p1(&img1);
    QPainter p2(&img2);
    c1.paint(&p1);
    c2.paint(&p2);
    p1.end();
    p2.end();

    EXPECT_FALSE(img1.isNull());
    EXPECT_FALSE(img2.isNull());
    EXPECT_NE(img1, img2);   // 不同相位 → 波形右移后的画面不同
}
