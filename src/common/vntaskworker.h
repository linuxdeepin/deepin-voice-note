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
#ifndef VNTASKWORKER_H
#define VNTASKWORKER_H

#include <QThread>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>

class VNTask;

class VNTaskWorker : public QThread
{
    Q_OBJECT
public:
    explicit VNTaskWorker(QObject *parent = nullptr);
    //添加任务
    void addTask(VNTask *task);
    //设置任务名称
    void setWorkerName(const QString &worker);
    //退出，结束线程
    void quitWorker();
signals:

public slots:
protected:
    virtual void run() override;

protected:
    QVector<VNTask *> m_safetyTaskQueue;
    QMutex m_taskLock;
    QWaitCondition m_taskCondition;
    QString m_workerName {"VNTaskWoker"};
    volatile bool m_fQuit {false};
};

#endif // VNTASKWORKER_H
