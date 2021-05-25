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

#include <QRect>
#include <QPaintEvent>
#include <standarditemcommon.h>

ut_MoveView_test::ut_MoveView_test()
{
    MoveView moveView;
    VNoteItem *vnoteitem = new VNoteItem;
    vnoteitem->noteId = 23;
    vnoteitem->folderId = 23;
    vnoteitem->noteTitle = "test1";
    QList<VNoteItem *> list;
    list.append(vnoteitem);
    moveView.setNoteList(list);
    moveView.setNotesNumber(1);
    moveView.grab();

    VNoteItem *vnoteitem1 = new VNoteItem;
    vnoteitem1->noteId = 2;
    vnoteitem1->folderId = 3;
    vnoteitem1->noteTitle = "test2";
    list.append(vnoteitem1);
    moveView.setNoteList(list);
    moveView.setNotesNumber(2);
    moveView.grab();

    moveView.m_noteList.clear();
    VNoteFolder *vnotefolder = new VNoteFolder;
    vnotefolder->id = 0;
    vnotefolder->category = 1;
    vnotefolder->notesCount = 2;
    vnotefolder->defaultIcon = 3;
    vnotefolder->folder_state = vnotefolder->Normal;
    vnotefolder->name = "test";
    vnotefolder->iconPath = "/home/zhangteng/works/deepin-voice-note/assets/icons/deepin/builtin/default_folder_icons";
    vnotefolder->sortNumber = 4;
    vnotefolder->createTime = QDateTime::currentDateTime();
    vnotefolder->modifyTime = QDateTime::currentDateTime();
    vnotefolder->deleteTime = QDateTime::currentDateTime();
    moveView.setFolder(vnotefolder);
    moveView.grab();

    delete vnoteitem;
    delete vnoteitem1;
    delete vnotefolder;
}

TEST_F(ut_MoveView_test, setFolder)
{
    MoveView moveview;
    VNoteFolder vnotefolder;
    vnotefolder.id = 0;
    vnotefolder.category = 1;
    vnotefolder.notesCount = 2;
    vnotefolder.defaultIcon = 3;
    vnotefolder.folder_state = vnotefolder.Normal;
    vnotefolder.name = "test";
    vnotefolder.iconPath = "/home/zhangteng/works/deepin-voice-note/assets/icons/deepin/builtin/default_folder_icons";
    vnotefolder.sortNumber = 4;
    vnotefolder.createTime = QDateTime::currentDateTime();
    vnotefolder.modifyTime = QDateTime::currentDateTime();
    vnotefolder.deleteTime = QDateTime::currentDateTime();

    VNoteItem vnoteitem;
    vnoteitem.noteId = 0;
    vnoteitem.folderId = 1;
    vnoteitem.noteTitle = "test";
    vnoteitem.createTime = QDateTime::currentDateTime();
    vnoteitem.modifyTime = QDateTime::currentDateTime();
    vnoteitem.deleteTime = QDateTime::currentDateTime();

    moveview.setFolder(&vnotefolder);
    moveview.setNote(&vnoteitem);
}

TEST_F(ut_MoveView_test, setNotesNumber)
{
    MoveView moveView;
    moveView.setNotesNumber(2);
}

TEST_F(ut_MoveView_test, setNoteList)
{
    QList<VNoteItem *> noteList;
    MoveView moveView;
    moveView.setNoteList(noteList);
}
