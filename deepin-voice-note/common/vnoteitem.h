#ifndef VNOTEITEM_H
#define VNOTEITEM_H

#include <QDateTime>

#include <DWidget>

DWIDGET_USE_NAMESPACE

struct VNoteItem
{
public:
    VNoteItem();

    bool isValid();
    void delVoiceFile();

    enum {
        INVALID_ID = -1
    };

    enum State {
        Normal,
        Deleted,
    };

    enum VNOTE_TYPE {
        VNT_Text = 0,
        VNT_Voice,
    };

    qint32 noteId {INVALID_ID};
    qint64 folderId {INVALID_ID};
    qint32 noteType {VNOTE_TYPE::VNT_Text};
    qint64 voiceSize {0};
    qint32 noteState {State::Normal};

    QString noteTitle;
    QString noteText;
    QString voicePath;

    QDateTime createTime;
    QDateTime modifyTime;
    QDateTime deleteTime;

    friend QDebug& operator << (QDebug& out, VNoteItem &noteItem);
};

class VNoteItemWidget :public DWidget //语音文字通用接口
{
    Q_OBJECT
public:
    explicit VNoteItemWidget(QWidget *parent = nullptr);
    virtual VNoteItem *getNoteItem() = 0;
    virtual void updateData() = 0; //更新数据
};

#endif // VNOTEITEM_H
