#include "olddbvisistors.h"

OldFolderQryDbVisitor::OldFolderQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool OldFolderQryDbVisitor::visitorData()
{

}

bool OldFolderQryDbVisitor::prepareSqls()
{

}

OldNoteQryDbVisitor::OldNoteQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result)
    :DbVisitor (db, inParam, result)
{

}

bool OldNoteQryDbVisitor::visitorData()
{

}

bool OldNoteQryDbVisitor::prepareSqls()
{

}
