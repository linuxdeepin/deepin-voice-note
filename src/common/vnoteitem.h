/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef VNOTEITEM_H
#define VNOTEITEM_H

#include "common/datatypedef.h"

#include <DWidget>

#include <QDateTime>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextDocument>

DWIDGET_USE_NAMESPACE

struct VNoteBlock;
struct VNoteFolder;

struct VNoteItem {
public:
    VNoteItem();
    //是否可用
    bool isValid();
    //删除数据
    void delNoteData();
    //查找数据
    bool search(const QString &keyword);
    //源数据设置
    void setMetadata(const QVariant &meta);
    //绑定记事本项
    void setFolder(VNoteFolder *folder);
    //获取记事本项
    VNoteFolder *folder() const;

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
    //记事项id
    qint32 noteId {INVALID_ID};
    //记事本id
    qint64 folderId {INVALID_ID};
    //数据类型
    qint32 noteType {VNOTE_TYPE::VNT_Text};
    //状态
    qint32 noteState {State::Normal};
    //是否置顶
    qint32 isTop {0};
    //标题名称
    QString noteTitle {""};
    //创建时间
    QDateTime createTime;
    //修改时间
    QDateTime modifyTime;
    //删除时间
    QDateTime deleteTime;
    //获取元数据
    QVariant &metaDataRef();
    const QVariant &metaDataConstRef() const;
    //获取语音最大id值
    qint32 &maxVoiceIdRef();
    qint32 voiceMaxId() const;

    VNOTE_DATAS datas;
    //创建数据块
    VNoteBlock *newBlock(int type);
    //添加数据块
    void addBlock(VNoteBlock *block);
    //指定数据块后面插入数据块
    void addBlock(VNoteBlock *before, VNoteBlock *block);
    //删除数据块
    void delBlock(VNoteBlock *block);
    //是否有语音项
    bool haveVoice() const;
    //是否有文本项
    bool haveText() const;
    //语音数目
    qint32 voiceCount() const;

protected:
    QVariant metaData;

    //Use to make default voice name
    //auto increment.
    qint32 maxVoiceId {0};

    //TODO:
    //    Don't used now ,Used for quick lookup.
    VNoteFolder *ownFolder {nullptr};

    friend QDebug &operator<<(QDebug &out, VNoteItem &noteItem);
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
    //释放资源，语音时用于删除文件
    virtual void releaseSpecificData() = 0;
    //获取数据类型
    qint32 getType();

    qint32 blockType {InValid};
    /*
     * Comment:
     *      For text block, store the content of the text,
     * for voice block, store the result of audio to text.
     * */
    QString blockText;

    union {
        VNoteBlock *ptrBlock;
        VNTextBlock *ptrText;
        VNVoiceBlock *ptrVoice;
    };

protected:
    explicit VNoteBlock(qint32 type = InValid);
    explicit VNoteBlock(const VNoteBlock &);
    const VNoteBlock &operator=(const VNoteBlock &);
};

//文本内容
struct VNTextBlock : public VNoteBlock {
    VNTextBlock();
    virtual ~VNTextBlock() override;
    virtual void releaseSpecificData() override;
};

//语音内容
struct VNVoiceBlock : public VNoteBlock {
    VNVoiceBlock();
    virtual ~VNVoiceBlock() override;
    virtual void releaseSpecificData() override;

    QString voicePath {""};
    qint64 voiceSize {0};
    QString voiceTitle {""};
    bool state {false};
    QDateTime createTime;
};

//详情页编辑器，语音插件的基类
class DetailItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DetailItemWidget(QWidget *parent = nullptr);
    //查找高亮内容
    virtual void updateSearchKey(QString searchKey);
    //粘贴
    virtual void pasteText();
    //鼠标点是否在文字区域，语音插件用于判断是否在语音转文字区域
    virtual bool isTextContainsPos(const QPoint &globalPos);
    //获取数据块
    virtual VNoteBlock *getNoteBlock() = 0;
    //获取光标
    virtual QTextCursor getTextCursor() = 0;
    //设置光标
    virtual void setTextCursor(const QTextCursor &cursor) = 0;
    //文本是否为空
    virtual bool textIsEmpty() = 0;
    //获取光标区域
    virtual QRect getCursorRect() = 0;
    //设置焦点
    virtual void setFocus(bool hasVoice) = 0;
    //判断是否有焦点
    virtual bool hasFocus() = 0;
    //选中文本
    virtual void selectText(const QPoint &globalPos, QTextCursor::MoveOperation op) = 0;
    virtual void selectText(QTextCursor::MoveOperation op) = 0;
    //删除选中文本
    virtual void removeSelectText() = 0;
    //整个选中
    virtual void selectAllText() = 0;
    //清除选中
    virtual void clearSelection() = 0;
    //是否有选中
    virtual bool hasSelection() = 0;
    //是否全部选中
    virtual bool isSelectAll() = 0;
    //获取选中内容片段
    virtual QTextDocumentFragment getSelectFragment() = 0;
    //获取整个内容
    virtual QTextDocument *getTextDocument() = 0;
};

#endif // VNOTEITEM_H
