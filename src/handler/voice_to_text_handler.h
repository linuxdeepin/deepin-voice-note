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

private:
    void onA2TStart();
    Q_SLOT void onA2TError(int error);
    Q_SLOT void onA2TSuccess(const QString &text);

private:
    VNoteA2TManager *m_a2tManager { nullptr };
    QSharedPointer<VNVoiceBlock> m_voiceBlock;
};

#endif  // VOICETOTEXTHANDLER_H
