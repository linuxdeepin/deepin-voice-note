#include "vnoteitemoper.h"
#include "vnotefolderoper.h"
#include "vnotedbmanager.h"
#include "globaldef.h"
#include "common/metadataparser.h"
#include "common/vnoteitem.h"
#include "common/vnotedatamanager.h"
#include "db/dbvisitor.h"

#include <DLog>

const QStringList VNoteItemOper::noteColumnsName = {
    "note_id",
    "folder_id",
    "note_type",
    "note_title",
    "meta_data",
    "note_state"
    "create_time",
    "modify_time",
    "delete_time"
};

VNoteItemOper::VNoteItemOper(VNoteItem* note)
    :m_note(note)
{
}

VNOTE_ALL_NOTES_MAP *VNoteItemOper::loadAllVNotes()
{
    static constexpr char const *QUERY_NOTES_FMT = "SELECT * FROM %s ORDER BY %s;";

    QString querySql;
    querySql.sprintf(QUERY_NOTES_FMT
                     , VNoteDbManager::NOTES_TABLE_NAME
                     , noteColumnsName[folder_id].toUtf8().data()
                     );

    VNOTE_ALL_NOTES_MAP * notesMap = new VNOTE_ALL_NOTES_MAP();

    //DataManager data should set autoRelease flag
    notesMap->autoRelease = true;

    static struct timeval start,backups, end;

    gettimeofday(&start, nullptr);
    backups = start;

    NoteQryDbVisitor noteVisitor(VNoteDbManager::instance()->getVNoteDb(), notesMap);

    if (VNoteDbManager::instance()->queryData(querySql, &noteVisitor) ) {
        gettimeofday(&end, nullptr);

        qDebug() << "queryData(ms):" << TM(start, end);
    }

    return notesMap;
}

bool VNoteItemOper::modifyNoteTitle(QString title)
{
    static constexpr char const *MODIFY_NOTETEXT_FMT = "UPDATE %s SET %s='%s', %s='%s' WHERE %s=%s AND %s=%s;";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s WHERE %s=%s;";

    bool isUpdateOK = false;

    QString modifyNoteTextSql;

    if (nullptr != m_note) {
        QDateTime modifyTime = QDateTime::currentDateTime();

        modifyNoteTextSql.sprintf(MODIFY_NOTETEXT_FMT
                          , VNoteDbManager::NOTES_TABLE_NAME
                          , noteColumnsName[note_title].toUtf8().data()
                          , title.toUtf8().data()
                          , noteColumnsName[modify_time].toUtf8().data()
                          , modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data()
                          , noteColumnsName[folder_id].toUtf8().data()
                          , QString("%1").arg(m_note->folderId).toUtf8().data()
                          , noteColumnsName[note_id].toUtf8().data()
                          , QString("%1").arg(m_note->noteId).toUtf8().data()
                          );

        QString updateSql;
        QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

        updateSql.sprintf(UPDATE_FOLDER_TIME
                          , VNoteDbManager::FOLDER_TABLE_NAME
                          , VNoteFolderOper::folderColumnsName[VNoteFolderOper::modify_time].toUtf8().data()
                          , sqlGetTime.toUtf8().data()
                          , VNoteFolderOper::folderColumnsName[VNoteFolderOper::folder_id].toUtf8().data()
                          , QString("%1").arg(m_note->folderId).toUtf8().data()
                          );

        QStringList sqls;
        sqls.append(modifyNoteTextSql);
        sqls.append(updateSql);

        if (VNoteDbManager::instance()->updateData(sqls)) {
            m_note->noteTitle  = title;
            m_note->modifyTime = modifyTime;

            isUpdateOK = true;
        }
    }

    return isUpdateOK;
}

bool VNoteItemOper::updateNote()
{
    static constexpr char const *MODIFY_NOTETEXT_FMT = "UPDATE %s SET %s='%s', %s='%s' WHERE %s=%s AND %s=%s;";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s WHERE %s=%s;";

    bool isUpdateOK = false;

    QString modifyNoteTextSql;

    if (nullptr != m_note) {
        //Prepare meta data
        MetaDataParser metaParser;
        QVariant metaData;
        metaParser.makeMetaData(m_note->datas, metaData);

        QDateTime modifyTime = QDateTime::currentDateTime();

        modifyNoteTextSql.sprintf(MODIFY_NOTETEXT_FMT
                          , VNoteDbManager::NOTES_TABLE_NAME
                          , noteColumnsName[meta_data].toUtf8().data()
                          , metaData.toString().toUtf8().data()
                          , noteColumnsName[modify_time].toUtf8().data()
                          , modifyTime.toString(VNOTE_TIME_FMT).toUtf8().data()
                          , noteColumnsName[folder_id].toUtf8().data()
                          , QString("%1").arg(m_note->folderId).toUtf8().data()
                          , noteColumnsName[note_id].toUtf8().data()
                          , QString("%1").arg(m_note->noteId).toUtf8().data()
                          );

        QString updateSql;
        QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

        updateSql.sprintf(UPDATE_FOLDER_TIME
                          , VNoteDbManager::FOLDER_TABLE_NAME
                          , VNoteFolderOper::folderColumnsName[VNoteFolderOper::modify_time].toUtf8().data()
                          , sqlGetTime.toUtf8().data()
                          , VNoteFolderOper::folderColumnsName[VNoteFolderOper::folder_id].toUtf8().data()
                          , QString("%1").arg(m_note->folderId).toUtf8().data()
                          );

        QStringList sqls;
        sqls.append(modifyNoteTextSql);
        sqls.append(updateSql);

        if (VNoteDbManager::instance()->updateData(sqls)) {
            m_note->setMetadata(metaData);
            m_note->modifyTime = modifyTime;

            isUpdateOK = true;
        }
    }

    return isUpdateOK;
}

VNoteItem *VNoteItemOper::addNote(VNoteItem &note)
{
    static constexpr char const *INSERT_FMT = "INSERT INTO %s (%s,%s,%s,%s) VALUES (%s,%s,'%s','%s');";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s WHERE %s=%s;";
    static constexpr char const *NEWREC_FMT = "SELECT * FROM %s WHERE %s=%s ORDER BY %s DESC LIMIT 1;";

    //Prepare meta data
    MetaDataParser metaParser;
    QVariant metaData;
    metaParser.makeMetaData(note.datas, metaData);

    QString insertSql;

    insertSql.sprintf(INSERT_FMT
                      , VNoteDbManager::NOTES_TABLE_NAME
                      , noteColumnsName[folder_id].toUtf8().data()
                      , noteColumnsName[note_type].toUtf8().data()
                      , noteColumnsName[note_title].toUtf8().data()
                      , noteColumnsName[meta_data].toUtf8().data()
                      , QString("%1").arg(note.folderId).toUtf8().data()
                      , QString("%1").arg(note.noteType).toUtf8().data()
                      , note.noteTitle.toUtf8().data()
                      , metaData.toString().toUtf8().data()
                      );

    QString updateSql;
    QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

    updateSql.sprintf(UPDATE_FOLDER_TIME
                      , VNoteDbManager::FOLDER_TABLE_NAME
                      , VNoteFolderOper::folderColumnsName[VNoteFolderOper::modify_time].toUtf8().data()
                      , sqlGetTime.toUtf8().data()
                      , VNoteFolderOper::folderColumnsName[VNoteFolderOper::folder_id].toUtf8().data()
                      , QString("%1").arg(note.folderId).toUtf8().data()
                      );

    QString queryNewRec;
    queryNewRec.sprintf(NEWREC_FMT
                      , VNoteDbManager::NOTES_TABLE_NAME
                      , noteColumnsName[folder_id].toUtf8().data()
                      , QString("%1").arg(note.folderId).toUtf8().data()
                      , noteColumnsName[note_id].toUtf8().data()
                      );

    QStringList sqls;
    sqls.append(insertSql);
    sqls.append(updateSql);
    sqls.append(queryNewRec);

    VNoteItem *newNote= new VNoteItem();
    AddNoteDbVisitor addNoteVisitor(VNoteDbManager::instance()->getVNoteDb(), newNote);

    if (VNoteDbManager::instance()->insertData(sqls, &addNoteVisitor) ) {

        if (Q_UNLIKELY(nullptr == VNoteDataManager::instance()->addNote(newNote))) {
            qInfo() << "Add to datamanager failed:"
                    << "New Note:"  << newNote->noteId
                    << "Folder ID:" << newNote->folderId
                    << "Note type:" << newNote->noteType
                    << "Create time:" << newNote->createTime
                    << "Modify time:" << newNote->modifyTime;
            //Add to DataManager failed, release it
            QScopedPointer<VNoteItem> autoRelease(newNote);
            newNote = nullptr;
        }
    } else {
        qCritical()  << "New Note:"  << newNote->noteId
                     << "Folder ID:" << newNote->folderId
                     << "Note type:" << newNote->noteType
                     << "Create time:" << newNote->createTime
                     << "Modify time:" << newNote->modifyTime;

        QScopedPointer<VNoteItem> autoRelease(newNote);
        newNote = nullptr;
    }

    return newNote;
}

VNoteItem *VNoteItemOper::getNote(qint64 folderId, qint32 noteId)
{
    return VNoteDataManager::instance()->getNote(folderId, noteId);
}

VNOTE_ITEMS_MAP *VNoteItemOper::getFolderNotes(qint64 folderId)
{
    return VNoteDataManager::instance()->getFolderNotes(folderId);
}

bool VNoteItemOper::deleteNote()
{
    bool delOK = true;

    if (nullptr != m_note) {
        delOK = deleteNote(m_note->folderId, m_note->noteId);
    }

    return delOK;
}

bool VNoteItemOper::deleteNote(qint64 folderId, qint32 noteId)
{
    static constexpr char const *DEL_NOTE_FMT = "DELETE FROM %s WHERE %s=%s AND %s=%s;";
    static constexpr char const *UPDATE_FOLDER_TIME = "UPDATE %s SET %s=%s WHERE %s=%s;";

    QString deleteSql;

    deleteSql.sprintf(DEL_NOTE_FMT
                      , VNoteDbManager::NOTES_TABLE_NAME
                      , noteColumnsName[folder_id].toUtf8().data()
                      , QString("%1").arg(folderId).toUtf8().data()
                      , noteColumnsName[note_id].toUtf8().data()
                      , QString("%1").arg(noteId).toUtf8().data()
                      );

    QString updateSql;
    QString sqlGetTime("STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')");

    updateSql.sprintf(UPDATE_FOLDER_TIME
                      , VNoteDbManager::FOLDER_TABLE_NAME
                      , VNoteFolderOper::folderColumnsName[VNoteFolderOper::modify_time].toUtf8().data()
                      , sqlGetTime.toUtf8().data()
                      , VNoteFolderOper::folderColumnsName[VNoteFolderOper::folder_id].toUtf8().data()
                      , QString("%1").arg(folderId).toUtf8().data()
                      );

    QStringList sqls;
    sqls.append(deleteSql);
    sqls.append(updateSql);

    bool delOK = false;

    if (VNoteDbManager::instance()->deleteData(sqls) ) {
        //Release note Object
        QScopedPointer<VNoteItem>autoRelease(VNoteDataManager::instance()->delNote(folderId, noteId));

        delOK = true;
    }

    return  delOK;
}
