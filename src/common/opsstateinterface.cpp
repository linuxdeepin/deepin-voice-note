// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "opsstateinterface.h"
#include <QDBusConnectionInterface>
#include <QDBusInterface>
// #include <DSysInfo>
#include <DLog>

// DCORE_USE_NAMESPACE

/**
 * @brief OpsStateInterface::OpsStateInterface
 */
OpsStateInterface::OpsStateInterface()
{
    // qInfo() << "OpsStateInterface constructor called";
    operState(StateAISrvAvailable, true);
    // qInfo() << "OpsStateInterface constructor finished";
}

/**
 * @brief OpsStateInterface::operState
 * @param type 操作目标
 * @param isSet 设置状态
 */
void OpsStateInterface::operState(int type, bool isSet)
{
    // qInfo() << "Operating state, type:" << type << "isSet:" << isSet;
    quint8 shift = static_cast<quint8>(type);

    if (shift > StateNone && shift < StateMax) {
        if (isSet) {
            m_states |= (1 << shift);
            // qDebug() << "Setting state" << type << "to true";
        } else {
            m_states &= (~(1 << shift));
            // qDebug() << "Setting state" << type << "to false";
        }
    } else {
        qCritical() << "Operation error: Invalid opsType =" << type 
                   << " (must be between" << StateNone << "and" << StateMax << ")";
    }
    // qInfo() << "State operation finished";
}

/**
 * @brief OpsStateInterface::isSearching
 * @return true 正在搜索
 */
bool OpsStateInterface::isSearching() const
{
    // qInfo() << "Checking if searching";
    return (m_states & (1 << StateSearching));
}

/**
 * @brief OpsStateInterface::isRecording
 * @return true 正在录音
 */
bool OpsStateInterface::isRecording() const
{
    // qInfo() << "Checking if recording";
    return (m_states & (1 << StateRecording));
}

/**
 * @brief OpsStateInterface::isPlaying
 * @return true 正在播放
 */
bool OpsStateInterface::isPlaying() const
{
    // qInfo() << "Checking if playing";
    return (m_states & (1 << StatePlaying));
}

/**
 * @brief OpsStateInterface::isVoice2Text
 * @return true 正在语音转文字
 */
bool OpsStateInterface::isVoice2Text() const
{
    // qInfo() << "Checking if voice to text";
    return (m_states & (1 << StateVoice2Text));
}

/**
 * @brief OpsStateInterface::isAppQuit
 * @return true 需要退出
 */
bool OpsStateInterface::isAppQuit() const
{
    // qInfo() << "Checking if app quit";
    return (m_states & (1 << StateAppQuit));
}

/**
 * @brief OpsStateInterface::isAiSrvExist
 * @return true 语音功能可用
 */
bool OpsStateInterface::isAiSrvExist() const
{
    // qInfo() << "Checking if AI service exists";
    return (m_states & (1 << StateAISrvAvailable));
}

/**
 * @brief OpsStateInterface::instance
 * @return 单例对象
 */
OpsStateInterface *OpsStateInterface::instance()
{
    // qInfo() << "OpsStateInterface instance requested";
    static OpsStateInterface _instance;
    return &_instance;
}
