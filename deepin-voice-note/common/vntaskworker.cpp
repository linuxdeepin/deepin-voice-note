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
        VNTask* task = nullptr;

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
    } while(1);
}
