#ifndef DBVISITOR_H
#define DBVISITOR_H

#include "common/datatypedef.h"

#include <QSqlQuery>
#include <QScopedPointer>

class DbVisitor
{
public:
    explicit DbVisitor(QSqlDatabase& db, void* result);
    virtual ~DbVisitor();

    virtual bool visitorData() = 0;

    QSqlQuery* sqlQuery();
protected:
    union {
        VNOTE_FOLDERS_MAP   *folders;
        VNOTE_ALL_NOTES_MAP *notes;
        VNoteFolder         *newFolder;
        VNoteItem           *newNote;
        qint32              *count;
        void                *ptr;
    } results;

    QScopedPointer<QSqlQuery> m_sqlQuery {nullptr};
};

class FolderQryDbVisitor : public DbVisitor {
public:
    explicit FolderQryDbVisitor(QSqlDatabase& db, void* result);

    virtual bool visitorData() override;
};

class CountDbVisitor : public DbVisitor {
public:
    explicit CountDbVisitor(QSqlDatabase& db, void* result);

    virtual bool visitorData() override;
};

class AddFolderDbVisitor : public DbVisitor {
public:
    explicit AddFolderDbVisitor(QSqlDatabase& db, void* result);

    virtual bool visitorData() override;
};

class NoteQryDbVisitor : public DbVisitor {
public:
    explicit NoteQryDbVisitor(QSqlDatabase& db, void* result);

    virtual bool visitorData() override;
};

class AddNoteDbVisitor : public DbVisitor {
public:
    explicit AddNoteDbVisitor(QSqlDatabase& db, void* result);

    virtual bool visitorData() override;
};
#endif // DBVISITOR_H
