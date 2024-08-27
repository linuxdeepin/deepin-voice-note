// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VOICEPLAYERHANDLER_H
#define VOICEPLAYERHANDLER_H

#include <QObject>
#include <QSharedPointer>

class VNVoiceBlock;
class VlcPlayer;

class VoicePlayerHandler : public QObject
{
    Q_OBJECT
public:
    enum PlayState {
        Playing = 0,
        Paused = 1,
        End = 2,
    };

    explicit VoicePlayerHandler(QObject *parent = nullptr);

    Q_SLOT void playVoice(const QVariant &json, bool bIsSame);

    Q_SLOT void onPlay();
    Q_SLOT void onStop();
    Q_SLOT void onToggleStateChange();
    Q_SLOT void setPlayPosition(qint64 ms);

    Q_SIGNAL void voiceFileError();
    Q_SIGNAL void playStatusChanged(PlayState state);
    Q_SIGNAL void playDurationChanged(qint64 duration);
    Q_SIGNAL void playPositionChanged(qint64 ms);

private:
    void playVoiceImpl(bool bIsSame);

private:
    VlcPlayer *m_player { nullptr };
    QSharedPointer<VNVoiceBlock> m_voiceBlock;
};

#endif  // VOICEPLAYERHANDLER_H
