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

ut_vnotedatamanager_test::ut_vnotedatamanager_test()
{
}

TEST_F(ut_vnotedatamanager_test, folderNotesCount)
{
}

TEST_F(ut_vnotedatamanager_test, isAllDatasReady)
{
    VNoteDataManager vnotedatamanager;
    //    vnotedatamanager.reqNoteDefIcons();
    //    vnotedatamanager.reqNoteFolders();
    //    vnotedatamanager.reqNoteItems();
    vnotedatamanager.isAllDatasReady();
}

TEST_F(ut_vnotedatamanager_test, instance)
{
    VNoteDataManager *vnotedatamanager;
    vnotedatamanager = VNoteDataManager::instance();
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
    vnotedatamanager.addFolder(&folder);
    vnotedatamanager.getFolder(folder.id);
    vnotedatamanager.delFolder(folder.id);

    VNoteItem tmpNote;
    tmpNote.folderId = 2;
    tmpNote.noteType = VNoteItem::VNT_Text;
    VNoteBlock *ptrBlock1 = tmpNote.newBlock(VNoteBlock::Text);
    tmpNote.addBlock(ptrBlock1);
    vnotedatamanager.addNote(&tmpNote);
    vnotedatamanager.getNote(folder.id, tmpNote.noteId);
    vnotedatamanager.getNote(3, tmpNote.noteId);
    vnotedatamanager.delNote(folder.id, tmpNote.noteId);
    vnotedatamanager.delNote(3, tmpNote.noteId);
    vnotedatamanager.folderNotesCount(folder.id);
    vnotedatamanager.getFolderNotes(folder.id);
    vnotedatamanager.folderCount();
    vnotedatamanager.getFolderNotes(3);
    vnotedatamanager.getAllNotesInFolder();

    QString defaultIconPathFmt(":/icons/deepin/builtin/default_folder_icons/%1.svg");

    for (int i = 0; i < 10; i++) {
        QString iconPath = defaultIconPathFmt.arg(i + 1);
        QPixmap bitmap(iconPath);
        VNoteDataManager::m_defaultIcons[IconsType::DefaultIcon].push_back(bitmap);
    }
    vnotedatamanager.getDefaultIcon(0, IconsType::DefaultIcon);
}
