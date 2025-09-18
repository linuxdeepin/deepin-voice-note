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
#include <QDebug>

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
    // qInfo() << "DbVisitor constructor called";
    param.ptr = inParam;
    results.ptr = result;
}

/**
 * @brief DbVisitor::~DbVisitor
 */
DbVisitor::~DbVisitor()
{
    // qInfo() << "DbVisitor destructor called";
}

/**
 * @brief DbVisitor::visitorData
 * @return true 成功
 */
bool DbVisitor::visitorData()
{
    // qInfo() << "Visiting data";
    // Default do nothing
    return true;
}

/**
 * @brief DbVisitor::sqlQuery
 * @return 数据库对象
 */
QSqlQuery *DbVisitor::sqlQuery()
{
    // qInfo() << "Getting SQL query object";
    return m_sqlQuery.get();
}

/**
 * @brief DbVisitor::dbvSqls
 * @return sql语句
 */
const QStringList &DbVisitor::dbvSqls()
{
    // qInfo() << "Getting database SQL statements";
    return m_dbvSqls;
}

/**
 * @brief DbVisitor::extraData
 * @return 扩展数据
 */
DbVisitor::ExtraData &DbVisitor::extraData()
{
    // qInfo() << "Getting extra data";
    return m_extraData;
}

/**
 * @brief DbVisitor::checkSqlStr
 * @param sql
 */
void DbVisitor::checkSqlStr(QString &sql)
{
    // qInfo() << "Checking SQL string";
    sql.replace("'", "''");
    // qInfo() << "SQL string check finished";
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
    // qInfo() << "FolderQryDbVisitor constructor called";
}

/**
 * @brief FolderQryDbVisitor::visitorData
 * @return true 成功
 */
bool FolderQryDbVisitor::visitorData()
{
    // qInfo() << "Visiting folder query data";
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
            qDebug() << "Found folder:" << folder->name << "ID:" << folder->id;
            results.folders->folders.insert(folder->id, folder);
        }
    }

    // qInfo() << "Folder query data visit finished, result:" << isOK;
    return isOK;
}

/**
 * @brief FolderQryDbVisitor::prepareSqls
 * @return true 成功
 */
bool FolderQryDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing der query SQL statements";
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
    // qInfo() << "NoteQryDbVisitor constructor called";
}

/**
 * @brief NoteQryDbVisitor::visitorData
 * @return true 成功
 */
bool NoteQryDbVisitor::visitorData()
{
    // qInfo() << "Visiting note query data";
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
                qDebug() << "Created new folder notes map for folder:" << note->folderId;
            }
        }
    }

    // qInfo() << "Note query data visit finished, result:" << isOK;
    return isOK;
}

/**
 * @brief NoteQryDbVisitor::prepareSqls
 * @return 成功
 */
bool NoteQryDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing note query SQL statements";
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
    // qInfo() << "MaxIdFolderDbVisitor constructor called";
}

/**
 * @brief MaxIdFolderDbVisitor::visitorData
 * @return true 成功
 */
bool MaxIdFolderDbVisitor::visitorData()
{
    // qInfo() << "Visiting max ID folder data";
    bool isOK = false;

    if (nullptr != results.id) {
        isOK = true;

        while (m_sqlQuery->next()) {
            *results.id = m_sqlQuery->value(0).toInt();
            break;
        }
    }

    // qInfo() << "Max ID folder data visit finished, result:" << isOK;
    return isOK;
}

/**
 * @brief MaxIdFolderDbVisitor::prepareSqls
 * @return true 成功
 */
bool MaxIdFolderDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing max ID folder SQL statements";
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
    // qInfo() << "AddFolderDbVisitor constructor called";
}

/**
 * @brief AddFolderDbVisitor::visitorData
 * @return true 成功
 */
bool AddFolderDbVisitor::visitorData()
{
    // qInfo() << "Visiting add folder data";
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

            qDebug() << "Added new folder:" << results.newFolder->name << "ID:" << results.newFolder->id;
            break;
        }
    }

    // qInfo() << "Add folder data visit finished, result:" << isOK;
    return isOK;
}

/**
 * @brief AddFolderDbVisitor::prepareSqls
 * @return true 成功
 */
bool AddFolderDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing add folder SQL statements";
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

    // qInfo() << "Add folder SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "RenameFolderDbVisitor constructor called";
}

/**
 * @brief RenameFolderDbVisitor::prepareSqls
 * @return true 成功
 */
bool RenameFolderDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing rename folder SQL statements";
    bool fPrepareOK = true;
    const VNoteFolder *folder = param.newFolder;
    if (nullptr != folder) {
        qDebug() << "Preparing rename folder SQL for folder:" << folder->name << "ID:" << folder->id;
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
        qWarning() << "Failed to prepare rename folder SQL: Invalid folder";
        fPrepareOK = false;
    }

    // qInfo() << "Rename folder SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "DelFolderDbVisitor constructor called";
}

/**
 * @brief DelFolderDbVisitor::prepareSqls
 * @return true 成功
 */
bool DelFolderDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing delete folder SQL statements";
    bool fPrepareOK = true;

    if (nullptr != param.id) {
        qint64 folderId = *param.id;
        qDebug() << "Preparing delete folder SQL for folder ID:" << folderId;

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
        qWarning() << "Failed to prepare delete folder SQL: Invalid folder ID";
        fPrepareOK = false;
    }

    // qInfo() << "Delete folder SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "AddNoteDbVisitor constructor called";
}

/**
 * @brief AddNoteDbVisitor::visitorData
 * @return true 成功
 */
bool AddNoteDbVisitor::visitorData()
{
    // qInfo() << "Visiting add note data";
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

            qDebug() << "Added new note:" << note->noteTitle << "ID:" << note->noteId << "in folder:" << note->folderId;
            break;
        }
    }

    // qInfo() << "Add note data visit finished, result:" << isOK;
    return isOK;
}

/**
 * @brief AddNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool AddNoteDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing add note SQL statements";
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

    // qInfo() << "Add note SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "RenameNoteDbVisitor constructor called";
}

/**
 * @brief RenameNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool RenameNoteDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing rename note SQL statements";
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;

    if (nullptr != note) {
        qDebug() << "Preparing rename note SQL for note:" << note->noteTitle << "ID:" << note->noteId;
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
        qWarning() << "Failed to prepare rename note SQL: Invalid note";
        fPrepareOK = false;
    }

    // qInfo() << "Rename note SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "UpdateNoteDbVisitor constructor called";
}

/**
 * @brief UpdateNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool UpdateNoteDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing update note SQL statements";
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;

    if (nullptr != note) {
        qDebug() << "Preparing update note SQL for note:" << note->noteTitle << "ID:" << note->noteId;
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
        qWarning() << "Failed to prepare update note SQL: Invalid note";
        fPrepareOK = false;
    }

    // qInfo() << "Update note SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "UpdateNoteTopDbVisitor constructor called";
}

/**
 * @brief UpdateNoteDbVisitor::prepareSqls
 */
bool UpdateNoteTopDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing update note top status SQL statements";
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;
    if (note != nullptr) {
        qDebug() << "Preparing update note top status SQL for note:" << note->noteTitle << "ID:" << note->noteId << "top:" << note->isTop;
        QString updateSql = QString("UPDATE %1 SET %2=%3 WHERE %4=%5;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::is_top].toUtf8().data())
        .arg(note->isTop)
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data())
        .arg(note->noteId);
        m_dbvSqls.append(updateSql);
    } else {
        qWarning() << "Failed to prepare update note top status SQL: Invalid note";
        fPrepareOK = false;
    }
    // qInfo() << "Update note top status SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "UpdateNoteFolderIdDbVisitor constructor called";
}

/**
 * @brief UpdateNoteFolderIdDbVisitor::prepareSqls
 * @return
 */
bool UpdateNoteFolderIdDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing update note folder ID SQL statements";
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;
    if (note != nullptr) {
        qDebug() << "Preparing update note folder ID SQL for note:" << note->noteTitle << "ID:" << note->noteId << "new folder ID:" << note->folderId;
        QString updateSql = QString("UPDATE %1 SET %2=%3 WHERE %4=%5;")
        .arg(VNoteDbManager::NOTES_TABLE_NAME)
        .arg(DBNote::noteColumnsName[DBNote::folder_id].toUtf8().data())
        .arg(note->folderId)
        .arg(DBNote::noteColumnsName[DBNote::note_id].toUtf8().data())
        .arg(note->noteId);
        m_dbvSqls.append(updateSql);
    } else {
        qWarning() << "Failed to prepare update note folder ID SQL: Invalid note";
        fPrepareOK = false;
    }
    // qInfo() << "Update note folder ID SQL statements preparation finished, result:" << fPrepareOK;
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
    // qInfo() << "DelNoteDbVisitor constructor called";
}

/**
 * @brief DelNoteDbVisitor::prepareSqls
 * @return true 成功
 */
bool DelNoteDbVisitor::prepareSqls()
{
    // qInfo() << "Preparing delete note SQL statements";
    bool fPrepareOK = true;
    const VNoteItem *note = param.newNote;
    if (nullptr != note && nullptr != note->folder()) {
        qDebug() << "Preparing delete note SQL for note:" << note->noteTitle << "ID:" << note->noteId << "in folder:" << note->folderId;
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
        qWarning() << "Failed to prepare delete note SQL: Invalid note or folder";
        fPrepareOK = false;
    }

    // qInfo() << "Delete note SQL statements preparation finished, result:" << fPrepareOK;
    return fPrepareOK;
}
