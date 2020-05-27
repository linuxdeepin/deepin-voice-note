#include "vndatasafertask.h"
#include "db/vnotesaferoper.h"

#include <DLog>

VNDataSaferTask::VNDataSaferTask(const VDataSafer& safer
                                 ,QObject *parent)
    : VNTask(parent)
    , m_dataSafer(safer)
{

}

void VNDataSaferTask::run()
{
    VNoteSaferOper saferOper;

    if (VDataSafer::Safe == m_dataSafer.saferType) {
        saferOper.addSafer(m_dataSafer);
    } else if(VDataSafer::Unsafe == m_dataSafer.saferType) {
        //Normal operation,just remove the safer.
        saferOper.rmSafer(m_dataSafer);
    } else if (VDataSafer::ExceptionSafer == m_dataSafer.saferType) {
        //Exception operation happened ,need clear the data.
        QFileInfo fileInfo(m_dataSafer.path);

        if (fileInfo.exists()) {
            QFile::remove(m_dataSafer.path);
        }

        saferOper.rmSafer(m_dataSafer);
    }

    qInfo() << "Safer task done:"<< this
            << " " << m_dataSafer;
}
