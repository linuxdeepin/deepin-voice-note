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
#include "vnotedbmanager.h"
#include "vnotedatamanager.h"
#include "vnotefolderoper.h"
#include "vnoteforlder.h"
#include "vnoteitemoper.h"
#include "vnoteitem.h"

ut_vnoteitemoper_test::ut_vnoteitemoper_test()
{
}

void ut_vnoteitemoper_test::SetUp()
{
    VNOTE_ALL_NOTES_MAP *notes = VNoteDataManager::instance()->getAllNotesInFolder();
    if (notes && !notes->notes.isEmpty()) {
        VNOTE_ITEMS_MAP *tmp = notes->notes.first();
        if (tmp && !tmp->folderNotes.isEmpty()) {
            m_note = tmp->folderNotes.first();
        }
    }

    m_vnoteitemoper = new VNoteItemOper(m_note);
}

void ut_vnoteitemoper_test::TearDown()
{
    delete m_vnoteitemoper;
}

TEST_F(ut_vnoteitemoper_test, loadAllVNotes)
{
    VNOTE_ALL_NOTES_MAP *notes = m_vnoteitemoper->loadAllVNotes();
    delete notes;
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
    VNoteItem tmpNote;
    tmpNote.folderId = m_note->folderId;
    tmpNote.noteType = VNoteItem::VNT_Text;
    tmpNote.noteTitle = m_vnoteitemoper->getDefaultNoteName(tmpNote.folderId);
    VNoteBlock *ptrBlock1 = tmpNote.newBlock(VNoteBlock::Text);
    tmpNote.addBlock(ptrBlock1);
    m_vnoteitemoper->addNote(tmpNote);
    VNoteItemOper op(&tmpNote);
    op.deleteNote();
}

TEST_F(ut_vnoteitemoper_test, getDefaultVoiceName)
{
    m_vnoteitemoper->getDefaultVoiceName();
}

TEST_F(ut_vnoteitemoper_test, updateTop)
{
    m_vnoteitemoper->updateTop(!m_note->isTop);
    m_vnoteitemoper->updateTop(!m_note->isTop);
}

TEST_F(ut_vnoteitemoper_test, updateFolderId)
{
    m_vnoteitemoper->updateFolderId(m_note);
}

TEST_F(ut_vnoteitemoper_test, getNote)
{
    m_vnoteitemoper->getNote(m_note->folderId, m_note->noteId);
}
