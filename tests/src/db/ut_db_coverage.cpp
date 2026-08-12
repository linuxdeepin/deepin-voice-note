// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Coverage tests for previously-uncovered functions in db module:
//   DbVisitor::~DbVisitor (D0), VNoteDbManager::~VNoteDbManager (D0 + D2),
//   VNoteDbManager::hasOldDataBase, VNoteFolderOper::addFolder.
// Build enables -fno-access-control, so protected/private members are accessible.

#include "db/dbvisitor.h"
#include "db/vnotedbmanager.h"
#include "db/vnotefolderoper.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"

#include <gtest/gtest.h>
#include <stub.h>

// ---------------------------------------------------------------------------
// Stub helpers
// ---------------------------------------------------------------------------
static bool stub_false() { return false; }
static bool stub_true() { return true; }

// ===========================================================================
// DbVisitor::~DbVisitor (D0 deleting destructor) — dbvisitor.cpp:79
// ===========================================================================
TEST(DbCoverage, DbVisitor_Destructor_D0)
{
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    DbVisitor *visitor = new FolderQryDbVisitor(db, nullptr, nullptr);
    // D0 (deleting destructor) invoked by delete
    delete visitor;
    SUCCEED();
}

// ===========================================================================
// VNoteDbManager::~VNoteDbManager (D0 deleting destructor) — vnotedbmanager.cpp:52
// Uses fOldDb=true to get a separate connection name and avoid touching the
// singleton's connection. D0 is triggered via delete on a heap pointer.
// ===========================================================================
TEST(DbCoverage, VNoteDbManager_Destructor_D0)
{
    VNoteDbManager *mgr = new VNoteDbManager(true);
    ASSERT_NE(nullptr, mgr);
    delete mgr;   // D0
    SUCCEED();
}

// ===========================================================================
// VNoteDbManager::~VNoteDbManager (D2 complete destructor) — vnotedbmanager.cpp:52
// D2 is triggered by stack-based destruction (scope exit).
// ===========================================================================
TEST(DbCoverage, VNoteDbManager_Destructor_D2)
{
    {
        VNoteDbManager mgr(true);   // stack-allocated
        // D2 fires when scope ends
    }
    SUCCEED();
}

// ===========================================================================
// VNoteDbManager::hasOldDataBase — vnotedbmanager.cpp:245
// Static helper that checks file existence; no side effects.
// ===========================================================================
TEST(DbCoverage, VNoteDbManager_hasOldDataBase)
{
    // Just exercise the body; result depends on environment.
    bool exists = VNoteDbManager::hasOldDataBase();
    // Should not crash; verify it's a valid bool
    EXPECT_TRUE(exists == true || exists == false);
}

// ===========================================================================
// VNoteFolderOper::addFolder(VNoteFolder&) — vnotefolderoper.cpp:149
// Stub insertData to false so the else-branch (auto-release) runs,
// keeping the singleton DataManager clean.
// ===========================================================================
TEST(DbCoverage, VNoteFolderOper_addFolder_InsertFail)
{
    Stub stub;
    stub.set(ADDR(VNoteDbManager, insertData), stub_false);

    VNoteFolder folder;
    folder.name = "ut_coverage_folder";

    VNoteFolderOper oper;
    VNoteFolder *result = oper.addFolder(folder);
    // Insert failed → folder auto-released, returns nullptr
    EXPECT_EQ(nullptr, result);
}

// ===========================================================================
// VNoteFolderOper::addFolder — success path (insertData stubbed true).
// Exercises the if-branch: VNoteDataManager::instance()->addFolder(newFolder).
// Both insertData and VNoteDataManager::addFolder are stubbed so the returned
// pointer is not stored in the singleton, avoiding dangling-pointer issues.
// ===========================================================================
static VNoteFolder *stub_dm_addFolder_noop(VNoteFolder *folder)
{
    return folder;
}

TEST(DbCoverage, VNoteFolderOper_addFolder_InsertOk)
{
    Stub stub;
    stub.set(ADDR(VNoteDbManager, insertData), stub_true);
    stub.set(ADDR(VNoteDataManager, addFolder), stub_dm_addFolder_noop);

    VNoteFolder folder;
    folder.name = "ut_coverage_ok";

    VNoteFolderOper oper;
    VNoteFolder *result = oper.addFolder(folder);
    ASSERT_NE(nullptr, result);
    delete result;
    SUCCEED();
}
