// SPDX-FileCopyrightText: 2019-2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VTEXTSPEECHANDTRMANAGER_H
#define VTEXTSPEECHANDTRMANAGER_H

#include <QObject>
#include <QSharedPointer>

class QDBusInterface;

class VTextSpeechAndTrManager : public QObject
{
    Q_OBJECT

public:
    static VTextSpeechAndTrManager *instance();

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

    inline Status status() const { return m_status; }
    inline bool valid() const { return Enable == m_status; }
    void checkUosAiExists();

    // 语音朗读
    Status onTextToSpeech();
    // 停止语音朗读
    Status onStopTextToSpeech();
    // 语音听写
    Status onSpeechToText();
    // 文本翻译
    Status onTextTranslate();

    QString errorString(Status status);

private:
    explicit VTextSpeechAndTrManager(QObject *parent = nullptr);
    ~VTextSpeechAndTrManager() override = default;

    // 检测是否在朗读文本
    bool isTextToSpeechInWorking();
    // 检测语音朗读开关是否打开
    bool getTextToSpeechEnable();
    // 检测语音听写开关是否打开
    bool getSpeechToTextEnable();
    // 检测文本翻译开关是否打开
    bool getTransEnable();

    Status checkValid();

    static Status copilotInstalled(const QSharedPointer<QDBusInterface> &copilot);
    static Status isCopilotEnabled(const QSharedPointer<QDBusInterface> &copilot);
    static Status launchCopilotChat(const QSharedPointer<QDBusInterface> &copilot);

    bool m_isSpeeching{false};
    Status m_status{NotInstalled};
    QSharedPointer<QDBusInterface> m_copilot;
};

#endif  // VTEXTSPEECHANDTRMANAGER_H
