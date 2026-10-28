// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "loadnoteitemsworker.h"
#include "ddlog.h"
#include "db/vnoteitemoper.h"
#include "globaldef.h"
#include <QDebug>

// #include <DLog>

#include <QThread>

/**
 * @brief LoadNoteItemsWorker::LoadNoteItemsWorker
 * @param parent
 */
LoadNoteItemsWorker::LoadNoteItemsWorker(QObject *parent)
    : VNTask(parent)
{
    qCInfo(voiceNoteLog) << "LoadNoteItemsWorker constructor called";
}

/**
 * @brief LoadNoteItemsWorker::run
 */
void LoadNoteItemsWorker::run()
{
    qCInfo(voiceNoteLog) << "LoadNoteItemsWorker run called";
    static struct timeval start, backups, end;

    gettimeofday(&start, nullptr);
    backups = start;

    VNoteItemOper notesOper;
    VNOTE_ALL_NOTES_MAP *notesMap = notesOper.loadAllVNotes();

    gettimeofday(&end, nullptr);

    qCDebug(voiceNoteLog) << "LoadNoteItemsWorker(ms):" << TM(start, end);

    //TODO:
    //    Add load folder code here

    qCDebug(voiceNoteLog) << __FUNCTION__ << " load all notes ok:" << notesMap->notes.size() /*<< " thread id:" << QThread::currentThreadId()*/;

    emit onAllNotesLoaded(notesMap);
}
