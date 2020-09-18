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

#include "ut_vnoteforlder.h"
#include "vnoteforlder.h"

ut_vnoteforlder_test::ut_vnoteforlder_test()
{

}

void ut_vnoteforlder_test::SetUp()
{
    m_vnoteforlder = new VNoteFolder;
}

void ut_vnoteforlder_test::TearDown()
{
    delete m_vnoteforlder;
}

TEST_F(ut_vnoteforlder_test, isValid)
{
    m_vnoteforlder->isValid();
}

TEST_F(ut_vnoteforlder_test, maxNoteIdRef)
{
    m_vnoteforlder->maxNoteIdRef();
}

TEST_F(ut_vnoteforlder_test, getNotesCount)
{
//    m_vnoteforlder->getNotesCount();
}

TEST_F(ut_vnoteforlder_test, qdebugtest)
{
    VNoteFolder vnotefolder;
    vnotefolder.id = 0;
    vnotefolder.category = 1;
    vnotefolder.notesCount = 2;
    vnotefolder.defaultIcon = 3;
    vnotefolder.folder_state = vnotefolder.Normal;
    vnotefolder.name = "test";
    vnotefolder.iconPath = "test";
    vnotefolder.createTime = QDateTime::currentDateTime();
    vnotefolder.modifyTime = QDateTime::currentDateTime();
    vnotefolder.deleteTime = QDateTime::currentDateTime();

    qDebug() << "" << vnotefolder;
}
