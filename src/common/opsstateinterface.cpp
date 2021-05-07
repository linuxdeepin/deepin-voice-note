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
#include "opsstateinterface.h"
#include <DSysInfo>
#include <DLog>

DCORE_USE_NAMESPACE

static OpsStateInterface *objectInstance = nullptr;

/**
 * @brief OpsStateInterface::OpsStateInterface
 */
OpsStateInterface::OpsStateInterface()
{
    bool fAiServiceExist = false;
    DSysInfo::UosEdition uosType = DSysInfo::uosEditionType();
    switch (uosType) {
    case DSysInfo::UosProfessional:
    case DSysInfo::UosHome:
        fAiServiceExist = true;
        break;
    default:
        break;
    }
    QString systemInfo = QString("[%1-%2-%3]")
                             .arg(DSysInfo::uosSystemName())
                             .arg(DSysInfo::uosProductTypeName())
                             .arg(DSysInfo::uosEditionName());
    qInfo() << systemInfo << " IsAvailable use voice to text:" << fAiServiceExist;
    operState(StateAISrvAvailable, fAiServiceExist);
}

/**
 * @brief OpsStateInterface::operState
 * @param type 操作目标
 * @param isSet 设置状态
 */
void OpsStateInterface::operState(int type, bool isSet)
{
    quint8 shift = static_cast<quint8>(type);

    if (shift > StateNone && shift < StateMax) {
        if (isSet) {
            m_states |= (1 << shift);
        } else {
            m_states &= (~(1 << shift));
        }
    } else {
        qCritical() << "Operation error:Invalid opsType =" << type;
    }
}

/**
 * @brief OpsStateInterface::isSearching
 * @return true 正在搜索
 */
bool OpsStateInterface::isSearching() const
{
    return (m_states & (1 << StateSearching));
}

/**
 * @brief OpsStateInterface::isRecording
 * @return true 正在录音
 */
bool OpsStateInterface::isRecording() const
{
    return (m_states & (1 << StateRecording));
}

/**
 * @brief OpsStateInterface::isPlaying
 * @return true 正在播放
 */
bool OpsStateInterface::isPlaying() const
{
    return (m_states & (1 << StatePlaying));
}

/**
 * @brief OpsStateInterface::isVoice2Text
 * @return true 正在语音转文字
 */
bool OpsStateInterface::isVoice2Text() const
{
    return (m_states & (1 << StateVoice2Text));
}

/**
 * @brief OpsStateInterface::isAppQuit
 * @return true 需要退出
 */
bool OpsStateInterface::isAppQuit() const
{
    return (m_states & (1 << StateAppQuit));
}

/**
 * @brief OpsStateInterface::isAiSrvExist
 * @return true 语音功能可用
 */
bool OpsStateInterface::isAiSrvExist() const
{
    return (m_states & (1 << StateAISrvAvailable));
}

/**
 * @brief OpsStateInterface::instance
 * @return 单例对象
 */
OpsStateInterface *OpsStateInterface::instance()
{
    if (objectInstance == nullptr) {
        objectInstance = new OpsStateInterface;
    }
    return objectInstance;
}
