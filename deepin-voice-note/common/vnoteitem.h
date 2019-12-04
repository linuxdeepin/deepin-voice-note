#ifndef VNOTEITEM_H
#define VNOTEITEM_H

#include <QtGlobal>
#include <QDateTime>

struct VNoteItem
{
public:
    VNoteItem();

    bool isValid();

    enum {
        INVALID_ID = -1
    };

    enum VNOTE_TYPE {
        VNT_Text = 0,
        VNT_Voice,
    };

    qint64 noteId {INVALID_ID};
    qint64 folderId {INVALID_ID};
    qint8  noteType {VNOTE_TYPE::VNT_Text};

    QString noteText;
    QString voicePath;

    QDateTime createTime;
    QDateTime modifyTime;
};
#endif // VNOTEITEM_H
