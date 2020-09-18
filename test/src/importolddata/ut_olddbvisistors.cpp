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

ut_olddbvisistors_test::ut_olddbvisistors_test()
{

}

TEST_F(ut_olddbvisistors_test, visitorData)
{
    QSqlDatabase db;
    OldFolderQryDbVisitor oldfolderqrydbvisitor(db, nullptr, nullptr);
    oldfolderqrydbvisitor.prepareSqls();
    oldfolderqrydbvisitor.visitorData();

    OldNoteQryDbVisitor oldnoteqrydbvisitor(db, nullptr, nullptr);
    oldnoteqrydbvisitor.prepareSqls();
    oldnoteqrydbvisitor.visitorData();
}
