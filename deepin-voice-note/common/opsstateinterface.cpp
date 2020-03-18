#include "opsstateinterface.h"
#include "vnoteapplication.h"

#include <DLog>

OpsStateInterface::OpsStateInterface()
{

}

void OpsStateInterface::operState(int type, bool isSet)
{
    quint8 shift = static_cast<quint8>(type);

    if (shift > StateNone && shift < StateMax) {
        if (isSet) {
            m_states |= (1<<shift);
        } else {
            m_states &= (~(1<<shift));
        }
    } else {
        qCritical() << "Operation error:Invalid opsType =" << type;
    }
}

bool OpsStateInterface::isSearching() const
{
    return (m_states & (1<<StateSearching));
}

bool OpsStateInterface::isRecording() const
{
    return (m_states & (1<<StateRecording));
}

bool OpsStateInterface::isPlaying() const
{
    return (m_states & (1<<StatePlaying));
}

bool OpsStateInterface::isVoice2Text() const
{
    return (m_states & (1<<StateVoice2Text));
}

bool OpsStateInterface::isAppQuit() const
{
    return (m_states & (1<<StateAppQuit));
}

OpsStateInterface *gVNoteOpsStates()
{
    return reinterpret_cast<VNoteApplication*>(qApp)->mainWindow();
}
