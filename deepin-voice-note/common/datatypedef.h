#ifndef DATATYPEDEF_H
#define DATATYPEDEF_H

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
typedef QVector<VNoteBlock*> VNoteDatas;

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

static QStringList default_FolderData_imgpath =
{
    ":/images/icon/DefaultAvatar/1.svg", ":/images/icon/DefaultAvatar/2.svg",
    ":/images/icon/DefaultAvatar/3.svg", ":/images/icon/DefaultAvatar/4.svg",
    ":/images/icon/DefaultAvatar/5.svg", ":/images/icon/DefaultAvatar/6.svg",
    ":/images/icon/DefaultAvatar/7.svg", ":/images/icon/DefaultAvatar/8.svg",
    ":/images/icon/DefaultAvatar/9.svg", ":/images/icon/DefaultAvatar/10.svg"
};
#endif // DATATYPEDEF_H
