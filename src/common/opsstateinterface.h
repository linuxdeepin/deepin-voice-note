/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
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
#ifndef OPSSTATEINTERFACE_H
#define OPSSTATEINTERFACE_H

#include <QtGlobal>

class OpsStateInterface
{
public:
    OpsStateInterface();

    enum {
        StateNone = 0,
        StateSearching,
        StateRecording,
        StatePlaying,
        StateVoice2Text,
        StateAppQuit,
        StateAISrvAvailable,
        //Add other state at here
        StateMax,
    };

    static OpsStateInterface *instance();

    void operState(int type, bool isSet);

    bool isSearching() const;
    bool isRecording() const;
    bool isPlaying() const;
    bool isVoice2Text() const;
    bool isAppQuit() const;
    bool isAiSrvExist() const;

protected:
    quint64 m_states {StateNone};
};

#endif // OPSSTATEINTERFACE_H
