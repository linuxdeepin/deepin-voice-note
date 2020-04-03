#include "vnotesaferoper.h"
#include "common/datatypedef.h"
#include "db/dbvisitor.h"
#include "db/vnotedbmanager.h"

#include <DLog>

const QStringList VNoteSaferOper::saferColumnsName = {
    "id",
    "folder_id",
    "note_id",
    "path",
    "state",
    "meta_data",
    "create_time",
};

VNoteSaferOper::VNoteSaferOper()
{

}

VNoteSaferOper::VNoteSaferOper(const VDataSafer &safer)
    :m_dataSafer(safer)
{

}

SafetyDatas *VNoteSaferOper::loadSafers()
{
    static constexpr char const *QUERY_FOLDERS_FMT = "SELECT * FROM %s ORDER BY %s DESC ;";

    QString querySql;
    querySql.sprintf(QUERY_FOLDERS_FMT
                     , VNoteDbManager::SAFER_TABLE_NAME
                     , saferColumnsName[create_time].toUtf8().data()
                     );

    SafetyDatas* safers = new SafetyDatas();

    SaferQryDbVisitor folderVisitor(VNoteDbManager::instance()->getVNoteDb(), safers);

    if (!VNoteDbManager::instance()->queryData(querySql, &folderVisitor) ) {
      qCritical() << "Query faild!";
    }

    return safers;
}

void VNoteSaferOper::addSafer(const VDataSafer &safer)
{
    static constexpr char const *INSERT_SAFER_FMT = "INSERT INTO %s (%s,%s,%s) VALUES (%s,%s,'%s');";

    if (safer.isValid()) {
        QString addSaferSql;
        addSaferSql.sprintf(INSERT_SAFER_FMT
                            , VNoteDbManager::SAFER_TABLE_NAME
                            , saferColumnsName[folder_id].toUtf8().data()
                            , saferColumnsName[note_id].toUtf8().data()
                            , saferColumnsName[path].toUtf8().data()
                            , QString("%1").arg(safer.folder_id).toUtf8().data()
                            , QString("%1").arg(safer.note_id).toUtf8().data()
                            , safer.path.toUtf8().data()
                            );

        QStringList sqls;
        sqls.append(addSaferSql);

        AddSaferDbVisitor addSaferVisitor(VNoteDbManager::instance()->getVNoteDb(), nullptr);

        if (Q_UNLIKELY(!VNoteDbManager::instance()->insertData(sqls, &addSaferVisitor))) {
            qInfo() << "Add safer failed:" << safer;
        }
    } else {
        qInfo() << "Invalid parameter:" << safer;
    }
}

void VNoteSaferOper::rmSafer(const VDataSafer &safer)
{
    static constexpr char const *DEL_SAFER_FMT = "DELETE FROM %s WHERE %s=%s AND %s=%s AND %s='%s';";

    if (safer.isValid()) {
        QString delSaferSql;
        delSaferSql.sprintf(DEL_SAFER_FMT
                            , VNoteDbManager::SAFER_TABLE_NAME
                            , saferColumnsName[folder_id].toUtf8().data()
                            , QString("%1").arg(safer.folder_id).toUtf8().data()
                            , saferColumnsName[note_id].toUtf8().data()
                            , QString("%1").arg(safer.note_id).toUtf8().data()
                            , saferColumnsName[path].toUtf8().data()
                            , safer.path.toUtf8().data()
                            );

        QStringList sqls;
        sqls.append(delSaferSql);

        if (Q_UNLIKELY(!VNoteDbManager::instance()->deleteData(sqls))) {
            qInfo() << "Delete safer failed:" << safer;
        }
    } else {
        qInfo() << "Invalid parameter:" << safer;
    }
}
