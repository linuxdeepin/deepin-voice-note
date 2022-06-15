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

#include "ut_moveview.h"
#include "vnoteforlder.h"
#include "moveview.h"
#include "vnoteitem.h"
#include "middleview.h"
#include "common/vnotedatamanager.h"

#include <QRect>
#include <QPaintEvent>
#include <standarditemcommon.h>

UT_MoveView::UT_MoveView()
{
}

TEST_F(UT_MoveView, UT_MoveView_setFolder_001)
{
    MoveView moveView;
    VNoteFolder *folder = VNoteDataManager::instance()->getNoteFolders()->folders[0];
    moveView.setFolder(folder);
    EXPECT_EQ(moveView.m_folder, folder);
    moveView.grab();
    moveView.m_hasComposite = !moveView.m_hasComposite;
    moveView.grab();
}

TEST_F(UT_MoveView, UT_MoveView_setNoteList_001)
{
    MoveView moveView;
    VNoteFolder *folder = VNoteDataManager::instance()->getNoteFolders()->folders[0];
    VNoteItem *note = folder->getNotes()->folderNotes[0];
    QList<VNoteItem *> list;
    list.append(note);
    moveView.setNote(note);
    EXPECT_EQ(moveView.m_note, note);
    moveView.setNoteList(list);
    EXPECT_EQ(moveView.m_noteList, list);
    moveView.setNotesNumber(1);
    EXPECT_EQ(moveView.m_notesNumber, 1);
    moveView.grab();
    moveView.setNotesNumber(2);
    EXPECT_EQ(moveView.m_notesNumber, 2);
    moveView.grab();
    moveView.m_hasComposite = !moveView.m_hasComposite;
    moveView.setNotesNumber(1);
    EXPECT_EQ(moveView.m_notesNumber, 1);
    moveView.grab();
    moveView.setNotesNumber(2);
    EXPECT_EQ(moveView.m_notesNumber, 2);
    moveView.grab();
}
