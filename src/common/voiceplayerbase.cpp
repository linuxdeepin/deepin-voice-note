#include "voiceplayerbase.h"
#include "ddlog.h"
#include <QDebug>

VoicePlayerBase::VoicePlayerBase(QObject *parent)
{
    qCInfo(voiceNoteLog) << "VoicePlayerBase constructor called";
}

void VoicePlayerBase::setChangePlayFile(bool flag)
{
    qCInfo(voiceNoteLog) << "Setting change play file flag to:" << flag;
    Q_UNUSED(flag)
}
