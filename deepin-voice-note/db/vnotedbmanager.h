#ifndef VNOTEDBMANAGER_H
#define VNOTEDBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMutex>

class DbVisitor;

class VNoteDbManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDbManager(QObject *parent = nullptr);
    virtual ~VNoteDbManager();

    static VNoteDbManager* instance();

    static constexpr char const *FOLDER_TABLE_NAME = "vnote_folder_tbl";
    static constexpr char const *FOLDER_KEY        = "folder_id";
    static constexpr int         FOLDER_COLUMS     = 7;
    static constexpr char const *NOTES_TABLE_NAME = "vnote_items_tbl";
    static constexpr char const *NOTES_KEY        = "note_id";
    static constexpr int         NOTES_COLUMS     = 7;

    //icon_path: Not used, maybe used in future
    static constexpr char const *CREATETABLE_FMT = "\
         CREATE TABLE IF NOT EXISTS vnote_folder_tbl(\
            folder_id INTEGER PRIMARY KEY AUTOINCREMENT , \
            folder_name TEXT NOT NULL, \
            default_icon INT DEFAULT 0, \
            icon_path TEXT,\
            note_count INT DEFAULT 0, \
            create_time DATETIME NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f','now','localtime')), \
            modify_time DATETEXT NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f','now','localtime'))); \
         CREATE TABLE IF NOT EXISTS vnote_items_tbl(\
            note_id INTEGER PRIMARY KEY AUTOINCREMENT, \
            folder_id INTEGER UNIQUE, \
            note_type INT NOT NULL DEFAULT 0, \
            note_text TEXT, \
            voice_path TEXT, \
            create_time DATETIME NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')), \
            modify_time DATETEXT NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')));";

    enum DB_TABLE {
        VNOTE_FOLDER_TBL,
        VNOTE_ITEM_TBL,
        VNOTE_MAX_TBL
    };

    QSqlDatabase& getVNoteDb();

    bool insertData(DB_TABLE table,
                    QString insertSql,
                    DbVisitor* visitor /*out*/);
    bool updateData(DB_TABLE table, QString updateSql);
    bool queryData(DB_TABLE table, QString querySql, DbVisitor* visitor);
    bool deleteData(DB_TABLE table, QString delSql);
signals:

public slots:

protected:
        int initVNoteDb();
        void createTablesIfNeed();

protected:
    QSqlDatabase m_vnoteDB;
//    QScopedPointer<QSqlQuery> m_sqlQuery;

    QMutex m_dbLock;

    static VNoteDbManager* _instance;
};

#endif // VNOTEDBMANAGER_H
