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
    static constexpr char const *NOTES_TABLE_NAME = "vnote_items_tbl";
    static constexpr char const *NOTES_KEY        = "note_id";

    //icon_path: Not used, maybe used in future
    //expand_fields are place holder, will be used in future
    static constexpr char const *CREATETABLE_FMT = "\
         CREATE TABLE IF NOT EXISTS vnote_folder_tbl(\
            folder_id INTEGER PRIMARY KEY AUTOINCREMENT , \
            folder_name TEXT NOT NULL, \
            default_icon INT DEFAULT 0, \
            icon_path TEXT,\
            folder_state INT DEFAULT 0, \
            max_noteid INT DEFAULT 0, \
            create_time DATETIME NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f','now','localtime')), \
            modify_time DATETEXT NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f','now','localtime')), \
            delete_time DATETEXT DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f','now','localtime')), \
            expand_filed1 INT, \
            expand_filed2 INT, \
            expand_filed3 INT, \
            expand_filed4 TEXT, \
            expand_filed5 TEXT, \
            expand_filed6 TEXT \
         ); \
         CREATE TABLE IF NOT EXISTS vnote_items_tbl(\
            note_id INTEGER PRIMARY KEY AUTOINCREMENT, \
            folder_id INTEGER, \
            note_type INT NOT NULL DEFAULT 0, \
            note_title TEXT NOT NULL, \
            meta_data TEXT, \
            note_state INT DEFAULT 0, \
            create_time DATETIME NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')), \
            modify_time DATETEXT NOT NULL DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f', 'now', 'localtime')), \
            delete_time DATETEXT DEFAULT (STRFTIME ('%Y-%m-%d %H:%M:%f','now','localtime')), \
            expand_filed1 INT, \
            expand_filed2 INT, \
            expand_filed3 INT, \
            expand_filed4 TEXT, \
            expand_filed5 TEXT, \
            expand_filed6 TEXT \
         );";

    enum DB_TABLE {
        VNOTE_FOLDER_TBL,
        VNOTE_ITEM_TBL,
        VNOTE_MAX_TBL
    };

    QSqlDatabase& getVNoteDb();

    bool insertData(const QStringList& insertSql, DbVisitor* visitor /*out*/);
    bool updateData(const QStringList& updateSql);
    bool queryData(const QString querySql, DbVisitor* visitor);
    bool deleteData(const QStringList& delSql);
signals:

public slots:

protected:
        int initVNoteDb();
        void createTablesIfNeed();

protected:
    QSqlDatabase m_vnoteDB;
//    QScopedPointer<QSqlQuery> m_sqlQuery;

    QMutex m_dbLock;
    bool   m_isDbInitOK {false};

    static VNoteDbManager* _instance;
};

#endif // VNOTEDBMANAGER_H
