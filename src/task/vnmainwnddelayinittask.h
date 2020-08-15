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
#ifndef VNMAINWNDDELAYINITTASK_H
#define VNMAINWNDDELAYINITTASK_H

#include "vntask.h"
#include "views/vnotemainwindow.h"

#include <QObject>

class VNoteMainWindow;
//延时任务线程，一些程序启动时不需要马上完成的任务，可以放在此线程中处理
class VNMainWndDelayInitTask : public VNTask
{
    Q_OBJECT
public:
    explicit VNMainWndDelayInitTask(VNoteMainWindow *pMainWnd, QObject *parent = nullptr);

signals:

public slots:

protected:
    //执行任务
    virtual void run();

protected:
    VNoteMainWindow *m_pMainWnd {nullptr};
};

#endif // VNMAINWNDDELAYINITTASK_H
