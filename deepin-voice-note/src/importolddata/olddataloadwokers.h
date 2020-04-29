#ifndef OLDDATALOADWOKERS_H
#define OLDDATALOADWOKERS_H

#include "common/datatypedef.h"
#include "task/vntask.h"

class OldDataLoadTask : public VNTask
{
    Q_OBJECT
public:
    OldDataLoadTask(QObject *parent=nullptr);

protected:
    virtual void run();
signals:
    void finishLoad();
};

class OldDataUpgradeTask : public VNTask
{
    Q_OBJECT
public:
    OldDataUpgradeTask(QObject *parent=nullptr);

protected:
    virtual void run();
signals:
    void progressValue(qint32);
    void finishUpgrade();
};

#endif // OLDDATALOADWOKERS_H
