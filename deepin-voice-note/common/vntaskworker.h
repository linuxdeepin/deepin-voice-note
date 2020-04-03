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

    void addTask(VNTask* task);
    void setWorkerName(const QString& worker);
    void quitWorker();
signals:

public slots:
protected:
    virtual void run() override;
protected:
    QVector<VNTask*> m_safetyTaskQueue;
    QMutex           m_taskLock;
    QWaitCondition   m_taskCondition;
    QString          m_workerName {"VNTaskWoker"};
    volatile bool    m_fQuit {false};
};

#endif // VNTASKWORKER_H
