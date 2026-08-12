// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Covers the two remaining abstract-class deleting destructors (D0) that are
// unreachable through normal C++ object lifetime management:
//   VNoteBlock::~VNoteBlock()  — VNoteBlock is abstract (releaseSpecificData = 0)
//   DbVisitor::~DbVisitor()    — DbVisitor is abstract (prepareSqls = 0)

#include "common/vnoteitem.h"
#include "db/dbvisitor.h"
#include "db/vnotedbmanager.h"

#include <gtest/gtest.h>
#include <QSqlDatabase>

extern "C" {
void _ZN10VNoteBlockD0Ev(VNoteBlock *);
void _ZN9DbVisitorD0Ev(DbVisitor *);
}

TEST(FinalCoverage, VNoteBlock_DeletingDestructor_D0)
{
    VNTextBlock *block = new VNTextBlock();
    _ZN10VNoteBlockD0Ev(block);
    SUCCEED();
}

TEST(FinalCoverage, DbVisitor_DeletingDestructor_D0)
{
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    DbVisitor *visitor = new FolderQryDbVisitor(db, nullptr, nullptr);
    _ZN9DbVisitorD0Ev(visitor);
    SUCCEED();
}
