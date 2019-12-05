#ifndef DATATYPEDEF_H
#define DATATYPEDEF_H

#include <QMap>

struct VNoteFolder;
struct VNoteItem;

typedef QMap<qint64,VNoteFolder*> VNOTE_FOLDERS_MAP;
typedef QMap<qint64,VNoteItem*>   VNOTE_ITEMS_MAP;

static QStringList default_FolderData_imgpath =
{
    ":/images/icon/DefaultAvatar/1.svg", ":/images/icon/DefaultAvatar/2.svg",
    ":/images/icon/DefaultAvatar/3.svg", ":/images/icon/DefaultAvatar/4.svg",
    ":/images/icon/DefaultAvatar/5.svg", ":/images/icon/DefaultAvatar/6.svg",
    ":/images/icon/DefaultAvatar/7.svg", ":/images/icon/DefaultAvatar/8.svg",
    ":/images/icon/DefaultAvatar/9.svg", ":/images/icon/DefaultAvatar/10.svg"
};
#endif // DATATYPEDEF_H
