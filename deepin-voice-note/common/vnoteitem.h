#ifndef VNOTEITEM_H
#define VNOTEITEM_H

#include <QtGlobal>
#include <QDateTime>

#include <DWidget>
DWIDGET_USE_NAMESPACE

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

    qint32 noteId {INVALID_ID};
    qint64 folderId {INVALID_ID};
    qint32 noteType {VNOTE_TYPE::VNT_Text};

    QString noteText;
    QString voicePath;

    QDateTime createTime;
    QDateTime modifyTime;
};

class VNoteItemWidget :public DWidget //语音文字通用接口，在子类中重写虚函数
{
    Q_OBJECT
public:
    explicit VNoteItemWidget(QWidget *parent = nullptr);
    ~VNoteItemWidget();
    virtual VNoteItem *getNoteItem();
};

#endif // VNOTEITEM_H
