#ifndef DATATYPEDEF_H
#define DATATYPEDEF_H

#include "common/opsstateinterface.h"

#include <QMap>
#include <QVector>
#include <QReadWriteLock>

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

#endif // DATATYPEDEF_H
