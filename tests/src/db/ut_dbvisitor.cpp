/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_dbvisitor.h"
#include "dbvisitor.h"
#include "vnotedbmanager.h"

UT_DbVisitor::UT_DbVisitor()
{
}

TEST_F(UT_DbVisitor, UT_DbVisitor_visitorData_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    FolderQryDbVisitor folderqrydbvisitor(db, nullptr, nullptr);
    dbvisitor = new FolderQryDbVisitor(db, nullptr, nullptr);
    EXPECT_TRUE(dbvisitor->prepareSqls());
    EXPECT_FALSE(dbvisitor->visitorData());
    EXPECT_FALSE(nullptr == dbvisitor->sqlQuery());
    EXPECT_FALSE(dbvisitor->dbvSqls().isEmpty());
    EXPECT_FALSE(dbvisitor->extraData().data.flag);
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_MaxIdFolderDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    MaxIdFolderDbVisitor maxidfolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new MaxIdFolderDbVisitor(db, nullptr, nullptr);
    EXPECT_TRUE(dbvisitor->prepareSqls());
    EXPECT_FALSE(dbvisitor->visitorData());
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_AddFolderDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    AddFolderDbVisitor addfolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new AddFolderDbVisitor(db, nullptr, nullptr);
    EXPECT_FALSE(dbvisitor->prepareSqls());
    EXPECT_FALSE(dbvisitor->visitorData());
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_RenameFolderDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    RenameFolderDbVisitor renamefolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new RenameFolderDbVisitor(db, nullptr, nullptr);
    EXPECT_FALSE(dbvisitor->prepareSqls());
    EXPECT_TRUE(dbvisitor->visitorData());
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_DelFolderDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    DelFolderDbVisitor delfolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new DelFolderDbVisitor(db, nullptr, nullptr);
    EXPECT_FALSE(dbvisitor->prepareSqls());
    EXPECT_TRUE(dbvisitor->visitorData());
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_NoteQryDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    NoteQryDbVisitor noteqrydbvisitor(db, nullptr, nullptr);
    dbvisitor = new NoteQryDbVisitor(db, nullptr, nullptr);
    EXPECT_TRUE(dbvisitor->prepareSqls());
    EXPECT_FALSE(dbvisitor->visitorData());
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_RenameNoteDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    RenameNoteDbVisitor renamenotedbvisitor(db, nullptr, nullptr);
    dbvisitor = new RenameNoteDbVisitor(db, nullptr, nullptr);
    EXPECT_FALSE(dbvisitor->prepareSqls());
    EXPECT_TRUE(dbvisitor->visitorData());
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_UpdateNoteDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    UpdateNoteDbVisitor updatenotedbvisitor(db, nullptr, nullptr);
    dbvisitor = new UpdateNoteDbVisitor(db, nullptr, nullptr);
    EXPECT_FALSE(dbvisitor->prepareSqls());
    EXPECT_TRUE(dbvisitor->visitorData());
    delete dbvisitor;
}

TEST_F(UT_DbVisitor, UT_DbVisitor_DelNoteDbVisitor_001)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    DelNoteDbVisitor delnotedbvisitor(db, nullptr, nullptr);
    dbvisitor = new DelNoteDbVisitor(db, nullptr, nullptr);
    EXPECT_FALSE(dbvisitor->prepareSqls());
    EXPECT_TRUE(dbvisitor->visitorData());
    delete dbvisitor;
}
