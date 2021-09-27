/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ut_loadnoteitemsworker.h"
#include "task/loadnoteitemsworker.h"

UT_LoadNoteItemsWorker::UT_LoadNoteItemsWorker(QObject *parent)
    : QObject(parent)
{
}

void UT_LoadNoteItemsWorker::onNoteLoad(VNOTE_ALL_NOTES_MAP *notesMap)
{
    EXPECT_TRUE(nullptr != notesMap);
    if (notesMap) {
        delete notesMap;
        notesMap = nullptr;
    }
}

TEST_F(UT_LoadNoteItemsWorker, UT_LoadNoteItemsWorker_run_001)
{
    LoadNoteItemsWorker work;
    connect(&work, &LoadNoteItemsWorker::onAllNotesLoaded, this, &UT_LoadNoteItemsWorker::onNoteLoad);
    work.run();
}
