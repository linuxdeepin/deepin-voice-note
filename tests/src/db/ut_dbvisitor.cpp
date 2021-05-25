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

ut_dbvisitor_test::ut_dbvisitor_test()
{
}

TEST_F(ut_dbvisitor_test, visitorData)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    FolderQryDbVisitor folderqrydbvisitor(db, nullptr, nullptr);
    dbvisitor = new FolderQryDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    dbvisitor->sqlQuery();
    dbvisitor->dbvSqls();
    dbvisitor->extraData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, MaxIdFolderDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    MaxIdFolderDbVisitor maxidfolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new MaxIdFolderDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, AddFolderDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    AddFolderDbVisitor addfolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new AddFolderDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, RenameFolderDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    RenameFolderDbVisitor renamefolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new RenameFolderDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, DelFolderDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    DelFolderDbVisitor delfolderdbvisitor(db, nullptr, nullptr);
    dbvisitor = new DelFolderDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, NoteQryDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    NoteQryDbVisitor noteqrydbvisitor(db, nullptr, nullptr);
    dbvisitor = new NoteQryDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

//TEST_F(ut_dbvisitor_test, AddNoteDbVisitor)
//{
//    DbVisitor *dbvisitor;
//    QSqlDatabase db=VNoteDbManager::instance()->getVNoteDb();
//    AddNoteDbVisitor addnotedbvisitor(db, nullptr, nullptr);
//    dbvisitor = new AddNoteDbVisitor(db, nullptr, nullptr);
//    dbvisitor->prepareSqls();
//    dbvisitor->visitorData();
//}

TEST_F(ut_dbvisitor_test, RenameNoteDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    RenameNoteDbVisitor renamenotedbvisitor(db, nullptr, nullptr);
    dbvisitor = new RenameNoteDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, UpdateNoteDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    UpdateNoteDbVisitor updatenotedbvisitor(db, nullptr, nullptr);
    dbvisitor = new UpdateNoteDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, DelNoteDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    DelNoteDbVisitor delnotedbvisitor(db, nullptr, nullptr);
    dbvisitor = new DelNoteDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, SaferQryDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    SaferQryDbVisitor saferqrydbvisitor(db, nullptr, nullptr);
    dbvisitor = new SaferQryDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, AddSaferDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    AddSaferDbVisitor addsaferdbvisitor(db, nullptr, nullptr);
    dbvisitor = new AddSaferDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}

TEST_F(ut_dbvisitor_test, DelSaferDbVisitor)
{
    DbVisitor *dbvisitor;
    QSqlDatabase db = VNoteDbManager::instance()->getVNoteDb();
    DelSaferDbVisitor delsaferdbvisitor(db, nullptr, nullptr);
    dbvisitor = new DelSaferDbVisitor(db, nullptr, nullptr);
    dbvisitor->prepareSqls();
    dbvisitor->visitorData();
    delete dbvisitor;
}
