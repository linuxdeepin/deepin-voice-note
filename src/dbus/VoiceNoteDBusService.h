// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VOICENOTEDBUSSERVICE_H
#define VOICENOTEDBUSSERVICE_H

#include <QObject>
#include <QString>

class VoiceNoteDBusService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.voicenote")

public:
    explicit VoiceNoteDBusService(QObject *parent = nullptr);
    ~VoiceNoteDBusService();

    bool initDBusService();

private:
    // 跟踪窗口最大化状态（DDE 最小化时会丢失 Maximized 标志）
    bool m_wasMaximized = false;

    void trackMainWindowState();

public slots:
    // D-Bus接口1: 获取笔记列表
    QString GetNotesList();

    // D-Bus接口2: 录音
    bool RecordVoice(int folderId, int noteId);

    // D-Bus接口3: 激活窗口（从最小化恢复并显示到前台）
    void ActivateWindow();
};

#endif // VOICENOTEDBUSSERVICE_H

