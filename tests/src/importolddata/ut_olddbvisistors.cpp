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

#include "ut_olddbvisistors.h"
#include "olddbvisistors.h"
#include "stub.h"

static bool ret = true;

bool stub_sql()
{
    if (ret == true) {
        ret = false;
        return true;
    }
    return ret;
}

ut_olddbvisistors_test::ut_olddbvisistors_test()
{
}

TEST_F(ut_olddbvisistors_test, visitorData)
{
    Stub stub1;
    stub1.set(ADDR(QSqlQuery, next), stub_sql);
    QSqlDatabase db;
    VNOTE_FOLDERS_MAP folders;
    folders.autoRelease = true;

    OldFolderQryDbVisitor oldfolderqrydbvisitor(db, nullptr, &folders);
    oldfolderqrydbvisitor.prepareSqls();
    oldfolderqrydbvisitor.visitorData();
    VNOTE_ALL_NOTES_MAP notes;
    notes.autoRelease = true;

    ret = true;
    OldNoteQryDbVisitor oldnoteqrydbvisitor(db, nullptr, &notes);
    oldnoteqrydbvisitor.prepareSqls();
    oldnoteqrydbvisitor.visitorData();
}
