#include "db/vnotedbmanager.h"
#include "globaldef.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QSqlError>

#include <DLog>

VNoteDbManager* VNoteDbManager::_instance = nullptr;

VNoteDbManager::VNoteDbManager(QObject *parent)
    : QObject(parent)
{
    initVNoteDb();
}

VNoteDbManager::~VNoteDbManager()
{
    m_vnoteDB.close();
}

VNoteDbManager *VNoteDbManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteDbManager();
    }

    return  _instance;
}

bool VNoteDbManager::insertData(DB_TABLE table,
                                QString insertSql,
                                QVariantList& record )
{
    int colums = 0;

    bool insertOK = true;

    switch (table) {
    case VNOTE_FOLDER_TBL: {
        colums    = FOLDER_COLUMS;
    } break;
    case VNOTE_ITEM_TBL: {
        colums    = NOTES_COLUMS;
    } break;
    default:
        break;
    };

    if (colums == 0) {
        qCritical() << "insertData invalid parameter table:" << table;
        return false;
    }

    m_rwDbLock.lockForWrite();

    //Begin Transaction
    m_vnoteDB.transaction();

    QStringList sqls = insertSql.split(";");

    for (auto it : sqls) {
        if (!it.trimmed().isEmpty()) {
            if(!m_sqlQuery->exec(it)) {
                qCritical() << "insert data failed:" << it <<" reason:" << m_sqlQuery->lastError().text();
                insertOK = false;
            }
        }
    }

    //Get new record data
    while(m_sqlQuery->next()) {

        for (int index=0; index<colums; index++) {
            record.append(m_sqlQuery->value(index));
        }
        break;
    }

    //If insert&query new record ok,commit
    //db, else rollback
    if (insertOK) {
        m_vnoteDB.commit();
    } else {
        m_vnoteDB.rollback();
    }

    m_rwDbLock.unlock();

    return insertOK;
}

bool VNoteDbManager::updateData(VNoteDbManager::DB_TABLE table, QString updateSql)
{
    Q_UNUSED(table);

    bool updateOK = true;

    m_rwDbLock.lockForWrite();

    if(!m_sqlQuery->exec(updateSql)) {
        qCritical() << "Update data failed:" << updateSql <<" reason:" << m_sqlQuery->lastError().text();
        updateOK = false;
    }

    m_rwDbLock.unlock();

    return updateOK;
}

bool VNoteDbManager::queryData(VNoteDbManager::DB_TABLE table, QString querySql, QList<QList<QVariant> > &result)
{
    int colums = 0;

    bool queryOK = true;

    m_rwDbLock.lockForWrite();

    //Begin Transaction
    m_vnoteDB.transaction();

    switch (table) {
    case VNOTE_FOLDER_TBL: {
        colums    = FOLDER_COLUMS;
    } break;
    case VNOTE_ITEM_TBL: {
        colums    = NOTES_COLUMS;
    } break;
    default:
        break;
    };

    if (colums == 0) {
        qCritical() << "InsertData invalid parameter table:" << table;
        return false;
    }

    if(!m_sqlQuery->exec(querySql)) {
        qCritical() << "Query data failed:" << querySql <<" reason:" << m_sqlQuery->lastError().text();
        queryOK = false;
    }

    //Begin Transaction
    m_vnoteDB.commit();

    while(m_sqlQuery->next()) {

        QList<QVariant> record;
        for (int index=0; index<colums; index++) {
            record.append(m_sqlQuery->value(index));
        }

        result.append(record);
    }

    m_rwDbLock.unlock();

    return queryOK;
}

bool VNoteDbManager::deleteData(VNoteDbManager::DB_TABLE table, QString delSql)
{
    Q_UNUSED(table);

    bool deleteOK = true;

    m_rwDbLock.lockForWrite();

    if(!m_sqlQuery->exec(delSql)) {
        qCritical() << "Delete data failed:" << delSql <<" reason:" << m_sqlQuery->lastError().text();
        deleteOK = false;
    }

    m_rwDbLock.unlock();

    return deleteOK;
}

int VNoteDbManager::initVNoteDb()
{
    QString vnoteDatabasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QFileInfo dbDir(vnoteDatabasePath);

    //TODO:
    //    Remove the old version database
    //make a app owne directory
    if (!dbDir.isDir() && dbDir.exists()) {
        QFile oldDbFile(vnoteDatabasePath);
        if (!oldDbFile.remove()) {
            qInfo() << oldDbFile.fileName() << ":" <<oldDbFile.errorString();
        }
    }

    dbDir.setFile(vnoteDatabasePath+QDir::separator());

    if (!dbDir.exists()) {
        QDir(dbDir.filePath()).mkdir(dbDir.filePath());
        qInfo() << "Create vnote db directory:" << vnoteDatabasePath;
    }

    QString vnoteDBName = DEEPIN_VOICE_NOTE + QString(".db");

    QString vnoteDbFullPath = dbDir.filePath() + vnoteDBName;

    if (QSqlDatabase::contains(DEEPIN_VOICE_NOTE)) {
        m_vnoteDB = QSqlDatabase::database(DEEPIN_VOICE_NOTE);
    } else {
        m_vnoteDB = QSqlDatabase::addDatabase("QSQLITE", DEEPIN_VOICE_NOTE);
        m_vnoteDB.setDatabaseName(vnoteDbFullPath);
    }



    if (!m_vnoteDB.open()) {
        qCritical() << "Open database failded:" << m_vnoteDB.lastError().text();

        return -1;
    }

    qInfo() << "Database opened:" << vnoteDbFullPath;

    m_sqlQuery.reset(new QSqlQuery(m_vnoteDB));

    createTablesIfNeed();

    return 0;
}

void VNoteDbManager::createTablesIfNeed()
{
    QStringList createTableSqls = QString(CREATETABLE_FMT).split(";");

    for (auto it: createTableSqls) {
        if (!it.trimmed().isEmpty()) {
            if(!m_sqlQuery->exec(it)) {
                qCritical() << it << "init tables failed error: " << m_sqlQuery->lastError().text();
            }
        }
    }
}
