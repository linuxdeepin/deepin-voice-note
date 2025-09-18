#include "voiceplayerbase.h"
#include <QDebug>

VoicePlayerBase::VoicePlayerBase(QObject *parent)
{
    qInfo() << "VoicePlayerBase constructor called";
}

void VoicePlayerBase::setChangePlayFile(bool flag)
{
    qInfo() << "Setting change play file flag to:" << flag;
    Q_UNUSED(flag)
}
