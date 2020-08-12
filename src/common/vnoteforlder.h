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
#ifndef VNOTEFORLDER_H
#define VNOTEFORLDER_H

#include "datatypedef.h"

#include <QtGlobal>
#include <QDateTime>
#include <QPixmap>
struct VNoteFolder {
public:
    VNoteFolder();
    ~VNoteFolder();

    bool isValid();

    enum {
        INVALID_ID = -1
    };

    enum State {
        Normal,
        Deleted,
    };

    qint64 id {INVALID_ID};
    qint32 category {0};
    qint64 notesCount {0};
    qint32 defaultIcon {0};
    qint32 folder_state {State::Normal};

    QString name;
    QString iconPath;

    QDateTime createTime;
    QDateTime modifyTime;
    QDateTime deleteTime;

    struct {
        QPixmap icon;
        QPixmap grayIcon;
    } UI;

    qint32 &maxNoteIdRef();

    qint32 getNotesCount();
    VNOTE_ITEMS_MAP *getNotes();

    friend QDebug &operator<<(QDebug &out, VNoteFolder &folder);

protected:
    bool fIsDataLoaded {false};

    //Current max note id in the folder,auto
    //increament
    qint32 maxNoteId {0};

    VNOTE_ITEMS_MAP *notes {nullptr};

    friend class VNoteFolderOper;
};

#endif // VNOTEFORLDER_H
