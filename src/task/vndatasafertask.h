#ifndef VNDATASAFERTASK_H
#define VNDATASAFERTASK_H

#include "vntask.h"
#include "common/datatypedef.h"

#include <QObject>

class VNDataSaferTask : public VNTask
{
    Q_OBJECT
public:
    explicit VNDataSaferTask(const VDataSafer& safer
                             ,QObject *parent = nullptr);
signals:

public slots:
protected:
    virtual void run();
protected:
    VDataSafer m_dataSafer;
};

#endif // VNDATASAFERTASK_H
