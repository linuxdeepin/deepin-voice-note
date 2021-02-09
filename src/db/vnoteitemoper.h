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
#ifndef VNOTEITEMOPER_H
#define VNOTEITEMOPER_H

#include "common/datatypedef.h"

//记事项表操作
class VNoteItemOper
{
public:
    explicit VNoteItemOper(VNoteItem *note = nullptr);
    //获取所有记事项数据
    VNOTE_ALL_NOTES_MAP *loadAllVNotes();
    //修改名称
    bool modifyNoteTitle(QString title);
    //更新数据
    bool updateNote();
    //添加记事项
    VNoteItem *addNote(VNoteItem &note);
    //获取记事项
    VNoteItem *getNote(qint64 folderId, qint32 noteId);
    //获取一个记事本所有记事项
    VNOTE_ITEMS_MAP *getFolderNotes(qint64 folderId);
    //生成默认名称
    QString getDefaultNoteName(qint64 folderId);
    //生成默认语音名称
    QString getDefaultVoiceName() const;
    //删除记事项
    bool deleteNote();
    //更新置顶属性
    bool updateTop(int value);
    //更新folderid
    bool updateFolderId(VNoteItem *data);

protected:
    VNoteItem *m_note {nullptr};
};

#endif // VNOTEITEMOPER_H
