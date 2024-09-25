// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbvisitor.h"
#include "globaldef.h"
#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"
#include "common/datatypedef.h"
#include "common/metadataparser.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
// #include "common/setting.h"

#include "db/vnotedbmanager.h"

// #include <DLog>

#include <QVariant>

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
    "expand_filed1", //使用扩展字段记录记事本数据是否已经加密
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
    "expand_filed1", //使用扩展字段记录笔记是否置顶
    "expand_filed2", //使用扩展字段记录笔记数据是否已经加密
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

/**
 * @brief DbVisitor::DbVisitor
 * @param db 数据库对象
 * @param inParam 参数
 * @param result 结果
 */
DbVisitor::DbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : m_sqlQuery(new QSqlQuery(db))
{
    param.ptr = inParam;
    results.ptr = result;
}

/**
 * @brief DbVisitor::~DbVisitor
 */
DbVisitor::~DbVisitor()
{
}

/**
 * @brief DbVisitor::visitorData
 * @return true 成功
 */
bool DbVisitor::visitorData()
{
    // Default do nothing
    return true;
}

/**
 * @brief DbVisitor::sqlQuery
 * @return 数据库对象
 */
QSqlQuery *DbVisitor::sqlQuery()
{
    return m_sqlQuery.get();
}

/**
 * @brief DbVisitor::dbvSqls
 * @return sql语句
 */
const QStringList &DbVisitor::dbvSqls()
{
    return m_dbvSqls;
}

/**
 * @brief DbVisitor::extraData
 * @return 扩展数据
 */
DbVisitor::ExtraData &DbVisitor::extraData()
{
    return m_extraData;
}

/**
 * @brief DbVisitor::checkSqlStr
 * @param sql
 */
void DbVisitor::checkSqlStr(QString &sql)
{
    sql.replace("'", "''");
}

/**
 * @brief FolderQryDbVisitor::FolderQryDbVisitor
 * @param db
 * @param inParam 参数
 * @param result 结果
 */
FolderQryDbVisitor::FolderQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief FolderQryDbVisitor::visitorData
 * @return true 成功
 */
bool FolderQryDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.folders) {
        isOK = true;

        while (m_sqlQuery->next()) {
            VNoteFolder *folder = new VNoteFolder();

            folder->id = m_sqlQuery->value(DBFolder::folder_id).toInt();
            folder->category = m_sqlQuery->value(DBFolder::category_id).toInt();
            QVariant folderName = m_sqlQuery->value(DBFolder::folder_name);
            folder->defaultIcon = m_sqlQuery->value(DBFolder::default_icon).toInt();
            folder->iconPath = m_sqlQuery->value(DBFolder::icon_path).toString();
            folder->folder_state = m_sqlQuery->value(DBFolder::folder_state).toInt();

            folder->maxNoteIdRef() = m_sqlQuery->value(DBFolder::max_noteid).toInt();

            folder->createTime = m_sqlQuery->value(DBFolder::create_time).toDateTime();
            folder->modifyTime = m_sqlQuery->value(DBFolder::modify_time).toDateTime();
            folder->deleteTime = m_sqlQuery->value(DBFolder::delete_time).toDateTime();
            folder->encryption = m_sqlQuery->value(DBFolder::encrypt).toInt();
            //查询时，如果是加密数据，则需要解密
            folder->name = folder->encryption ? QByteArray::fromBase64(folderName.toByteArray()) : folderName.toString();
            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************
            results.folders->folders.insert(folder->id, folder);
        }
    }

    return isOK;
}

/**
 * @brief FolderQryDbVisitor::prepareSqls
 * @return true 成功
 */
bool FolderQryDbVisitor::prepareSqls()
{
    QString querySql = QString("SELECT * FROM %1  ORDER BY %2 DESC ;").arg(VNoteDbManager::FOLDER_TABLE_NAME)
                           .arg(DBFolder::folderColumnsName[DBFolder::create_time].toUtf8().data());

    m_dbvSqls.append(querySql);

    return true;
}

/**
 * @brief NoteQryDbVisitor::NoteQryDbVisitor
 * @param db
 * @param inParam 参数
 * @param result 结果
 */
NoteQryDbVisitor::NoteQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief NoteQryDbVisitor::visitorData
 * @return true 成功
 */
bool NoteQryDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.notes) {
        isOK = true;

        MetaDataParser metaParser;

        while (m_sqlQuery->next()) {
            VNoteItem *note = new VNoteItem();

            note->encryption = m_sqlQuery->value(DBNote::encrypt).toInt();
            note->noteId = m_sqlQuery->value(DBNote::note_id).toInt();
            note->folderId = m_sqlQuery->value(DBNote::folder_id).toInt();
            note->noteType = m_sqlQuery->value(DBNote::note_type).toInt();
            QVariant noteTitle = m_sqlQuery->value(DBNote::note_title);
            //Parse meta data
            QVariant metaData = m_sqlQuery->value(DBNote::meta_data);

            //查询时，如果是加密数据，则需要解密
            if (note->encryption) {
                note->noteTitle = QByteArray::fromBase64(noteTitle.toByteArray());
                metaData = QByteArray::fromBase64(metaData.toByteArray());
            } else {
                note->noteTitle = noteTitle.toString();
            }

            note->setMetadata(metaData);
            metaParser.parse(metaData, note);

            note->noteState = m_sqlQuery->value(DBNote::note_state).toInt();

            note->createTime = m_sqlQuery->value(DBNote::create_time).toDateTime();
            note->modifyTime = m_sqlQuery->value(DBNote::modify_time).toDateTime();
            note->deleteTime = m_sqlQuery->value(DBNote::modify_time).toDateTime();

            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here
            note->isTop = m_sqlQuery->value(DBNote::is_top).toInt();
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
                VNOTE_ITEMS_MAP *folderNotes = new VNOTE_ITEMS_MAP();

                //DataManager data should set autoRelease flag
                folderNotes->autoRelease = true;

                folderNotes->folderNotes.insert(note->noteId, note);
                results.notes->notes.insert(note->folderId, folderNotes);
            }
        }
    }

    return isOK;
}

/**
 * @brief NoteQryDbVisitor::prepareSqls
 * @return 成功
 */
bool NoteQryDbVisitor::prepareSqls()
{
    QString querySql = QString("SELECT * FROM %1 ORDER BY %2;").arg(VNoteDbManager::NOTES_TABLE_NAME).arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data());

    m_dbvSqls.append(querySql);

    return true;
}

/**
 * @brief MaxIdFolderDbVisitor::MaxIdFolderDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
MaxIdFolderDbVisitor::MaxIdFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief MaxIdFolderDbVisitor::visitorData
 * @return true 成功
 */
bool MaxIdFolderDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.id) {
        isOK = true;

        while (m_sqlQuery->next()) {
            *results.id = m_sqlQuery->value(0).toInt();
            break;
        }
    }

    return isOK;
}

/**
 * @brief MaxIdFolderDbVisitor::prepareSqls
 * @return true 成功
 */
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
    QString querySql = QString("SELECT SEQ FROM SQLITE_SEQUENCE where NAME='%1';").arg(VNoteDbManager::FOLDER_TABLE_NAME);

    m_dbvSqls.append(querySql);

    if (m_extraData.data.flag) {
        QString resetFolderIdSql = QString("UPDATE SQLITE_SEQUENCE SET SEQ=%1 where NAME='%2';")
        .arg(QString("%1").arg(0).toUtf8().data())
        .arg(VNoteDbManager::FOLDER_TABLE_NAME);
        m_dbvSqls.append(resetFolderIdSql);
    }

    return true;
}

/**
 * @brief AddFolderDbVisitor::AddFolderDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
AddFolderDbVisitor::AddFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief AddFolderDbVisitor::visitorData
 * @return true 成功
 */
bool AddFolderDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.newFolder) {
        isOK = true;

        while (m_sqlQuery->next()) {
            results.newFolder->id = m_sqlQuery->value(DBFolder::folder_id).toInt();
            results.newFolder->category = m_sqlQuery->value(DBFolder::category_id).toInt();
            results.newFolder->name = m_sqlQuery->value(DBFolder::folder_name).toString();
            results.newFolder->defaultIcon = m_sqlQuery->value(DBFolder::default_icon).toInt();
            results.newFolder->iconPath = m_sqlQuery->value(DBFolder::icon_path).toString();
            results.newFolder->folder_state = m_sqlQuery->value(DBFolder::folder_state).toInt();
            results.newFolder->createTime = m_sqlQuery->value(DBFolder::create_time).toDateTime();
            results.newFolder->modifyTime = m_sqlQuery->value(DBFolder::modify_time).toDateTime();
            results.newFolder->deleteTime = m_sqlQuery->value(DBFolder::delete_time).toDateTime();
            results.newFolder->encryption = m_sqlQuery->value(DBFolder::encrypt).toInt();
            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************

            break;
        }
    }

    return isOK;
}

/**
 * @brief AddFolderDbVisitor::prepareSqls
 * @return true 成功
 */
bool AddFolderDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    if (nullptr != param.newFolder) {
        //Check&Init the create time parameter
        //create/modify/delete time are same for new folder
        QDateTime createTime = param.newFolder->createTime;
        if (createTime.isNull()) {
            createTime = QDateTime::currentDateTime();
        }

        QString insertSql = QString("INSERT INTO %1 (%2,%3,%4,%5,%6,%7) VALUES ('%8', %9, '%10', '%11', '%12', %13);")
        .arg(VNoteDbManager::FOLDER_TABLE_NAME)
        .arg(DBFolder::folderColumnsName[DBFolder::folder_name].toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::default_icon].toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::create_time].toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::delete_time].toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::encrypt].toUtf8().data())
        .arg(param.newFolder->name.toUtf8().data())
        .arg(param.newFolder->defaultIcon)
        .arg(createTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(createTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(createTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(0);

        QString queryNewRec = QString("SELECT * FROM %1 ORDER BY %2 DESC LIMIT 1;").arg(VNoteDbManager::FOLDER_TABLE_NAME).arg(DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data());

        m_dbvSqls.append(insertSql);
        m_dbvSqls.append(queryNewRec);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

/**
 * @brief RenameFolderDbVisitor::RenameFolderDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
RenameFolderDbVisitor::RenameFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief RenameFolderDbVisitor::prepareSqls
 * @return true 成功
 */
bool RenameFolderDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    const VNoteFolder *folder = param.newFolder;
    if (nullptr != folder) {
        QString sqlFolderName = folder->name;
        checkSqlStr(sqlFolderName);

        QString renameSql = QString("UPDATE %1 SET %2='%3', %4='%5' WHERE %6=%7;")
        .arg(VNoteDbManager::FOLDER_TABLE_NAME)
        .arg(DBFolder::folderColumnsName[DBFolder::folder_name].toUtf8().data())
        .arg(folder->encryption ? sqlFolderName.toLocal8Bit().toBase64().data() : sqlFolderName.toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data())
        .arg(folder->modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data())
        .arg(QString("%1").arg(folder->id).toUtf8().data());

        m_dbvSqls.append(renameSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

/**
 * @brief DelFolderDbVisitor::DelFolderDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
DelFolderDbVisitor::DelFolderDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief DelFolderDbVisitor::prepareSqls
 * @return true 成功
 */
bool DelFolderDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;

    if (nullptr != param.id) {
        qint64 folderId = *param.id;
        QString deleteFolderSql = QString("DELETE FROM %1 WHERE %2=%3;")
        .arg(VNoteDbManager::FOLDER_TABLE_NAME)
        .arg(DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data())
        .arg(QString("%1").arg(folderId).toUtf8().data());

        QString deleteNotesSql = QString("DELETE FROM %1 WHERE %2=%3;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(QString("%1").arg(folderId).toUtf8().data());

        m_dbvSqls.append(deleteFolderSql);
        m_dbvSqls.append(deleteNotesSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

/**
 * @brief AddNoteDbVisitor::AddNoteDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
AddNoteDbVisitor::AddNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief AddNoteDbVisitor::visitorData
 * @return true 成功
 */
bool AddNoteDbVisitor::visitorData()
{
    bool isOK = false;

    if (nullptr != results.newNote) {
        isOK = true;

        MetaDataParser metaParser;

        while (m_sqlQuery->next()) {
            VNoteItem *note = results.newNote;

            note->noteId = m_sqlQuery->value(DBNote::note_id).toInt();
            note->folderId = m_sqlQuery->value(DBNote::folder_id).toInt();
            note->noteType = m_sqlQuery->value(DBNote::note_type).toInt();
            note->encryption = m_sqlQuery->value(DBNote::encrypt).toInt();
            note->noteTitle = m_sqlQuery->value(DBNote::note_title).toString();
            //Parse meta data
            QVariant metaData = m_sqlQuery->value(DBNote::meta_data);

            note->setMetadata(metaData);
            metaParser.parse(metaData, note);

            note->noteState = m_sqlQuery->value(DBNote::note_state).toInt();

            note->createTime = m_sqlQuery->value(DBNote::create_time).toDateTime();
            note->modifyTime = m_sqlQuery->value(DBNote::modify_time).toDateTime();
            note->deleteTime = m_sqlQuery->value(DBNote::modify_time).toDateTime();

            //************Expand fileds begin**********
            //TODO:
            //    Add the expand fileds parse code here

            //************Expand fileds end************

            break;
        }
    }

    return isOK;
}

/**
 * @brief AddNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool AddNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    const VNoteFolder *folder = param.newNote->folder();
    const VNoteItem *note = param.newNote;

    if ((nullptr != note) && (nullptr != folder)) {
        //Check&Init the create time parameter
        //create/modify/delete time are same for new note
        QDateTime createTime = param.newNote->createTime;
        if (createTime.isNull()) {
            createTime = QDateTime::currentDateTime();
        }

        QString metaDataStr = note->metaDataConstRef().toString();
        checkSqlStr(metaDataStr);
        QString noteTitle = note->noteTitle;
        checkSqlStr(noteTitle);

        QString insertSql = QString("INSERT INTO %1 (%2,%3,%4,%5,%6,%7,%8,%9) VALUES (%10,%11,'%12','%13','%14','%15','%16',%17);")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::note_type].toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::note_title].toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::meta_data].toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::create_time].toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::modify_time].toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::delete_time].toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::encrypt].toUtf8().data())
        .arg(note->folderId)
        .arg(note->noteType)
        .arg(noteTitle.toUtf8().data())
        .arg(metaDataStr.toUtf8().data())
        .arg(createTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(createTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(createTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(0);

        QString updateSql = QString("UPDATE %1 SET %2=%3,%4='%5' WHERE %6=%7;")
        .arg(VNoteDbManager::FOLDER_TABLE_NAME)
        .arg(DBFolder::folderColumnsName[DBFolder::max_noteid].toUtf8().data())
        .arg(QString("%1").arg(param.newNote->folder()->maxNoteIdRef()).toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data())
        .arg(createTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data())
        .arg(QString("%1").arg(note->folderId).toUtf8().data());

        QString queryNewRec = QString("SELECT * FROM %1 WHERE %2=%3 ORDER BY %4 DESC LIMIT 1;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(QString("%1").arg(note->folderId).toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data());

        m_dbvSqls.append(insertSql);
        m_dbvSqls.append(updateSql);
        m_dbvSqls.append(queryNewRec);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

/**
 * @brief RenameNoteDbVisitor::RenameNoteDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
RenameNoteDbVisitor::RenameNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief RenameNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool RenameNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;

    if (nullptr != note) {
        QString sqlTitle = note->noteTitle;
        checkSqlStr(sqlTitle);

        QString modifyNoteTextSql = QString("UPDATE %1 SET %2='%3', %4='%5' WHERE %6=%7 AND %8=%9;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::note_title].toUtf8().data())
        .arg(note->encryption ? sqlTitle.toLocal8Bit().toBase64().data() : sqlTitle.toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::modify_time].toUtf8().data())
        .arg(note->modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(note->folderId)
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data())
        .arg(note->noteId);

        QDateTime modifyTime = QDateTime::currentDateTime();
        QString updateSql = QString("UPDATE %1 SET %2='%3' WHERE %4=%5;")
        .arg(VNoteDbManager::FOLDER_TABLE_NAME)
        .arg(DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data())
        .arg(modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data())
        .arg(QString("%1").arg(note->folderId).toUtf8().data());

        m_dbvSqls.append(modifyNoteTextSql);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

/**
 * @brief UpdateNoteDbVisitor::UpdateNoteDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
UpdateNoteDbVisitor::UpdateNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief UpdateNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool UpdateNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;

    if (nullptr != note) {
        QString metaDataStr = note->metaDataConstRef().toString();
        checkSqlStr(metaDataStr);

        QString modifyNoteTextSql = QString("UPDATE %1 SET %2='%3', %4='%5' WHERE %6=%7 AND %8=%9;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::meta_data].toUtf8().data())
        .arg(note->encryption ? metaDataStr.toLocal8Bit().toBase64().data() : metaDataStr.toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::modify_time].toUtf8().data())
        .arg(note->modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(note->folderId)
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data())
        .arg(note->noteId);

        QDateTime modifyTime = QDateTime::currentDateTime();
        QString updateSql = QString("UPDATE %1 SET %2='%3' WHERE %4=%5;")
        .arg(VNoteDbManager::FOLDER_TABLE_NAME)
        .arg(DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data())
        .arg(modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data())
        .arg(QString("%1").arg(note->folderId).toUtf8().data());

        m_dbvSqls.append(modifyNoteTextSql);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}

/**
 * @brief UpdateNoteTopDbVisitor::UpdateNoteTopDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
UpdateNoteTopDbVisitor::UpdateNoteTopDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief UpdateNoteDbVisitor::prepareSqls
 */
bool UpdateNoteTopDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;
    if (note != nullptr) {
        QString updateSql = QString("UPDATE %1 SET %2=%3 WHERE %4=%5;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::is_top].toUtf8().data())
        .arg(note->isTop)
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data())
        .arg(note->noteId);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }
    return fPrepareOK;
}

/**
 * @brief UpdateNoteFolderIdDbVisitor::UpdateNoteFolderIdDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
UpdateNoteFolderIdDbVisitor::UpdateNoteFolderIdDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief UpdateNoteFolderIdDbVisitor::prepareSqls
 * @return
 */
bool UpdateNoteFolderIdDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;
    if (note != nullptr) {
        QString updateSql = QString("UPDATE %1 SET %2=%3 WHERE %4=%5;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(note->folderId)
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data())
        .arg(note->noteId);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }
    return fPrepareOK;
}

/**
 * @brief DelNoteDbVisitor::DelNoteDbVisitor
 * @param db
 * @param inParam
 * @param result
 */
DelNoteDbVisitor::DelNoteDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    : DbVisitor(db, inParam, result)
{
}

/**
 * @brief DelNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool DelNoteDbVisitor::prepareSqls()
{
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;
    if (nullptr != note && nullptr != note->folder()) {
        QString deleteSql = QString("DELETE FROM %1 WHERE %2=%3 AND %4=%5;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(QString("%1").arg(note->folderId).toUtf8().data())
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data())
        .arg(QString("%1").arg(note->noteId).toUtf8().data());

        QDateTime modifyTime = QDateTime::currentDateTime();
        QString updateSql = QString("UPDATE %1 SET %2=%3, %4='%5' WHERE %6=%7;")
        .arg(VNoteDbManager::FOLDER_TABLE_NAME)
        .arg(DBFolder::folderColumnsName[DBFolder::max_noteid].toUtf8().data())
        .arg(QString("%1").arg(note->folder()->maxNoteIdRef()).toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::modify_time].toUtf8().data())
        .arg(modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data())
        .arg(DBFolder::folderColumnsName[DBFolder::folder_id].toUtf8().data())
        .arg(QString("%1").arg(note->folderId).toUtf8().data());

        m_dbvSqls.append(deleteSql);
        m_dbvSqls.append(updateSql);
    } else {
        fPrepareOK = false;
    }

    return fPrepareOK;
}
