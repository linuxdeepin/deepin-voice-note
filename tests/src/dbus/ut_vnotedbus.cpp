// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VoiceNoteDBusService.

#include "VoiceNoteDBusService.h"
#include <gtest/gtest.h>
#include <QWindow>

TEST(VoiceNoteDBusServiceUT, initAndActivate)
{
    VoiceNoteDBusService svc;
    // Registration may succeed or fail (name possibly held by another process);
    // either way the function body executes.
    svc.initDBusService();
    svc.ActivateWindow();   // no top-level windows in test -> logs + returns
    SUCCEED();
}

TEST(VoiceNoteDBusServiceUT, getNotesList)
{
    VoiceNoteDBusService svc;
    QString json = svc.GetNotesList();   // uses pre-seeded VNoteDataManager
    EXPECT_FALSE(json.isEmpty());
}

TEST(VoiceNoteDBusServiceUT, recordVoiceNotFound)
{
    VoiceNoteDBusService svc;
    // folder 999 absent -> early "Folder not found" return (no recording side effect)
    EXPECT_FALSE(svc.RecordVoice(999, 999));
}

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================

// BUG 364253: 最小化后从启动器再次打开窗口丢失最大化状态/无法打开
// （2148616a 新增 trackMainWindowState：最小化时不更新记录，恢复时依据
//  m_wasMaximized 选择 showMaximized/showNormal）。
// PMS: https://pms.uniontech.com/bug-view-364253.html  commit: 2148616a
TEST(VoiceNoteDBusServiceUT, BUG364253_minimizedStatePreservedAndRestored)
{
    VoiceNoteDBusService svc;
    QWindow w;
    w.show();   // offscreen 平台下创建顶层窗口
    svc.trackMainWindowState();
    ASSERT_FALSE(svc.m_wasMaximized);   // 初始为普通态

    // 最小化事件不更新记录
    emit w.windowStateChanged(Qt::WindowMinimized);
    EXPECT_FALSE(svc.m_wasMaximized);

    // 最大化状态被记录
    emit w.windowStateChanged(Qt::WindowMaximized);
    EXPECT_TRUE(svc.m_wasMaximized);

    // 再次最小化：保留最大化记录，供激活时恢复
    emit w.windowStateChanged(Qt::WindowMinimized);
    EXPECT_TRUE(svc.m_wasMaximized);

    // 激活路径：依据记录恢复最大化（offscreen 下仅验证走通不崩溃）
    svc.ActivateWindow();
    SUCCEED();
}

// 无顶层窗口时 ActivateWindow 安全返回
// PMS: https://pms.uniontech.com/bug-view-364253.html  commit: 2148616a
TEST(VoiceNoteDBusServiceUT, BUG364253_activateWindowNoWindowSafe)
{
    VoiceNoteDBusService svc;
    svc.m_wasMaximized = true;   // 无窗口也不得解引用
    svc.ActivateWindow();
    SUCCEED();
}
