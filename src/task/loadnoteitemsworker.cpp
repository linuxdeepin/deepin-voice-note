/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
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
#include "loadnoteitemsworker.h"
#include "db/vnoteitemoper.h"
#include "globaldef.h"

#include <DLog>

/**
 * @brief LoadNoteItemsWorker::LoadNoteItemsWorker
 * @param parent
 */
LoadNoteItemsWorker::LoadNoteItemsWorker(QObject *parent)
    : VNTask(parent)
{
}

/**
 * @brief LoadNoteItemsWorker::run
 */
void LoadNoteItemsWorker::run()
{
    static struct timeval start, backups, end;

    gettimeofday(&start, nullptr);
    backups = start;

    VNoteItemOper notesOper;
    VNOTE_ALL_NOTES_MAP *notesMap = notesOper.loadAllVNotes();

    gettimeofday(&end, nullptr);

    qDebug() << "LoadNoteItemsWorker(ms):" << TM(start, end);

    //TODO:
    //    Add load folder code here

    qDebug() << __FUNCTION__ << " load all notes ok:" << notesMap->notes.size() << " thread id:" << QThread::currentThreadId();

    emit onAllNotesLoaded(notesMap);
}
