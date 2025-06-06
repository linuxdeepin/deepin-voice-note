// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnmainwnddelayinittask.h"
#include "views/vnotemainwindow.h"
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
        // 直接尝试调用 DBus 接口Add commentMore actions
        QDBusInterface aiInterface("com.iflytek.aiassistant",
                                 "/aiassistant/deepinmain",
                                 "org.freedesktop.DBus.Introspectable");

        QDBusReply<QString> reply = aiInterface.call("Introspect");
        if (reply.isValid()) {
            qInfo() << "com.iflytek.aiassistant service is available.";
        } else {
            qInfo() << "Failed to connect to com.iflytek.aiassistant: " << reply.error().message();
        }
        //Delay initialize work
        m_pMainWnd->initDelayWork();
    }
}
