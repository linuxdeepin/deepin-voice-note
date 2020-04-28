#include "dbvisitor.h"
#include "globaldef.h"
#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"
#include "db/vnotesaferoper.h"
#include "common/datatypedef.h"
#include "common/metadataparser.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "db/vnotedbmanager.h"

#include <QVariant>

#include <DLog>

const QStringList DbVisitor::DBFolder::folderColumnsName = {
    "folder_id",
    "category_id",
    "folder_name",
    "default_icon",
    "icon_path",
    "folder_state",
    "max_noteid",
    "create_time",
    "modify_time",
    "delete_time",
};

const QStringList DbVisitor::DBNote::noteColumnsName = {
    "note_id",
    "folder_id",
    "note_type",
    "note_title",
    "meta_data",
    "note_state",
    "create_time",
    "modify_time",
    "delete_time",
};

const QStringList DbVisitor::DBSafer::saferColumnsName = {
    "id",
    "folder_id",
    "note_id",
    "path",
    "state",
    "meta_data",
    "create_time",
};

DbVisitor::DbVisitor(QSqlDatabase& db, const void *inParam, void* result)
    :m_sqlQuery(new QSqlQuery(db))
{
    param.ptr = inParam;
    results.ptr = result;
}

DbVisitor::~DbVisitor()
{

}

bool DbVisitor::visitorData()
{
    // Default do nothing
    return true;
}

QSqlQuery *DbVisitor::sqlQuery()
{
    return m_sqlQuery.get();
}

const QStringList &DbVisitor::dbvSqls()
{
    return m_dbvSqls;
}

DbVisitor::ExtraData &DbVisitor::extraData()
{
    return m_extraData;
}

void DbVisitor::checkSqlStr(QString &sql)
{
    sql.replace("'", "''");
}

FolderQryDbVisitor::FolderQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool FolderQryDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.folders) {
        isOK = true;

        while(m_sqlQuery->next()) {
            VNoteFolder* folder = new VNoteFolder();

            folder->id          = m_sqlQuery->value(VNoteFolderOper::folder_id).toInt();
            folder->category    = m_sqlQuery->value(VNoteFolderOper::category_id).toInt();
            folder->name        = m_sqlQuery->value(VNoteFolderOper::folder_name).toString();
            folder->defaultIcon = m_sqlQuery->value(VNoteFolderOper::default_icon).toInt();
            folder->iconPath    = m_sqlQuery->value(VNoteFolderOper::icon_path).toString();
            folder->folder_state= m_sqlQuery->value(VNoteFolderOper::folder_state).toInt();

            folder->maxNoteIdRef()= m_sqlQuery->value(VNoteFolderOper::max_noteid).toInt();

            folder->createTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteFolderOper::create_time).toString(),VNOTE_TIME_FMT);
            folder->modifyTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteFolderOper::modify_time).toString(),VNOTE_TIME_FMT);
            folder->deleteTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteFolderOper::delete_time).toString(),VNOTE_TIME_FMT);

            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************

            //Get default icon image
            folder->UI.icon = VNoteDataManager::instance()->getDefaultIcon(folder->defaultIcon, IconsType::DefaultIcon);
            folder->UI.grayIcon = VNoteDataManager::instance()->getDefaultIcon(folder->defaultIcon, IconsType::DefaultGrayIcon);


            results.folders->folders.insert(folder->id, folder);
        }
    }

    return isOK;
}

bool FolderQryDbVisitor::prepareSqls()
{
    static constexpr char const *QUERY_FOLDERS_FMT = "SELECT * FROM %s  ORDER BY %s DESC ;";

    QString querySql;
    querySql.sprintf(QUERY_FOLDERS_FMT
                     , VNoteDbManager::FOLDER_TABLE_NAME
                     , DBFolder::folderColumnsName[DBFolder::create_time].toUtf8().data()
            );

    m_dbvSqls.append(querySql);

    return true;
}

NoteQryDbVisitor::NoteQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool NoteQryDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.notes) {
        isOK = true;

        MetaDataParser metaParser;

        while(m_sqlQuery->next()) {
            VNoteItem* note = new VNoteItem();

            note->noteId      = m_sqlQuery->value(VNoteItemOper::note_id).toInt();
            note->folderId    = m_sqlQuery->value(VNoteItemOper::folder_id).toInt();
            note->noteType    = m_sqlQuery->value(VNoteItemOper::note_type).toInt();
            note->noteTitle   = m_sqlQuery->value(VNoteItemOper::note_title).toString();

            //Parse meta data
            QVariant metaData  = m_sqlQuery->value(VNoteItemOper::meta_data);
            note->setMetadata(metaData);
            metaParser.parse(metaData, note);

            note->noteState   = m_sqlQuery->value(VNoteItemOper::note_state).toInt();

            note->createTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteItemOper::create_time).toString(),VNOTE_TIME_FMT);
            note->modifyTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteItemOper::modify_time).toString(),VNOTE_TIME_FMT);
            note->deleteTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteItemOper::modify_time).toString(),VNOTE_TIME_FMT);

            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************

            VNOTE_ALL_NOTES_DATA_MAP::iterator it =
                    results.notes->notes.find(note->folderId);
#ifdef QT_QML_DEBUG
            qInfo() << "" << (*note);
#endif
            //TODO
            //    If find the folder add note to it, or need create
            //folder items map first;
            if (it != results.notes->notes.end()) {
                (*it)->folderNotes.insert(note->noteId, note);
            } else {
                VNOTE_ITEMS_MAP* folderNotes = new VNOTE_ITEMS_MAP();

                //DataManager data should set autoRelease flag
                folderNotes->autoRelease = true;

                folderNotes->folderNotes.insert(note->noteId, note);
                results.notes->notes.insert(note->folderId, folderNotes);
            }
        }
    }

    return isOK;
}

bool NoteQryDbVisitor::prepareSqls()
{
    static constexpr char const *QUERY_NOTES_FMT = "SELECT * FROM %s ORDER BY %s;";

    QString querySql;
    querySql.sprintf(QUERY_NOTES_FMT
                     , VNoteDbManager::NOTES_TABLE_NAME
                     , DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data()
            );

    m_dbvSqls.append(querySql);

    return true;
}

MaxIdFolderDbVisitor::MaxIdFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool MaxIdFolderDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.id) {
        isOK = true;

        while(m_sqlQuery->next()) {
            *results.id =  m_sqlQuery->value(0).toInt();
            break;
        }
    }

    return isOK;
}

bool MaxIdFolderDbVisitor::prepareSqls()
{
    //TODO:
    //    The default folder is auto-increment, and
    //may be separate data for different category in future.
    //We query the max id every time now, need optimize when
    //category feature is added.
    //
    //SQLITE related:
    //    primary key table name : SQLITE_SEQUENCE
    //    max primary key feild  : SEQ
    static constexpr char const *QUERY_DEFNAME_FMT = "SELECT SEQ FROM SQLITE_SEQUENCE where NAME='%s';";
    static constexpr char const *RESET_FOLDER_ID   = "UPDATE SQLITE_SEQUENCE SET SEQ=%s where NAME='%s';";

    QString querySql;
    querySql.sprintf(QUERY_DEFNAME_FMT
                     , VNoteDbManager::FOLDER_TABLE_NAME
                     );

    m_dbvSqls.append(querySql);

    if (m_extraData.data.flag) {
        QString resetFolderIdSql;
        resetFolderIdSql.sprintf(RESET_FOLDER_ID
                                 , QString("%1").arg(0).toUtf8().data()
                                 ,VNoteDbManager::FOLDER_TABLE_NAME
                                 );
        m_dbvSqls.append(resetFolderIdSql);
    }

    return true;
}

AddFolderDbVisitor::AddFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool AddFolderDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.newFolder) {
        isOK = true;

        while(m_sqlQuery->next()) {
            results.newFolder->id          = m_sqlQuery->value(VNoteFolderOper::folder_id).toInt();
            results.newFolder->category    = m_sqlQuery->value(VNoteFolderOper::category_id).toInt();
            results.newFolder->name        = m_sqlQuery->value(VNoteFolderOper::folder_name).toString();
            results.newFolder->defaultIcon = m_sqlQuery->value(VNoteFolderOper::default_icon).toInt();
            results.newFolder->iconPath    = m_sqlQuery->value(VNoteFolderOper::icon_path).toString();
            results.newFolder->folder_state= m_sqlQuery->value(VNoteFolderOper::folder_state).toInt();

            results.newFolder->createTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteFolderOper::create_time).toString(),VNOTE_TIME_FMT);
            results.newFolder->modifyTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteFolderOper::modify_time).toString(),VNOTE_TIME_FMT);
            results.newFolder->deleteTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteFolderOper::delete_time).toString(),VNOTE_TIME_FMT);

            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************

            break;
        }
    }

    return isOK;
}

bool AddFolderDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *INSERT_FMT = "INSERT INTO %s (%s,%s) VALUES ('%s', %s);";
    static constexpr char const *NEWREC_FMT = "SELECT * FROM %s ORDER BY %s DESC LIMIT 1;";

    if (nullptr != param.newFolder) {
        QString insertSql;

        insertSql.sprintf(INSERT_FMT
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::folder_name].toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::default_icon].toUtf8().data()
                , param.newFolder->name.toUtf8().data()
                , QString("%1").arg(param.newFolder->defaultIcon).toUtf8().data()
                );

        QString queryNewRec;
        queryNewRec.sprintf(NEWREC_FMT
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data()
                );

        m_dbvSqls.append(insertSql);
        m_dbvSqls.append(queryNewRec);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

RenameFolderDbVisitor::RenameFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool RenameFolderDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *RENAME_FOLDERS_FMT = "UPDATE %s SET %s='%s', %s='%s' WHERE %s=%s;";

    const VNoteFolder* folder = param.newFolder;

    if (nullptr != folder) {

        QString sqlFolderName = folder->name;
        checkSqlStr(sqlFolderName);

        QString renameSql;

        renameSql.sprintf(RENAME_FOLDERS_FMT
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::folder_name].toUtf8().data()
                , sqlFolderName.toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data()
                , folder->modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data()
                , QString("%1").arg(folder->id).toUtf8().data()
                );

        m_dbvSqls.append(renameSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

DelFolderDbVisitor::DelFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool DelFolderDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *DEL_FOLDER_FMT = "DELETE FROM %s WHERE %s=%s;";
    static constexpr char const *DEL_FNOTE_FMT  = "DELETE FROM %s WHERE %s=%s;";

    if (nullptr != param.id) {
        qint64 folderId = *param.id;
        QString deleteFolderSql;

        deleteFolderSql.sprintf(DEL_FOLDER_FMT
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data()
                , QString("%1").arg(folderId).toUtf8().data()
                );

        QString deleteNotesSql;

        deleteNotesSql.sprintf(DEL_FNOTE_FMT
                , VNoteDbManager::NOTES_TABLE_NAME
                , DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data()
                , QString("%1").arg(folderId).toUtf8().data()
                );

        m_dbvSqls.append(deleteFolderSql);
        m_dbvSqls.append(deleteNotesSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

AddNoteDbVisitor::AddNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool AddNoteDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.newNote) {
        isOK = true;

        MetaDataParser metaParser;

        while(m_sqlQuery->next()) {
            VNoteItem* note = results.newNote;

            note->noteId      = m_sqlQuery->value(VNoteItemOper::note_id).toInt();
            note->folderId    = m_sqlQuery->value(VNoteItemOper::folder_id).toInt();
            note->noteType    = m_sqlQuery->value(VNoteItemOper::note_type).toInt();
            note->noteTitle   = m_sqlQuery->value(VNoteItemOper::note_title).toString();

            //Parse meta data
            QVariant metaData  = m_sqlQuery->value(VNoteItemOper::meta_data);
            note->setMetadata(metaData);
            metaParser.parse(metaData, note);

            note->noteState  = m_sqlQuery->value(VNoteItemOper::note_state).toInt();

            note->createTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteItemOper::create_time).toString(),VNOTE_TIME_FMT);
            note->modifyTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteItemOper::modify_time).toString(),VNOTE_TIME_FMT);
            note->deleteTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteItemOper::modify_time).toString(),VNOTE_TIME_FMT);

            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************

            break;
        }
    }

    return isOK;
}

bool AddNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *INSERT_FMT = "INSERT INTO %s (%s,%s,%s,%s) VALUES (%s,%s,'%s','%s');";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s,%s=%s WHERE %s=%s;";
    static constexpr char const *NEWREC_FMT = "SELECT * FROM %s WHERE %s=%s ORDER BY %s DESC LIMIT 1;";

    const VNoteFolder *folder = param.newNote->folder();
    const VNoteItem *note = param.newNote;

    if ((nullptr != note) && (nullptr != folder)) {
        QString insertSql;

        insertSql.sprintf(INSERT_FMT
                , VNoteDbManager::NOTES_TABLE_NAME
                , DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data()
                , DBNote::noteColumnsName[DBNote::note_type].toUtf8().data()
                , DBNote::noteColumnsName[DBNote::note_title].toUtf8().data()
                , DBNote::noteColumnsName[DBNote::meta_data].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                , QString("%1").arg(note->noteType).toUtf8().data()
                , note->noteTitle.toUtf8().data()
                , note->metaDataConstRef().toString().toUtf8().data()
                );

        QString updateSql;
        QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

        updateSql.sprintf(UPDATE_FOLDER_TIME
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::max_noteid].toUtf8().data()
                , QString("%1").arg(param.newNote->folder()->maxNoteIdRef()).toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data()
                , sqlGetTime.toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                );

        QString queryNewRec;
        queryNewRec.sprintf(NEWREC_FMT
                , VNoteDbManager::NOTES_TABLE_NAME
                , DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                , DBNote::noteColumnsName[DBNote::note_id].toUtf8().data()
                );

        m_dbvSqls.append(insertSql);
        m_dbvSqls.append(updateSql);
        m_dbvSqls.append(queryNewRec);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

RenameNoteDbVisitor::RenameNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool RenameNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *MODIFY_NOTETEXT_FMT = "UPDATE %s SET %s='%s', %s='%s' WHERE %s=%s AND %s=%s;";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s WHERE %s=%s;";

    const VNoteItem *note = param.newNote;

    if (nullptr != note) {
        QString sqlTitle = note->noteTitle;
        checkSqlStr(sqlTitle);

        QString modifyNoteTextSql;
        modifyNoteTextSql.sprintf(MODIFY_NOTETEXT_FMT
                , VNoteDbManager::NOTES_TABLE_NAME
                , DBNote::noteColumnsName[DBNote::note_title].toUtf8().data()
                , sqlTitle.toUtf8().data()
                , DBNote::noteColumnsName[DBNote::modify_time].toUtf8().data()
                , note->modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data()
                , DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                , DBNote::noteColumnsName[DBNote::note_id].toUtf8().data()
                , QString("%1").arg(note->noteId).toUtf8().data()
                );

        QString updateSql;
        QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

        updateSql.sprintf(UPDATE_FOLDER_TIME
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data()
                , sqlGetTime.toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                );


        m_dbvSqls.append(modifyNoteTextSql);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

UpdateNoteDbVisitor::UpdateNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool UpdateNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *MODIFY_NOTETEXT_FMT = "UPDATE %s SET %s='%s', %s='%s' WHERE %s=%s AND %s=%s;";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s WHERE %s=%s;";

    const VNoteItem *note = param.newNote;

    if (nullptr != note) {
        QString metaDataStr =note->metaDataConstRef().toString();
        checkSqlStr(metaDataStr);

        QString modifyNoteTextSql;
        modifyNoteTextSql.sprintf(MODIFY_NOTETEXT_FMT
                , VNoteDbManager::NOTES_TABLE_NAME
                , DBNote::noteColumnsName[DBNote::meta_data].toUtf8().data()
                , metaDataStr.toUtf8().data()
                , DBNote::noteColumnsName[DBNote::modify_time].toUtf8().data()
                , note->modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data()
                , DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                , DBNote::noteColumnsName[DBNote::note_id].toUtf8().data()
                , QString("%1").arg(note->noteId).toUtf8().data()
                );

        QString updateSql;
        QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

        updateSql.sprintf(UPDATE_FOLDER_TIME
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data()
                , sqlGetTime.toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                );

        m_dbvSqls.append(modifyNoteTextSql);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

DelNoteDbVisitor::DelNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool DelNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *DEL_NOTE_FMT = "DELETE FROM %s WHERE %s=%s AND %s=%s;";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s, %s=%s WHERE %s=%s;";

    const VNoteItem *note = param.newNote;

    if (nullptr != note && nullptr != note->folder()) {
        QString deleteSql;

        deleteSql.sprintf(DEL_NOTE_FMT
                , VNoteDbManager::NOTES_TABLE_NAME
                , DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                , DBNote::noteColumnsName[DBNote::note_id].toUtf8().data()
                , QString("%1").arg(note->noteId).toUtf8().data()
                );

        QString updateSql;
        QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

        updateSql.sprintf(UPDATE_FOLDER_TIME
                , VNoteDbManager::FOLDER_TABLE_NAME
                , DBFolder::folderColumnsName[DBFolder::max_noteid].toUtf8().data()
                , QString("%1").arg(note->folder()->maxNoteIdRef()).toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data()
                , sqlGetTime.toUtf8().data()
                , DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data()
                , QString("%1").arg(note->folderId).toUtf8().data()
                );

        m_dbvSqls.append(deleteSql);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

SaferQryDbVisitor::SaferQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool SaferQryDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.safetyDatas) {
        isOK = true;

        while(m_sqlQuery->next()) {
            VDataSafer dataSafer;
            //All data at the start moment are all exception safers.
            //Default safer type is ExceptionSafer.
            //dataSafer.setSaferType(VDataSafer::ExceptionSafer);

            dataSafer.id      = m_sqlQuery->value(VNoteSaferOper::id).toInt();
            dataSafer.folder_id = m_sqlQuery->value(VNoteSaferOper::folder_id).toInt();
            dataSafer.note_id = m_sqlQuery->value(VNoteSaferOper::note_id).toInt();
            dataSafer.path    = m_sqlQuery->value(VNoteSaferOper::path).toString();
            dataSafer.state   = m_sqlQuery->value(VNoteSaferOper::state).toInt();

            //Parse meta data
            QVariant metaData = m_sqlQuery->value(VNoteSaferOper::meta_data);
            dataSafer.meta_data = metaData.toString();

            dataSafer.createTime  = QDateTime::fromString(
                        m_sqlQuery->value(VNoteSaferOper::create_time).toString(),VNOTE_TIME_FMT);

            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************

            results.safetyDatas->push_back(dataSafer);
        }
    }

    return isOK;
}

bool SaferQryDbVisitor::prepareSqls()
{
    static constexpr char const *QUERY_FOLDERS_FMT = "SELECT * FROM %s ORDER BY %s DESC ;";

    QString querySql;
    querySql.sprintf(QUERY_FOLDERS_FMT
                     , VNoteDbManager::SAFER_TABLE_NAME
                     , DBSafer::saferColumnsName[DBSafer::create_time].toUtf8().data()
            );

    m_dbvSqls.append(querySql);

    return true;
}

AddSaferDbVisitor::AddSaferDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool AddSaferDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *INSERT_SAFER_FMT = "INSERT INTO %s (%s,%s,%s) VALUES (%s,%s,'%s');";

    if (nullptr != param.safer) {
        QString addSaferSql;
        addSaferSql.sprintf(INSERT_SAFER_FMT
                , VNoteDbManager::SAFER_TABLE_NAME
                , DBSafer::saferColumnsName[DBSafer::folder_id].toUtf8().data()
                , DBSafer::saferColumnsName[DBSafer::note_id].toUtf8().data()
                , DBSafer::saferColumnsName[DBSafer::path].toUtf8().data()
                , QString("%1").arg(param.safer->folder_id).toUtf8().data()
                , QString("%1").arg(param.safer->note_id).toUtf8().data()
                , param.safer->path.toUtf8().data()
                );

        m_dbvSqls.append(addSaferSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

DelSaferDbVisitor::DelSaferDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool DelSaferDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    static constexpr char const *DEL_SAFER_FMT = "DELETE FROM %s WHERE %s=%s AND %s=%s AND %s='%s';";

    if (nullptr != param.safer) {
        QString delSaferSql;
        delSaferSql.sprintf(DEL_SAFER_FMT
                , VNoteDbManager::SAFER_TABLE_NAME
                , DBSafer::saferColumnsName[DBSafer::folder_id].toUtf8().data()
                , QString("%1").arg(param.safer->folder_id).toUtf8().data()
                , DBSafer::saferColumnsName[DBSafer::note_id].toUtf8().data()
                , QString("%1").arg(param.safer->note_id).toUtf8().data()
                , DBSafer::saferColumnsName[DBSafer::path].toUtf8().data()
                , param.safer->path.toUtf8().data()
                );

        m_dbvSqls.append(delSaferSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

