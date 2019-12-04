#ifndef DATATYPEDEF_H
#define DATATYPEDEF_H

#include <QMap>

struct VNoteFolder;
struct VNoteItem;

typedef QMap<qint64,VNoteFolder*> VNOTE_FOLDERS_MAP;
typedef QMap<qint64,VNoteItem*>   VNOTE_ITEMS_MAP;

#endif // DATATYPEDEF_H
