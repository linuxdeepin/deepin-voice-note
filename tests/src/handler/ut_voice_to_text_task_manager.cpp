// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_voice_to_text_task_manager.h"
#include "voice_to_text_task_manager.h"

#include <QSignalSpy>

UT_VoiceToTextTaskManager::UT_VoiceToTextTaskManager()
{
}

void UT_VoiceToTextTaskManager::SetUp()
{
}

void UT_VoiceToTextTaskManager::TearDown()
{
    // 使用 clearAllTasks 统一重置单例状态，确保用例间完全隔离
    VoiceToTextTaskManager::instance()->clearAllTasks();
}

// ---------------------------------------------------------------------------
// addTask / getTask
// ---------------------------------------------------------------------------

TEST_F(UT_VoiceToTextTaskManager, AddTask_ValidVoiceId_AddsTask)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    QSignalSpy spy(mgr, &VoiceToTextTaskManager::taskStatusChanged);

    mgr->addTask(1, QStringLiteral("voice-add-001"));
    VoiceToTextTask *task = mgr->getTask(QStringLiteral("voice-add-001"));

    ASSERT_NE(nullptr, task);
    EXPECT_EQ(1, task->noteId);
    EXPECT_EQ(VoiceToTextTask::Converting, task->status);
    EXPECT_EQ(QStringLiteral("voice-add-001"), task->voiceId);

    EXPECT_EQ(1, spy.count());
}

TEST_F(UT_VoiceToTextTaskManager, AddTask_EmptyVoiceId_DoesNotAdd)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    QSignalSpy spy(mgr, &VoiceToTextTaskManager::taskStatusChanged);

    mgr->addTask(2, QString());
    EXPECT_EQ(nullptr, mgr->getTask(QString()));
    EXPECT_EQ(0, spy.count());
}

TEST_F(UT_VoiceToTextTaskManager, GetTask_NotExist_ReturnsNull)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    EXPECT_EQ(nullptr, mgr->getTask(QStringLiteral("nonexistent-uuid-999")));
}

// ---------------------------------------------------------------------------
// setTaskResult
// ---------------------------------------------------------------------------

TEST_F(UT_VoiceToTextTaskManager, SetTaskResult_Success_UpdatesStatus)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    mgr->addTask(3, QStringLiteral("voice-result-001"));

    QSignalSpy statusSpy(mgr, &VoiceToTextTaskManager::taskStatusChanged);
    QSignalSpy completedSpy(mgr, &VoiceToTextTaskManager::taskCompleted);

    mgr->setTaskResult(QStringLiteral("voice-result-001"), QStringLiteral("转写结果"), true);

    VoiceToTextTask *task = mgr->getTask(QStringLiteral("voice-result-001"));
    ASSERT_NE(nullptr, task);
    EXPECT_EQ(VoiceToTextTask::Completed, task->status);
    EXPECT_EQ(QStringLiteral("转写结果"), task->resultText);

    EXPECT_EQ(1, statusSpy.count());
    EXPECT_EQ(1, completedSpy.count());

    QList<QVariant> args = completedSpy.takeFirst();
    EXPECT_EQ(3, args.at(0).toInt());
    EXPECT_EQ(QStringLiteral("voice-result-001"), args.at(1).toString());
    EXPECT_EQ(QStringLiteral("转写结果"), args.at(2).toString());
    EXPECT_TRUE(args.at(3).toBool());
}

TEST_F(UT_VoiceToTextTaskManager, SetTaskResult_Failure_UpdatesStatus)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    mgr->addTask(4, QStringLiteral("voice-result-002"));

    QSignalSpy statusSpy(mgr, &VoiceToTextTaskManager::taskStatusChanged);
    QSignalSpy completedSpy(mgr, &VoiceToTextTaskManager::taskCompleted);

    mgr->setTaskResult(QStringLiteral("voice-result-002"), QString(), false);

    VoiceToTextTask *task = mgr->getTask(QStringLiteral("voice-result-002"));
    ASSERT_NE(nullptr, task);
    EXPECT_EQ(VoiceToTextTask::Failed, task->status);
    EXPECT_EQ(1, statusSpy.count());
    EXPECT_EQ(1, completedSpy.count());
}

TEST_F(UT_VoiceToTextTaskManager, SetTaskResult_NotExist_NoSignal)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    QSignalSpy statusSpy(mgr, &VoiceToTextTaskManager::taskStatusChanged);
    QSignalSpy completedSpy(mgr, &VoiceToTextTaskManager::taskCompleted);

    mgr->setTaskResult(QStringLiteral("nonexistent-uuid-998"), QString(), false);

    EXPECT_EQ(0, statusSpy.count());
    EXPECT_EQ(0, completedSpy.count());
}

// ---------------------------------------------------------------------------
// getTasksForNote / hasActiveTask
// ---------------------------------------------------------------------------

TEST_F(UT_VoiceToTextTaskManager, GetTasksForNote_ReturnsMatchingTasks)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    mgr->addTask(10, QStringLiteral("voice-note-a"));
    mgr->addTask(10, QStringLiteral("voice-note-b"));
    mgr->addTask(20, QStringLiteral("voice-note-c"));

    QList<VoiceToTextTask> tasks = mgr->getTasksForNote(10);
    EXPECT_EQ(2, tasks.size());
}

TEST_F(UT_VoiceToTextTaskManager, HasActiveTask_WithConvertingTask_ReturnsTrue)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    mgr->addTask(11, QStringLiteral("voice-active-001"));
    EXPECT_TRUE(mgr->hasActiveTask());
}

TEST_F(UT_VoiceToTextTaskManager, HasActiveTask_WithNoConvertingTask_ReturnsFalse)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    mgr->addTask(12, QStringLiteral("voice-active-002"));
    ASSERT_TRUE(mgr->hasActiveTask());

    mgr->setTaskResult(QStringLiteral("voice-active-002"), QStringLiteral("done"), true);
    EXPECT_FALSE(mgr->hasActiveTask());
}

// ---------------------------------------------------------------------------
// removeTask
// ---------------------------------------------------------------------------

TEST_F(UT_VoiceToTextTaskManager, RemoveTask_Existing_RemovesIt)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    mgr->addTask(13, QStringLiteral("voice-remove-001"));
    ASSERT_NE(nullptr, mgr->getTask(QStringLiteral("voice-remove-001")));

    mgr->removeTask(QStringLiteral("voice-remove-001"));
    EXPECT_EQ(nullptr, mgr->getTask(QStringLiteral("voice-remove-001")));
}

TEST_F(UT_VoiceToTextTaskManager, RemoveTask_NonExistent_NoCrash)
{
    VoiceToTextTaskManager *mgr = VoiceToTextTaskManager::instance();
    mgr->removeTask(QStringLiteral("nonexistent-uuid-997"));
    SUCCEED();
}
