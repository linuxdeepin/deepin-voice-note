/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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

bool OpsStateInterface::isAiSrvExist() const
{
    return (m_states & (1<<StateAISrvAvailable));
}

OpsStateInterface *gVNoteOpsStates()
{
    return reinterpret_cast<VNoteApplication*>(qApp)->mainWindow();
}
