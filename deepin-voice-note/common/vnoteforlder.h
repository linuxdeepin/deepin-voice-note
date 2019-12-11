#ifndef VNOTEFORLDER_H
#define VNOTEFORLDER_H

#include "datatypedef.h"

#include <QtGlobal>
#include <QDateTime>
#include <QImage>
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
    qint32  defaultIcon {0};
    QString iconPath;

    QDateTime createTime;
    QDateTime modifyTime;

    struct{
        QImage icon;
    }UI;

protected:
    bool fIsDataLoaded {false};

    VNOTE_ITEMS_MAP *notes{nullptr};

    friend class VNoteFolderOper;
};

#endif // VNOTEFORLDER_H
