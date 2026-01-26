// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VOICETOTEXTTASKMANAGER_H
#define VOICETOTEXTTASKMANAGER_H

#include <QObject>
#include <QMap>
#include <QDateTime>

/**
 * @brief 语音转文字任务状态
 */
struct VoiceToTextTask {
    enum Status {
        Pending,      // 等待转换
        Converting,   // 转换中
        Completed,    // 已完成
        Failed        // 失败
    };

    int noteId {-1};           // 笔记 ID
    QString voicePath;         // 语音文件路径（唯一标识）
    Status status {Pending};   // 转换状态
    QString resultText;        // 转换结果
    QDateTime startTime;       // 开始时间
};

/**
 * @brief 语音转文字任务管理器
 * 
 * 管理多个语音转文字任务，支持：
 * 1. 跟踪每个语音的转换状态
 * 2. 在笔记切换后仍能正确处理转换结果
 * 3. 查询特定笔记的转换任务
 */
class VoiceToTextTaskManager : public QObject
{
    Q_OBJECT
public:
    static VoiceToTextTaskManager* instance();

    /**
     * @brief 添加转换任务
     * @param noteId 发起转换的笔记 ID
     * @param voicePath 语音文件路径
     */
    void addTask(int noteId, const QString &voicePath);

    /**
     * @brief 获取任务
     * @param voicePath 语音文件路径
     * @return 任务指针，不存在返回 nullptr
     */
    VoiceToTextTask* getTask(const QString &voicePath);

    /**
     * @brief 设置任务结果
     * @param voicePath 语音文件路径
     * @param text 转换结果文本
     * @param success 是否成功
     */
    void setTaskResult(const QString &voicePath, const QString &text, bool success);

    /**
     * @brief 获取笔记的所有进行中任务
     * @param noteId 笔记 ID
     * @return 任务列表
     */
    QList<VoiceToTextTask> getTasksForNote(int noteId) const;

    /**
     * @brief 是否有任务正在转换
     */
    bool hasActiveTask() const;

    /**
     * @brief 移除已完成的任务（可选，用于清理）
     * @param voicePath 语音文件路径
     */
    void removeTask(const QString &voicePath);

signals:
    /**
     * @brief 任务状态变化信号
     */
    void taskStatusChanged(const QString &voicePath, VoiceToTextTask::Status status);

    /**
     * @brief 任务完成信号（用于通知后续处理）
     * @param noteId 笔记 ID
     * @param voicePath 语音文件路径
     * @param text 转换结果
     * @param success 是否成功
     */
    void taskCompleted(int noteId, const QString &voicePath, const QString &text, bool success);

private:
    explicit VoiceToTextTaskManager(QObject *parent = nullptr);

    QMap<QString, VoiceToTextTask> m_tasks;  // voicePath -> Task
};

#endif  // VOICETOTEXTTASKMANAGER_H
