#ifndef LOADSAFETEYDATAWORKER_H
#define LOADSAFETEYDATAWORKER_H

#include "task/vntask.h"
#include "common/datatypedef.h"

#include <QObject>

class LoadSafeteyDataWorker : public VNTask
{
    Q_OBJECT
public:
    explicit LoadSafeteyDataWorker(QObject *parent = nullptr);

signals:
    void saferLoaded(SafetyDatas* safers);
public slots:
protected:
    virtual void run();
};

#endif // LOADSAFETEYDATAWORKER_H
