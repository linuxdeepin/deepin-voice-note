#ifndef VNOTEITEM_H
#define VNOTEITEM_H

#include "common/datatypedef.h"

#include <QDateTime>

#include <DWidget>

DWIDGET_USE_NAMESPACE

struct VNoteBlock;

struct VNoteItem
{
public:
    VNoteItem();

    bool isValid();
    void delVoiceFile();
    bool makeMetaData();

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

    VNoteDatas datas;
protected:
    QString metaData;

    friend QDebug& operator << (QDebug& out, VNoteItem &noteItem);
};

struct VNTextBlock;
struct VNVoiceBlock;

struct VNoteBlock {
    enum {
        InValid,
        Text,
        Voice
    };

    qint32 getType();

    qint32 blockType {InValid};

    union {
        VNoteBlock*   ptrBlock;
        VNTextBlock*  ptrText;
        VNVoiceBlock* ptrVoice;
    };
protected:
    VNoteBlock(qint32 type = InValid);
    VNoteBlock( const VNoteBlock& );
    const VNoteBlock& operator=( const VNoteBlock& );
};

struct VNTextBlock  : public VNoteBlock {
    VNTextBlock();

    QString content;
};

struct VNVoiceBlock : public VNoteBlock {
    VNVoiceBlock();

    QString path;
    qint64  voiceSize;
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
