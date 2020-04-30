#include "vnotesaferoper.h"
#include "common/datatypedef.h"
#include "db/dbvisitor.h"
#include "db/vnotedbmanager.h"

#include <DLog>

//const QStringList VNoteSaferOper::saferColumnsName = {
//    "id",
//    "folder_id",
//    "note_id",
//    "path",
//    "state",
//    "meta_data",
//    "create_time",
//};

VNoteSaferOper::VNoteSaferOper()
{

}

VNoteSaferOper::VNoteSaferOper(const VDataSafer &safer)
    :m_dataSafer(safer)
{

}

SafetyDatas *VNoteSaferOper::loadSafers()
{
    SafetyDatas* safers = new SafetyDatas();

    SaferQryDbVisitor folderVisitor(VNoteDbManager::instance()->getVNoteDb(), nullptr, safers);

    if (!VNoteDbManager::instance()->queryData(&folderVisitor) ) {
      qCritical() << "Query faild!";
    }

    return safers;
}

void VNoteSaferOper::addSafer(const VDataSafer &safer)
{
    if (safer.isValid()) {
        AddSaferDbVisitor addSaferVisitor(VNoteDbManager::instance()->getVNoteDb(), &safer, nullptr);

        if (Q_UNLIKELY(!VNoteDbManager::instance()->insertData(&addSaferVisitor))) {
            qInfo() << "Add safer failed:" << safer;
        }
    } else {
        qInfo() << "Invalid parameter:" << safer;
    }
}

void VNoteSaferOper::rmSafer(const VDataSafer &safer)
{
    if (safer.isValid()) {
        DelSaferDbVisitor delSaferVisitor(VNoteDbManager::instance()->getVNoteDb(), &safer, nullptr);

        if (Q_UNLIKELY(!VNoteDbManager::instance()->deleteData(&delSaferVisitor))) {
            qInfo() << "Delete safer failed:" << safer;
        }
    } else {
        qInfo() << "Invalid parameter:" << safer;
    }
}
