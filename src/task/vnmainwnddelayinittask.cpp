// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnmainwnddelayinittask.h"
#include "views/vnotemainwindow.h"
#include "common/vtextspeechandtrmanager.h"
#include "common/opsstateinterface.h"
#include <QDBusInterface>
#include <QDBusReply>

/**
 * @brief VNMainWndDelayInitTask::VNMainWndDelayInitTask
 * @param pMainWnd 主窗口
 * @param parent
 */
VNMainWndDelayInitTask::VNMainWndDelayInitTask(VNoteMainWindow *pMainWnd, QObject *parent)
    : VNTask(parent)
    , m_pMainWnd(pMainWnd)
{
}

/**
 * @brief VNMainWndDelayInitTask::run
 */
void VNMainWndDelayInitTask::run()
{
    if (nullptr != m_pMainWnd) {
        // 激活ai dbus 服务
        qInfo() << "Starting AI service activation...";
        
        // 1. 检查 UOS AI 服务是否存在并激活
        VTextSpeechAndTrManager::instance()->getTextToSpeechEnable();
        
        // 延时初始化主窗口
        m_pMainWnd->initDelayWork();
    }
}
