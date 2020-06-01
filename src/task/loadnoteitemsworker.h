/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#ifndef LOADNOTEITEMSWORKER_H
#define LOADNOTEITEMSWORKER_H

#include "common/datatypedef.h"
#include "vntask.h"

#include <QObject>
#include <QRunnable>
#include <QtGlobal>

class LoadNoteItemsWorker : public VNTask
{
    Q_OBJECT
public:
    LoadNoteItemsWorker(QObject *parent=nullptr);
protected:
    virtual void run();
signals:
    void onAllNotesLoaded(VNOTE_ALL_NOTES_MAP* foldesMap);
public slots:

protected:
};

#endif // LOADNOTEITEMSWORKER_H
