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

#include "ut_exportnoteworker.h"

#define protected public
#include "exportnoteworker.h"
#undef protected

#include "vnoteitem.h"

#include <QThreadPool>

ut_exportnoteworker_test::ut_exportnoteworker_test()
{

}

TEST_F(ut_exportnoteworker_test, run)
{
    QString dirPath = "/home/zhangteng/test";
    int exportType = 1;
    VNoteItem *note = new VNoteItem;
    ExportNoteWorker *exportnoteworker = new ExportNoteWorker(dirPath, exportType, note);
    QThreadPool::globalInstance()->start(exportnoteworker);

    exportType = 2;
    VNoteBlock *ptrBlock = note->newBlock(2);
    VNoteBlock *ptrBlock1 = note->newBlock(2);
    note->addBlock(ptrBlock);
    note->addBlock(ptrBlock, ptrBlock1);
    ExportNoteWorker *exportnoteworker1 = new ExportNoteWorker(dirPath, exportType, note);
    QThreadPool::globalInstance()->start(exportnoteworker1);

    exportType = 3;
    ExportNoteWorker *exportnoteworker2 = new ExportNoteWorker(dirPath, exportType, note);
    QThreadPool::globalInstance()->start(exportnoteworker2);

    exportType = 4;
    ExportNoteWorker *exportnoteworker3 = new ExportNoteWorker(dirPath, exportType, note);
    QThreadPool::globalInstance()->start(exportnoteworker3);
}
