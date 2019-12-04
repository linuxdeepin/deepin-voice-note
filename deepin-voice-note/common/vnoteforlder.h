#ifndef VNOTEFORLDER_H
#define VNOTEFORLDER_H

#include "datatypedef.h"

#include <QtGlobal>
#include <QDateTime>

struct VNoteFolder
{
public:
    VNoteFolder();
    ~VNoteFolder();

    bool isValid();

    enum {
        INVALID_ID = -1
    };

    qint64  id {INVALID_ID};
    qint64  notesCount {0};
    QString name;
    QString iconPath;

    QDateTime createTime;
    QDateTime modifyTime;

    bool fIsloaded {false};
    VNOTE_ITEMS_MAP *notes{nullptr};
};

#endif // VNOTEFORLDER_H
