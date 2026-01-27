// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice_to_text_task_manager.h"

#include <QDebug>

VoiceToTextTaskManager::VoiceToTextTaskManager(QObject *parent)
    : QObject(parent)
{
}

VoiceToTextTaskManager* VoiceToTextTaskManager::instance()
{
    static VoiceToTextTaskManager instance;
    return &instance;
}

void VoiceToTextTaskManager::addTask(int noteId, const QString &voiceId)
{
    if (voiceId.isEmpty()) {
        qWarning() << "Cannot add task with empty voiceId";
        return;
    }

    VoiceToTextTask task;
    task.noteId = noteId;
    task.voiceId = voiceId;
    task.status = VoiceToTextTask::Converting;
    task.startTime = QDateTime::currentDateTime();

    m_tasks[voiceId] = task;

    qInfo() << "Added voice-to-text task, voiceId:" << voiceId << "noteId:" << noteId;
    emit taskStatusChanged(voiceId, VoiceToTextTask::Converting);
}

VoiceToTextTask* VoiceToTextTaskManager::getTask(const QString &voiceId)
{
    if (m_tasks.contains(voiceId)) {
        return &m_tasks[voiceId];
    }
    return nullptr;
}

void VoiceToTextTaskManager::setTaskResult(const QString &voiceId, const QString &text, bool success)
{
    if (!m_tasks.contains(voiceId)) {
        qWarning() << "Task not found for voiceId:" << voiceId;
        return;
    }

    VoiceToTextTask &task = m_tasks[voiceId];
    task.status = success ? VoiceToTextTask::Completed : VoiceToTextTask::Failed;
    task.resultText = text;

    qInfo() << "Task result set, voiceId:" << voiceId 
            << "success:" << success 
            << "text length:" << text.length();

    emit taskStatusChanged(voiceId, task.status);
    emit taskCompleted(task.noteId, voiceId, text, success);
}

QList<VoiceToTextTask> VoiceToTextTaskManager::getTasksForNote(int noteId) const
{
    QList<VoiceToTextTask> result;
    for (const auto &task : m_tasks) {
        if (task.noteId == noteId) {
            result.append(task);
        }
    }
    return result;
}

bool VoiceToTextTaskManager::hasActiveTask() const
{
    for (const auto &task : m_tasks) {
        if (task.status == VoiceToTextTask::Converting) {
            return true;
        }
    }
    return false;
}

void VoiceToTextTaskManager::removeTask(const QString &voiceId)
{
    if (m_tasks.remove(voiceId) > 0) {
        qDebug() << "Removed task for voiceId:" << voiceId;
    }
}
