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
    void delNoteData();
//    bool makeMetaData();
    void setMetadata(const QString& meta);

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
        VNT_Mixed,
    };

    qint32 noteId {INVALID_ID};
    qint64 folderId {INVALID_ID};
    qint32 noteType {VNOTE_TYPE::VNT_Text};
    qint32 noteState {State::Normal};

    QString noteTitle;

    QDateTime createTime;
    QDateTime modifyTime;
    QDateTime deleteTime;

    VNOTE_DATAS datas;
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

    virtual ~VNoteBlock();

    virtual void releaseSpecificData() = 0;

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
    virtual ~VNTextBlock() override;
    virtual void releaseSpecificData() override;

    QString content;
};

struct VNVoiceBlock : public VNoteBlock {
    VNVoiceBlock();
    virtual ~VNVoiceBlock() override;
    virtual void releaseSpecificData() override;

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
