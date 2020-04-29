#ifndef OLDDBVISISTORS_H
#define OLDDBVISISTORS_H

#include "db/dbvisitor.h"

class OldFolderQryDbVisitor : public DbVisitor {
public:
    explicit OldFolderQryDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

class OldNoteQryDbVisitor : public DbVisitor {
public:
    explicit OldNoteQryDbVisitor(QSqlDatabase& db, const void *inParam, void* result);

    enum OldNoteType {
        Voice = 0,
        Text,
    };

    virtual bool visitorData() override;
    virtual bool prepareSqls() override;
};

#endif // OLDDBVISISTORS_H
