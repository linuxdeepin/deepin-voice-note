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

    enum State {
        Normal,
        Deleted,
    };

    qint64  id {INVALID_ID};
    qint32  category {0};
    qint64  notesCount {0};
    qint32  defaultIcon {0};
    qint32  folder_state {State::Normal};

    QString name;
    QString iconPath;

    QDateTime createTime;
    QDateTime modifyTime;
    QDateTime deleteTime;

    struct {
        QImage icon;
    } UI;

    qint32& maxNoteIdRef();

    friend QDebug & operator << (QDebug &out, VNoteFolder &folder);
protected:
    bool fIsDataLoaded {false};

    //Current max note id in the folder,auto
    //increament
    qint32 maxNoteId {0};

    VNOTE_ITEMS_MAP *notes{nullptr};

    friend class VNoteFolderOper;
};

#endif // VNOTEFORLDER_H
