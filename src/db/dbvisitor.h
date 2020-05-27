#ifndef DBVISITOR_H
#define DBVISITOR_H

#include "common/datatypedef.h"

#include <QSqlQuery>
#include <QScopedPointer>

class DbVisitor
{
public:
    explicit DbVisitor(QSqlDatabase& db, const void *inParam, void* result = nullptr /*out*/);
    virtual ~DbVisitor();

    /*
     * Override the visitorData if need return data
     * */
    virtual bool visitorData();
    virtual bool prepareSqls() = 0;

    QSqlQuery* sqlQuery();
    const QStringList& dbvSqls();

    struct ExtraData {
        union {
            bool flag {false};
            //TODO:
            //    Add expand data here
        } data;
    };

    ExtraData& extraData();
public:
    struct DBFolder {
        enum  {
            folder_id = 0,
            category_id,
            folder_name,
            default_icon,
            icon_path,
            folder_state,
            max_noteid,
            create_time,
            modify_time,
            delete_time,
        };

        static const QStringList folderColumnsName;
    };

    struct DBNote {
        enum {
            note_id = 0,
            folder_id,
            note_type,
            note_title,
            meta_data,
            note_state,
            create_time,
            modify_time,
            delete_time,
        };

        static const QStringList noteColumnsName;
    };

    struct DBSafer {
        enum {
            id = 0,
            folder_id,
            note_id,
            path,
            state,
            meta_data,
            create_time,
        };

        static const QStringList saferColumnsName;
    };
protected:
    //Check & replace the "'" in the string.
    void checkSqlStr(QString& sql);

    union {
        VNOTE_FOLDERS_MAP   *folders;
        VNOTE_ALL_NOTES_MAP *notes;
        VNoteFolder         *newFolder;
        VNoteItem           *newNote;
        SafetyDatas         *safetyDatas;
        qint32              *count;
        qint64              *id;
        void                *ptr;
    } results;

    union {
        const VNoteFolder         *newFolder;
        const VNoteItem           *newNote;
        const VDataSafer          *safer;
        const qint32              *count;
        const qint64              *id;
        const void                *ptr;
    } param;

    QScopedPointer<QSqlQuery> m_sqlQuery {nullptr};

    QStringList m_dbvSqls;

    ExtraData   m_extraData; //Use defined, default not used.
};

class FolderQryDbVisitor : public DbVisitor {
public:
    explicit FolderQryDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

class MaxIdFolderDbVisitor : public DbVisitor {
public:
    explicit MaxIdFolderDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

class AddFolderDbVisitor : public DbVisitor {
public:
    explicit AddFolderDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

class RenameFolderDbVisitor : public DbVisitor {
public:
    explicit RenameFolderDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool prepareSqls() override;
};

class DelFolderDbVisitor : public DbVisitor {
public:
    explicit DelFolderDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool prepareSqls() override;
};

class NoteQryDbVisitor : public DbVisitor {
public:
    explicit NoteQryDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

class AddNoteDbVisitor : public DbVisitor {
public:
    explicit AddNoteDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

class RenameNoteDbVisitor : public DbVisitor {
public:
    explicit RenameNoteDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool prepareSqls() override;
};

class UpdateNoteDbVisitor : public DbVisitor {
public:
    explicit UpdateNoteDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool prepareSqls() override;
};

class DelNoteDbVisitor : public DbVisitor {
public:
    explicit DelNoteDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool prepareSqls() override;
};

class SaferQryDbVisitor : public DbVisitor {
public:
    explicit SaferQryDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

class AddSaferDbVisitor : public DbVisitor {
public:
    explicit AddSaferDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool prepareSqls() override;
};

class DelSaferDbVisitor : public DbVisitor {
public:
    explicit DelSaferDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool prepareSqls() override;
};
#endif // DBVISITOR_H
