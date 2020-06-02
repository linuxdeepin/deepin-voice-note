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
#ifndef VNOTEFOLDEROPER_H
#define VNOTEFOLDEROPER_H

#include "common/datatypedef.h"

#include <QPixmap>

class VNoteFolderOper
{
public:
    explicit VNoteFolderOper(VNoteFolder* folder = nullptr);

    inline bool        isNoteItemLoaded();
    VNOTE_FOLDERS_MAP* loadVNoteFolders();

    VNoteFolder* addFolder(VNoteFolder& folder);
    VNoteFolder* getFolder(qint64 folderId);
    qint32 getFoldersCount();
    qint32 getNotesCount(qint64 folderId);
    qint32 getNotesCount();

    QString getDefaultFolderName();
    qint32 getDefaultIcon();
    QPixmap getDefaultIcon(qint32 index, IconsType type);

    bool deleteVNoteFolder(qint64 folderId);
    bool deleteVNoteFolder(VNoteFolder* folder);
    bool renameVNoteFolder(QString folderName);

protected:
    VNoteFolder* m_folder {nullptr};
};

#endif // VNOTEFOLDEROPER_H
