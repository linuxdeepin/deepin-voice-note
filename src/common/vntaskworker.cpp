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
#include "vntaskworker.h"
#include "task/vntask.h"

#include <DLog>

VNTaskWorker::VNTaskWorker(QObject *parent)
    : QThread(parent)
{
}

void VNTaskWorker::addTask(VNTask *task)
{
    m_taskLock.lock();
    m_safetyTaskQueue.push_back(task);
    m_taskLock.unlock();

    m_taskCondition.wakeAll();
}

void VNTaskWorker::setWorkerName(const QString &worker)
{
    m_workerName = worker;
}

void VNTaskWorker::quitWorker()
{
    m_fQuit = true;
}

void VNTaskWorker::run()
{
    do {
        VNTask *task = nullptr;

        m_taskLock.lock();

        if (m_fQuit) {
            qInfo() << m_workerName << "-->Going to quit!";
            break;
        }

        if (m_safetyTaskQueue.size() == 0) {
            qInfo() << m_workerName << "-->No task to do,waitting!";

            m_taskCondition.wait(&m_taskLock);
        } else {
            task = m_safetyTaskQueue.front();
            m_safetyTaskQueue.pop_front();
        }

        m_taskLock.unlock();

        if (nullptr != task) {
            task->run();

            if (task->autoDelete()) {
                task->deleteLater();
            }
        }
    } while (1);
}
