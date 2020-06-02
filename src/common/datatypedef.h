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
#ifndef DATATYPEDEF_H
#define DATATYPEDEF_H

#include "common/opsstateinterface.h"

#include <QMap>
#include <QVector>
#include <QReadWriteLock>
#include <QDateTime>

struct VNoteFolder;
struct VNoteItem;
struct VNoteBlock;
struct VNOTE_ITEMS_MAP;

typedef QMap<qint64,VNoteFolder*> VNOTE_FOLDERS_DATA_MAP;
typedef QMap<qint64,VNoteItem*>   VNOTE_ITEMS_DATA_MAP;
typedef QMap<qint64, VNOTE_ITEMS_MAP*> VNOTE_ALL_NOTES_DATA_MAP;
typedef QVector<VNoteBlock*> VNOTE_DATA_VECTOR;

struct VNOTE_FOLDERS_MAP {
    ~VNOTE_FOLDERS_MAP();

    VNOTE_FOLDERS_DATA_MAP folders;
    QReadWriteLock lock;

    //TODO:
    //    Release data if true when destructor is called.
    //Only can be set TRUE in data manager
    bool autoRelease {false};
};

struct VNOTE_ITEMS_MAP {
    ~VNOTE_ITEMS_MAP();

    VNOTE_ITEMS_DATA_MAP folderNotes;
    QReadWriteLock lock;

    //TODO:
    //    Release data if true when destructor is called.
    //Only can be set TRUE in data manager
    bool autoRelease {false};
};

struct VNOTE_ALL_NOTES_MAP {
    ~VNOTE_ALL_NOTES_MAP();

    VNOTE_ALL_NOTES_DATA_MAP notes;
    QReadWriteLock lock;

    //TODO:
    //    Release data if true when destructor is called.
    //Only can be set TRUE in data manager
    bool autoRelease {false};
};

struct VNOTE_DATAS {
    ~VNOTE_DATAS();

    const VNOTE_DATA_VECTOR& dataConstRef();
protected:
    VNoteBlock* newBlock(int type);
    void addBlock(VNoteBlock* block);
    void addBlock(VNoteBlock* before, VNoteBlock* block);
    void delBlock(VNoteBlock* block);

    void classifyAddBlk(VNoteBlock* block);
    void classifyDelBlk(VNoteBlock* block);
    //Ordered data set
    VNOTE_DATA_VECTOR datas;

    //Classify data
    VNOTE_DATA_VECTOR textBlocks;
    VNOTE_DATA_VECTOR voiceBlocks;

    friend struct VNoteItem;
    friend class MetaDataParser;
    friend class ExportNoteWorker;
};

struct VDataSafer {
    enum {
        INVALID_ID = -1
    };

    enum SaferType {
        Safe,
        Unsafe,
        ExceptionSafer,
    };

    bool isValid() const;
    void setSaferType(SaferType type);

    SaferType saferType {ExceptionSafer};

    qint32  id {INVALID_ID};
    qint64  folder_id {INVALID_ID};
    qint32  note_id {INVALID_ID};
    qint32  state {0};
    QString path;
    QString meta_data;
    QDateTime createTime;

    friend QDebug& operator << (QDebug& out, const VDataSafer &safer);
};

typedef  QVector<VDataSafer> SafetyDatas;

enum IconsType{
   DefaultIcon = 0x0,
   DefaultGrayIcon,
   MaxIconsType
};

#endif // DATATYPEDEF_H
