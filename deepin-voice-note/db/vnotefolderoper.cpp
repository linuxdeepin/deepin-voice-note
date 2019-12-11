#include "vnotefolderoper.h"
#include "common/utils.h"
#include "common/vnoteforlder.h"
#include "common/vnotedatamanager.h"
#include "db/vnotedbmanager.h"
#include "globaldef.h"

#include <QVariant>
#include <QObject>
#include <QDateTime>

#include <DLog>

const QStringList VNoteFolderOper::folderColumnsName = {
    "folder_id",
    "folder_name",
    "icon_path",
    "note_count",
    "create_time",
    "modify_time",
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
    bool delOK = false;

    VNoteFolder* removeFolder = VNoteDataManager::instance()->delFolder(folderId);

    if (nullptr != removeFolder) {
        delOK = true;
        QScopedPointer<VNoteFolder> release(removeFolder);
    }

    return delOK;

}

bool VNoteFolderOper::renameVNoteFolder(QString folderName)
{
    static constexpr char const *RENAME_FOLDERS_FMT = "UPDATE %s SET %s='%s', %s='%s' WHERE %s=%s;";

    bool isUpdateOK = false;

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

        if (VNoteDbManager::instance()->updateData(
                    VNoteDbManager::DB_TABLE::VNOTE_FOLDER_TBL
                    ,renameSql)) {
            m_folder->name       = folderName;
            m_folder->modifyTime = modifyTime;

            isUpdateOK = true;
        }
    }

    return isUpdateOK;
}

VNOTE_FOLDERS_MAP *VNoteFolderOper::loadVNoteFolders()
{
    static constexpr char const *QUERY_FOLDERS_FMT = "SELECT * FROM %s;";

    QString querySql;
    querySql.sprintf(QUERY_FOLDERS_FMT
                     , VNoteDbManager::FOLDER_TABLE_NAME
                     );

    QList<QList<QVariant>> result;

    VNOTE_FOLDERS_MAP * foldersMap = new VNOTE_FOLDERS_MAP();

    static struct timeval start,backups, end;

    gettimeofday(&start, nullptr);
    backups = start;

    if (VNoteDbManager::instance()->queryData(
                VNoteDbManager::DB_TABLE::VNOTE_FOLDER_TBL
                , querySql, result) ) {
        gettimeofday(&end, nullptr);

        qDebug() << "queryData(ms):" << TM(start, end);

        start = end;

        for(auto it : result) {
            VNoteFolder* folder = new  VNoteFolder();

            folder->id         = it.at(folder_id).toInt();
            folder->name       = it.at(folder_name).toString();
            folder->defaultIcon= it.at(default_icon).toInt();
            folder->notesCount = it.at(note_count).toInt();
            folder->createTime = QDateTime::fromString(
                        it.at(create_time).toString(),VNOTE_TIME_FMT);
            folder->modifyTime = QDateTime::fromString(
                        it.at(modify_time).toString(),VNOTE_TIME_FMT);

            folder->UI.icon = VNoteDataManager::instance()->getDefaultIcon(folder->defaultIcon);

            foldersMap->folders.insert(folder->id, folder);
        }

        gettimeofday(&end, nullptr);
        qDebug() << " load for cycle(ms):" << TM(start, end);
    }

    return foldersMap;
}

VNoteFolder *VNoteFolderOper::addFolder(VNoteFolder &folder)
{
    static constexpr char const *INSERT_FMT = "INSERT INTO %s (%s,%s) VALUES ('%s','%s');";
    static constexpr char const *NEWREC_FMT = "SELECT * FROM %s ORDER BY %s DESC LIMIT 1;";

    QString insertSql;

    insertSql.sprintf(INSERT_FMT
                      , VNoteDbManager::FOLDER_TABLE_NAME
                      , folderColumnsName[folder_name].toUtf8().data()
                      , folderColumnsName[icon_path].toUtf8().data()
                      , folder.name.toUtf8().data()
                      , folder.iconPath.toUtf8().data()
                      );

    QString queryNewRec;
    queryNewRec.sprintf(NEWREC_FMT
                      , VNoteDbManager::FOLDER_TABLE_NAME
                      , folderColumnsName[folder_id].toUtf8().data()
                      );

    QString sqls = insertSql+queryNewRec;

    QVariantList record;

    VNoteFolder* newFolder = nullptr;

    if (VNoteDbManager::instance()->insertData(
                VNoteDbManager::DB_TABLE::VNOTE_FOLDER_TBL
                , sqls
                , record) ) {
        newFolder = new VNoteFolder(folder);

        //TODO:
        //    We can update any feilds here  db return all feilds
        //of new record
        newFolder->id = record.at(folder_id).toInt();
        newFolder->createTime = QDateTime::fromString(
                    record.at(create_time).toString(),VNOTE_TIME_FMT);
        newFolder->modifyTime = QDateTime::fromString(
                    record.at(modify_time).toString(),VNOTE_TIME_FMT);

        VNoteDataManager::instance()->addFolder(newFolder);

        qInfo() << "New folder:" << newFolder->id
                << "Name:"       << newFolder->name
                << "Create time:" << newFolder->createTime
                << "Modify time:" << newFolder->modifyTime;
    }


    return newFolder;
}

VNoteFolder *VNoteFolderOper::getFolder(qint64 folderId)
{
    VNoteFolder* folder = VNoteDataManager::instance()->getFolder(folderId);

    return folder;
}

QString VNoteFolderOper::getDefaultFolderName()
{
    static constexpr char const *QUERY_DEFNAME_FMT = "SELECT COUNT(*) FROM %s WHERE %s LIKE '%s%%';";

    QString querySql; //
    querySql.sprintf(QUERY_DEFNAME_FMT
                     , VNoteDbManager::FOLDER_TABLE_NAME
                     , folderColumnsName[folder_name].toUtf8().data()
                     , QObject::tr("NewFolder").toUtf8().data()
                     );

    QString defaultFolderName = QObject::tr("NewFolder");

    QList<QList<QVariant>> result;

    if (VNoteDbManager::instance()->queryData(
                VNoteDbManager::DB_TABLE::VNOTE_FOLDER_TBL
                , querySql, result) ) {
        if (result.size() > 0) {
            defaultFolderName += QString("%1").arg(result[0].at(0).toInt()+1);
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
