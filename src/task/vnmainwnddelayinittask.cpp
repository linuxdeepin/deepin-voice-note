// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnmainwnddelayinittask.h"
// #include "views/vnotemainwindow.h"
#include <QDebug>

/**
 * @brief VNMainWndDelayInitTask::VNMainWndDelayInitTask
 * @param pMainWnd 主窗口
 * @param parent
 */
VNMainWndDelayInitTask::VNMainWndDelayInitTask(/*VNoteMainWindow *pMainWnd, */ QObject *parent)
    : VNTask(parent)
    // , m_pMainWnd(pMainWnd)
{
    // qInfo() << "VNMainWndDelayInitTask constructor called";
}

/**
 * @brief VNMainWndDelayInitTask::run
 */
void VNMainWndDelayInitTask::run()
{
    // qInfo() << "Running VNMainWndDelayInitTask";
    if (nullptr != m_pMainWnd) {
        // qInfo() << "m_pMainWnd is not nullptr";
        //Delay initialize work
        // m_pMainWnd->initDelayWork();
    }
    // qInfo() << "VNMainWndDelayInitTask run finished";
}
