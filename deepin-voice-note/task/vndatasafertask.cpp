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

    if (VDataSafer::Safe == m_dataSafer.safeType) {
        saferOper.addSafer(m_dataSafer);
    } else {
        QFileInfo fileInfo(m_dataSafer.path);

        if (fileInfo.exists()) {
            QFile::remove(m_dataSafer.path);
        }

        saferOper.rmSafer(m_dataSafer);
    }

    qInfo() << "Safer task done:"<< this
            << " " << m_dataSafer;
}
