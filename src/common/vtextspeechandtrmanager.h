// Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VTEXTSPEECHANDTRMANAGER_H
#define VTEXTSPEECHANDTRMANAGER_H

#include <QObject>
#include <QDBusInterface>
#include <QSharedPointer>

class VTextSpeechAndTrManager : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Disable,
        Enable,

        NotInstalled,
        NoUserAgreement,    // whether the user agrees to the user agreement.
        NoInputDevice,
        NoOutputDevice,

        Success = Enable,
        Failed = Disable,
    };

    explicit VTextSpeechAndTrManager(QObject *parent = nullptr);
    static VTextSpeechAndTrManager *instance();
    
    void checkUosAiExists();
    
    // 检测是否在朗读文本
    bool isTextToSpeechInWorking();
    // 检测语音朗读开关是否打开
    bool getTextToSpeechEnable();
    // 检测语音听写开关是否打开
    bool getSpeechToTextEnable();
 
    // 语音朗读
    Status onTextToSpeech();
    // 停止语音朗读
    Status onStopTextToSpeech();
    // 语音听写
    Status onSpeechToText();
    
    // 获取错误信息
    QString errorString(Status status);
    
    // 获取当前状态
    Status status() const { return m_status; }
    
    Status checkValid();
    
    static Status copilotInstalled(const QSharedPointer<QDBusInterface> &copilot);
    static Status isCopilotEnabled(const QSharedPointer<QDBusInterface> &copilot);
    static Status launchCopilotChat(const QSharedPointer<QDBusInterface> &copilot);

private:
    bool m_isSpeeching{false};
    Status m_status{NotInstalled};
    QSharedPointer<QDBusInterface> m_copilot;
};

#endif // VTEXTSPEECHANDTRMANAGER_H
