// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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

public slots:
    // D-Bus接口1: 获取笔记列表
    QString GetNotesList();

    // D-Bus接口2: 录音
    bool RecordVoice(int folderId, int noteId);
};

#endif // VOICENOTEDBUSSERVICE_H

