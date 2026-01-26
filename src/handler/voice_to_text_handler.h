// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VOICETOTEXTHANDLER_H
#define VOICETOTEXTHANDLER_H

#include <QObject>
#include <QSharedPointer>

class VNVoiceBlock;
class VNoteA2TManager;

class VoiceToTextHandler : public QObject
{
    Q_OBJECT
public:
    explicit VoiceToTextHandler(QObject *parent = nullptr);

    void setAudioToText(const QSharedPointer<VNVoiceBlock> &voiceBlock);

    Q_SIGNAL void audioLengthLimit();
    Q_SIGNAL void noNetworkConnection();

private:
    void onA2TStart();
    bool checkNetworkState();
    Q_SLOT void onA2TError(int error);
    Q_SLOT void onA2TSuccess(const QString &text);

private:
    VNoteA2TManager *m_a2tManager { nullptr };
    QSharedPointer<VNVoiceBlock> m_voiceBlock;

    // 记录发起转换请求时的上下文，用于切换笔记后仍能正确处理结果
    int m_originalNoteId {-1};     // 原始笔记 ID
    QString m_originalVoicePath;   // 语音文件路径
};

#endif  // VOICETOTEXTHANDLER_H
