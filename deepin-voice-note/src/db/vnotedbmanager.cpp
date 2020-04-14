#include "db/vnotedbmanager.h"
#include "db/dbvisitor.h"
#include "globaldef.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QSqlError>

#include <DLog>

#define CRITICAL_SECTION_BEGIN() do { \
    m_dbLock.lock(); \
    /*m_vnoteDB.transaction();*/ \
} while(0)

#define CRITICAL_SECTION_END() do { \
    /*m_vnoteDB.commit();*/ \
    m_dbLock.unlock(); \
} while(0)

#define CHECK_DB_INIT() do { \
    if (!m_isDbInitOK) { \
        return false; \
    } \
} while(0)

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

QSqlDatabase &VNoteDbManager::getVNoteDb()
{
    return m_vnoteDB;
}

bool VNoteDbManager::insertData(const QStringList& insertSql, DbVisitor* visitor)
{
    CHECK_DB_INIT();

    bool insertOK = true;

    if (nullptr == visitor) {
        qCritical() << "insertData invalid parameter: visitor is null";
        return false;
    }

    CRITICAL_SECTION_BEGIN();

    for (auto it : insertSql) {
        if (!it.trimmed().isEmpty()) {
            if(!visitor->sqlQuery()->exec(it)) {
                qCritical() << "insert data failed:" << it <<" reason:" << visitor->sqlQuery()->lastError().text();
                insertOK = false;
            }
        }
    }

    CRITICAL_SECTION_END();

    //Get new record data
    if (nullptr != visitor) {
        if (!visitor->visitorData()) {
            insertOK = false;
            qCritical() << "Query new data failed: visitorData failed.";
        }
    }

    return insertOK;
}

bool VNoteDbManager::updateData(const QStringList& updateSql)
{
    CHECK_DB_INIT();

    bool updateOK = true;

    CRITICAL_SECTION_BEGIN();

    QScopedPointer<QSqlQuery> sqlQuery(new QSqlQuery(m_vnoteDB));

    for (auto it : updateSql) {
        if (!it.trimmed().isEmpty()) {
            if(!sqlQuery->exec(it)) {
                qCritical() << "Update data failed:" << it <<" reason:" << sqlQuery->lastError().text();
                updateOK = false;
            }
        }
    }

    CRITICAL_SECTION_END();

    return updateOK;
}

bool VNoteDbManager::queryData(const QString querySql, DbVisitor* visitor)
{
    CHECK_DB_INIT();

    bool queryOK = true;

    if (nullptr == visitor) {
        qCritical() << "Need DbVisitor parameter but is null";
        return false;
    }

    CRITICAL_SECTION_BEGIN();

    if(!visitor->sqlQuery()->exec(querySql)) {
        qCritical() << "Query data failed:" << querySql <<" reason:" << visitor->sqlQuery()->lastError().text();
        queryOK = false;
    }

    CRITICAL_SECTION_END();

    if (!visitor->visitorData()) {
        qCritical() << "Query data failed: visitorData failed.";
        queryOK = false;
    }

    return queryOK;
}

bool VNoteDbManager::deleteData(const QStringList& delSql)
{
    CHECK_DB_INIT();

    bool deleteOK = true;

    CRITICAL_SECTION_BEGIN();

    QScopedPointer<QSqlQuery> sqlQuery(new QSqlQuery(m_vnoteDB));

    for (auto it : delSql) {
        if (!it.trimmed().isEmpty()) {
            if(!sqlQuery->exec(it)) {
                qCritical() << "Delete data failed:" << it <<" reason:" << sqlQuery->lastError().text();
                deleteOK = false;
            }
        }
    }

    CRITICAL_SECTION_END();

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

    QString vnoteDBName = DEEPIN_VOICE_NOTE
            + QString(VNoteDbManager::DBVERSION) + QString(".db");

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

    createTablesIfNeed();

    m_isDbInitOK = true;

    return 0;
}

void VNoteDbManager::createTablesIfNeed()
{
    QStringList createTableSqls = QString(CREATETABLE_FMT).split(";");

    QScopedPointer<QSqlQuery> sqlQuery(new QSqlQuery(m_vnoteDB));

    for (auto it: createTableSqls) {
        if (!it.trimmed().isEmpty()) {
            if(!sqlQuery->exec(it)) {
                qCritical() << it << "init tables failed error: " << sqlQuery->lastError().text();
            }
        }
    }
}
