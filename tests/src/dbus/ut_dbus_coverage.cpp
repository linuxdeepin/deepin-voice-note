// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Coverage tests for previously-uncovered functions in VoiceNoteDBusService:
//   ~VoiceNoteDBusService (D0), trackMainWindowState() lambda.
// Build enables -fno-access-control, so private trackMainWindowState and the
// m_wasMaximized member are accessible.

#include "VoiceNoteDBusService.h"

#include <gtest/gtest.h>
#include <QWindow>
#include <QGuiApplication>

// ===========================================================================
// VoiceNoteDBusService::~VoiceNoteDBusService (D0 deleting destructor) — L28
// ===========================================================================
TEST(VoiceNoteDBusServiceCoverage, Destructor_D0)
{
    VoiceNoteDBusService *svc = new VoiceNoteDBusService();
    ASSERT_NE(nullptr, svc);
    delete svc;   // D0
    SUCCEED();
}

// ===========================================================================
// trackMainWindowState()::{lambda#1}::operator() — VoiceNoteDBusService.cpp:68
//
// The lambda is connected to QWindow::windowStateChanged inside
// trackMainWindowState(). We create a top-level QWindow so
// qApp->topLevelWindows() is non-empty, call trackMainWindowState, then
// change the window state to fire the signal → lambda body.
// ===========================================================================
TEST(VoiceNoteDBusServiceCoverage, trackMainWindowState_Lambda)
{
    VoiceNoteDBusService svc;

    // Create and show a top-level window so trackMainWindowState finds one.
    QWindow window;
    window.show();

    // trackMainWindowState is private; -fno-access-control grants access.
    svc.trackMainWindowState();

    // Initial m_wasMaximized reflects the window's initial state.
    bool initialFlag = svc.m_wasMaximized;
    (void)initialFlag;   // suppress unused warning

    // Change state to Maximized → lambda should set m_wasMaximized = true.
    window.setWindowState(Qt::WindowMaximized);
    qApp->processEvents();
    EXPECT_TRUE(svc.m_wasMaximized);

    // Change state to NoState (not Minimized) → lambda sets false.
    window.setWindowState(Qt::WindowNoState);
    qApp->processEvents();
    EXPECT_FALSE(svc.m_wasMaximized);

    // Record current value, then send Minimized.
    // Per lambda logic, Minimized must NOT update m_wasMaximized.
    bool beforeMin = svc.m_wasMaximized;
    window.setWindowState(Qt::WindowMinimized);
    qApp->processEvents();
    EXPECT_EQ(beforeMin, svc.m_wasMaximized);

    window.hide();
}

// ===========================================================================
// trackMainWindowState with no top-level windows → early return (no crash)
// ===========================================================================
TEST(VoiceNoteDBusServiceCoverage, trackMainWindowState_NoWindows)
{
    // Remove any previously created windows from the prior test
    const auto wins = qApp->topLevelWindows();
    for (auto *w : wins) {
        w->hide();
    }
    qApp->processEvents();

    VoiceNoteDBusService svc;
    // Should early-return without crashing because topLevelWindows is empty
    // (or the remaining hidden windows don't interfere).
    svc.trackMainWindowState();
    SUCCEED();
}
