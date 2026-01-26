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

void VoiceToTextTaskManager::addTask(int noteId, const QString &voicePath)
{
    if (voicePath.isEmpty()) {
        qWarning() << "Cannot add task with empty voice path";
        return;
    }

    VoiceToTextTask task;
    task.noteId = noteId;
    task.voicePath = voicePath;
    task.status = VoiceToTextTask::Converting;
    task.startTime = QDateTime::currentDateTime();

    m_tasks[voicePath] = task;

    qInfo() << "Added voice-to-text task:" << voicePath << "for note:" << noteId;
    emit taskStatusChanged(voicePath, VoiceToTextTask::Converting);
}

VoiceToTextTask* VoiceToTextTaskManager::getTask(const QString &voicePath)
{
    if (m_tasks.contains(voicePath)) {
        return &m_tasks[voicePath];
    }
    return nullptr;
}

void VoiceToTextTaskManager::setTaskResult(const QString &voicePath, const QString &text, bool success)
{
    if (!m_tasks.contains(voicePath)) {
        qWarning() << "Task not found for voice path:" << voicePath;
        return;
    }

    VoiceToTextTask &task = m_tasks[voicePath];
    task.status = success ? VoiceToTextTask::Completed : VoiceToTextTask::Failed;
    task.resultText = text;

    qInfo() << "Task result set for:" << voicePath 
            << "success:" << success 
            << "text length:" << text.length();

    emit taskStatusChanged(voicePath, task.status);
    emit taskCompleted(task.noteId, voicePath, text, success);
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

void VoiceToTextTaskManager::removeTask(const QString &voicePath)
{
    if (m_tasks.remove(voicePath) > 0) {
        qDebug() << "Removed task for voice path:" << voicePath;
    }
}
