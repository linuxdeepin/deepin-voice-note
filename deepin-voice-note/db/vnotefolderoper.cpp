#include "vnotefolderoper.h"
#include "vnoteitemoper.h"
#include "common/utils.h"
#include "common/vnoteforlder.h"
#include "common/vnotedatamanager.h"
#include "db/vnotedbmanager.h"
#include "db/dbvisitor.h"
#include "globaldef.h"

#include <QVariant>
#include <QObject>
#include <QDateTime>

#include <DLog>

const QStringList VNoteFolderOper::folderColumnsName = {
    "folder_id",
    "folder_name",
    "default_icon",
    "icon_path",
    "folder_state",
    "create_time",
    "modify_time",
    "delete_time",
};

VNoteFolderOper::VNoteFolderOper(VNoteFolder* folder)
    :m_folder(folder)
{
}

bool VNoteFolderOper::isNoteItemLoaded()
{
    return (m_folder) ? m_folder->fIsDataLoaded : false;
}

bool VNoteFolderOper::deleteVNoteFolder(qint64 folderId)
{
    static constexpr char const *DEL_FOLDER_FMT = "DELETE FROM %s WHERE %s=%s;";
    static constexpr char const *DEL_FNOTE_FMT  = "DELETE FROM %s WHERE %s=%s;";

    QString deleteFolderSql;

    deleteFolderSql.sprintf(DEL_FOLDER_FMT
                      , VNoteDbManager::FOLDER_TABLE_NAME
                      , folderColumnsName[folder_id].toUtf8().data()
                      , QString("%1").arg(folderId).toUtf8().data()
                      );

    QString deleteNotesSql;

    deleteNotesSql.sprintf(DEL_FNOTE_FMT
                      , VNoteDbManager::NOTES_TABLE_NAME
                      , VNoteItemOper::noteColumnsName[folder_id].toUtf8().data()
                      , QString("%1").arg(folderId).toUtf8().data()
                      );

    bool delOK = false;

    QStringList sqls;
    sqls.append(deleteFolderSql);
    sqls.append(deleteNotesSql);

    if (VNoteDbManager::instance()->deleteData(sqls) ) {
        delOK = true;
        QScopedPointer<VNoteFolder> release(VNoteDataManager::instance()->delFolder(folderId));
    }

    return delOK;
}

bool VNoteFolderOper::deleteVNoteFolder(VNoteFolder *folder)
{
    bool delOK = true;
    if (nullptr != folder) {
        delOK = deleteVNoteFolder(folder->id);
    }

    return delOK;
}

bool VNoteFolderOper::renameVNoteFolder(QString folderName)
{
    static constexpr char const *RENAME_FOLDERS_FMT = "UPDATE %s SET %s='%s', %s='%s' WHERE %s=%s;";

    bool isUpdateOK = false;

    QStringList sqls;
    QString renameSql;

    if (nullptr != m_folder) {
        QDateTime modifyTime = QDateTime::currentDateTime();

        renameSql.sprintf(RENAME_FOLDERS_FMT
                          , VNoteDbManager::FOLDER_TABLE_NAME
                          , folderColumnsName[folder_name].toUtf8().data()
                          , folderName.toUtf8().data()
                          , folderColumnsName[modify_time].toUtf8().data()
                          , modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data()
                          , folderColumnsName[folder_id].toUtf8().data()
                          , QString("%1").arg(m_folder->id).toUtf8().data()
                          );

        sqls.append(renameSql);

        if (VNoteDbManager::instance()->updateData(sqls)) {
            m_folder->name       = folderName;
            m_folder->modifyTime = modifyTime;

            isUpdateOK = true;
        }
    }

    return isUpdateOK;
}

VNOTE_FOLDERS_MAP *VNoteFolderOper::loadVNoteFolders()
{
    static constexpr char const *QUERY_FOLDERS_FMT = "SELECT * FROM %s  ORDER BY %s DESC ;";

    QString querySql;
    querySql.sprintf(QUERY_FOLDERS_FMT
                     , VNoteDbManager::FOLDER_TABLE_NAME
                     , folderColumnsName[create_time].toUtf8().data()
                     );

    VNOTE_FOLDERS_MAP * foldersMap = new VNOTE_FOLDERS_MAP();

    //DataManager should set autoRelease flag
    foldersMap->autoRelease = true;

    FolderQryDbVisitor folderVisitor(VNoteDbManager::instance()->getVNoteDb(), foldersMap);

    if (!VNoteDbManager::instance()->queryData(querySql, &folderVisitor) ) {
      qCritical() << "Query faild!";
    }

    return foldersMap;
}

VNoteFolder *VNoteFolderOper::addFolder(VNoteFolder &folder)
{
    static constexpr char const *INSERT_FMT = "INSERT INTO %s (%s,%s) VALUES ('%s', %s);";
    static constexpr char const *NEWREC_FMT = "SELECT * FROM %s ORDER BY %s DESC LIMIT 1;";

    QString insertSql;

    insertSql.sprintf(INSERT_FMT
                      , VNoteDbManager::FOLDER_TABLE_NAME
                      , folderColumnsName[folder_name].toUtf8().data()
                      , folderColumnsName[default_icon].toUtf8().data()
                      , folder.name.toUtf8().data()
                      , QString("%1").arg(folder.defaultIcon).toUtf8().data()
                      );

    QString queryNewRec;
    queryNewRec.sprintf(NEWREC_FMT
                      , VNoteDbManager::FOLDER_TABLE_NAME
                      , folderColumnsName[folder_id].toUtf8().data()
                      );

    QStringList sqls;
    sqls.append(insertSql);
    sqls.append(queryNewRec);

    QVariantList record;

    VNoteFolder *newFolder = new VNoteFolder();
    AddFolderDbVisitor addFolderVisitor(VNoteDbManager::instance()->getVNoteDb(), newFolder);


    if (VNoteDbManager::instance()->insertData(sqls, &addFolderVisitor) ) {

        //TODO:
        //    DbVisitor can update any feilds here  db return all feilds
        //of new record. Just load icon here
        newFolder->UI.icon = VNoteDataManager::instance()->getDefaultIcon(newFolder->defaultIcon);

        VNoteDataManager::instance()->addFolder(newFolder);

        qInfo() << "New folder:" << newFolder->id
                << "Name:"       << newFolder->name
                << "Create time:" << newFolder->createTime
                << "Modify time:" << newFolder->modifyTime;
    } else {
        qCritical() << "Add folder failded:"
                << "New folder:" << newFolder->id
                << "Name:"       << newFolder->name
                << "Create time:" << newFolder->createTime
                << "Modify time:" << newFolder->modifyTime;

        QScopedPointer<VNoteFolder> autoRelease(newFolder);
        newFolder = nullptr;
    }

    return newFolder;
}

VNoteFolder *VNoteFolderOper::getFolder(qint64 folderId)
{
    VNoteFolder* folder = VNoteDataManager::instance()->getFolder(folderId);

    return folder;
}

qint32 VNoteFolderOper::getFoldersCount()
{
    return VNoteDataManager::instance()->folderCount();
}

QString VNoteFolderOper::getDefaultFolderName()
{
    static constexpr char const *QUERY_DEFNAME_FMT = "SELECT COUNT(*) FROM %s WHERE %s LIKE '%s%%';";

    QString querySql; //
    querySql.sprintf(QUERY_DEFNAME_FMT
                     , VNoteDbManager::FOLDER_TABLE_NAME
                     , folderColumnsName[folder_name].toUtf8().data()
                     , DApplication::translate("AppMain","NewFolder").toUtf8().data()
                     );

    QString defaultFolderName = DApplication::translate("AppMain","NewFolder");

    int foldersCount = 0;
    CountDbVisitor folderVisitor(VNoteDbManager::instance()->getVNoteDb(), &foldersCount);

    if (VNoteDbManager::instance()->queryData(querySql, &folderVisitor) ) {
        if (foldersCount > 0) {
            defaultFolderName += QString("%1").arg(foldersCount+1);
        } else {
            defaultFolderName += QString("%1").arg(1);
        }
    }

    return defaultFolderName;
}

qint32 VNoteFolderOper::getDefaultIcon()
{
    const int defalutIconCnt = 10;

    QTime time = QTime::currentTime();
    qsrand(static_cast<uint>(time.msec()+time.second()*1000));

    return (qrand()%defalutIconCnt);
}

QImage VNoteFolderOper::getDefaultIcon(qint32 index)
{
    return VNoteDataManager::instance()->getDefaultIcon(index);
}
