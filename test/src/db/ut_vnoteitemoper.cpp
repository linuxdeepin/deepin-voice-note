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

#include "ut_vnoteitemoper.h"
#include "vnoteitemoper.h"
#include "vnoteitem.h"

ut_vnoteitemoper_test::ut_vnoteitemoper_test()
{

}

void ut_vnoteitemoper_test::SetUp()
{
    m_vnoteitemoper = new VNoteItemOper;
}

void ut_vnoteitemoper_test::TearDown()
{
    delete m_vnoteitemoper;
}

TEST_F(ut_vnoteitemoper_test, loadAllVNotes)
{
    m_vnoteitemoper->loadAllVNotes();
}

TEST_F(ut_vnoteitemoper_test, modifyNoteTitle)
{
    m_vnoteitemoper->modifyNoteTitle("test");
}

TEST_F(ut_vnoteitemoper_test, updateNote)
{
    m_vnoteitemoper->updateNote();
}

TEST_F(ut_vnoteitemoper_test, addNote)
{
//    VNoteItemOper noteOper;
//    VNoteItem noteitem;
//    noteitem.folderId = 1;
//    noteitem.noteType = VNoteItem::VNT_Text;
//    VNoteBlock *ptrBlock = noteitem.newBlock(VNoteBlock::Text);
//    noteitem.addBlock(ptrBlock);
//    noteitem.noteTitle = noteOper.getDefaultNoteName(noteitem.folderId);

//    m_vnoteitemoper->addNote(noteitem);
}

TEST_F(ut_vnoteitemoper_test, getDefaultVoiceName)
{
    m_vnoteitemoper->getDefaultVoiceName();
}

TEST_F(ut_vnoteitemoper_test, deleteNote)
{
    m_vnoteitemoper->deleteNote();
}
