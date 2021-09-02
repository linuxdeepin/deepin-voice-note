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

#include "ut_vnotedatamanager.h"
#include "vnotedatamanager.h"
#include "vnoteforlder.h"
#include "vnoteitem.h"
#include "vnoteitemoper.h"
#include "vnotefolderoper.h"

ut_vnotedatamanager_test::ut_vnotedatamanager_test()
{
}

TEST_F(ut_vnotedatamanager_test, folderNotesCount)
{
}

TEST_F(ut_vnotedatamanager_test, isAllDatasReady)
{
    VNoteDataManager vnotedatamanager;
    vnotedatamanager.m_fDataState = 3;
    EXPECT_TRUE(vnotedatamanager.isAllDatasReady()) << "m_fDataState is 3";
    vnotedatamanager.m_fDataState = 2;
    EXPECT_FALSE(vnotedatamanager.isAllDatasReady()) << "m_fDataState is 2";
    vnotedatamanager.m_fDataState = 1;
    EXPECT_FALSE(vnotedatamanager.isAllDatasReady()) << "m_fDataState is 1";
}

TEST_F(ut_vnotedatamanager_test, instance)
{
    VNoteDataManager *vnotedatamanager;
    vnotedatamanager = VNoteDataManager::instance();
    EXPECT_FALSE(vnotedatamanager == nullptr);
}

TEST_F(ut_vnotedatamanager_test, addFolder)
{
    VNoteFolder folder;
    folder.id = 2;
    folder.notesCount = 1;
    folder.name = "test";

    VNoteDataManager vnotedatamanager;
    vnotedatamanager.m_qspNoteFoldersMap.reset(new VNOTE_FOLDERS_MAP());
    vnotedatamanager.m_qspAllNotesMap.reset(new VNOTE_ALL_NOTES_MAP());
    EXPECT_EQ("test", vnotedatamanager.addFolder(&folder)->name) << "addFolder";
    EXPECT_EQ("test", vnotedatamanager.getFolder(2)->name) << "getFolder";

    VNoteItem tmpNote;
    tmpNote.folderId = 2;
    tmpNote.noteId = 0;
    tmpNote.noteType = VNoteItem::VNT_Text;
    VNoteBlock *ptrBlock1 = tmpNote.newBlock(VNoteBlock::Text);
    tmpNote.addBlock(ptrBlock1);
    //    vnotedatamanager.addNote(&tmpNote);
    EXPECT_EQ(VNoteItem::VNT_Text, vnotedatamanager.addNote(&tmpNote)->noteType);
    //    vnotedatamanager.getNote(folder.id, tmpNote.noteId);
    EXPECT_EQ(VNoteItem::VNT_Text, vnotedatamanager.getNote(folder.id, tmpNote.noteId)->noteType);
    //    vnotedatamanager.getNote(3, tmpNote.noteId);
    EXPECT_TRUE(nullptr == vnotedatamanager.getNote(3, tmpNote.noteId));
    vnotedatamanager.delNote(folder.id, tmpNote.noteId);
    vnotedatamanager.delNote(3, tmpNote.noteId);
    vnotedatamanager.folderNotesCount(folder.id);
    vnotedatamanager.getFolderNotes(folder.id);
    vnotedatamanager.folderCount();
    VNOTE_ITEMS_MAP *tmpfolder = vnotedatamanager.getFolderNotes(3);
    delete tmpfolder;
    vnotedatamanager.getAllNotesInFolder();
    vnotedatamanager.getDefaultIcon(0, IconsType::DefaultIcon);
    vnotedatamanager.delFolder(folder.id);
}

TEST_F(ut_vnotedatamanager_test, onFoldersLoaded)
{
    VNoteDataManager vnotedatamanager;
    vnotedatamanager.m_qspNoteFoldersMap.reset(new VNOTE_FOLDERS_MAP);
    VNOTE_FOLDERS_MAP *folders = vnotedatamanager.getNoteFolders();
    folders->folders.insert(0, new VNoteFolder());
    vnotedatamanager.onFoldersLoaded(folders);
    EXPECT_EQ(nullptr, vnotedatamanager.m_pForldesLoadThread);
}

TEST_F(ut_vnotedatamanager_test, onAllNotesLoaded)
{
    VNoteDataManager vnotedatamanager;
    vnotedatamanager.m_qspAllNotesMap.reset(new VNOTE_ALL_NOTES_MAP);
    VNOTE_ALL_NOTES_MAP *notes = vnotedatamanager.getAllNotesInFolder();
    VNOTE_ITEMS_MAP *items = new VNOTE_ITEMS_MAP;
    items->folderNotes.insert(0, new VNoteItem);
    notes->notes.insert(0, items);
    vnotedatamanager.onAllNotesLoaded(notes);
    EXPECT_EQ(nullptr, vnotedatamanager.m_pNotesLoadThread);
}

TEST_F(ut_vnotedatamanager_test, reqNoteDefIcons)
{
    VNoteDataManager vnotedatamanager;
    vnotedatamanager.reqNoteDefIcons();
}
