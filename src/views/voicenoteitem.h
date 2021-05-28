/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
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

#ifndef VOICENOTEITEM_H
#define VOICENOTEITEM_H
#include "common/vnoteitem.h"

#include <DWidget>
#include <DFrame>
#include <DLabel>
#include <DMenu>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE

class TextNoteEdit;
class VNote2SIconButton;

//Playing animation interface
class PlayAnimInferface
{
public:
    virtual ~PlayAnimInferface();

    virtual void startAnim();
    virtual void stopAnim();
    void setAnimTimer(QTimer *timer);
    virtual void updateAnim() = 0;

protected:
    qint32 m_animPicIndex {0};
    QTimer *m_refreshTimer {nullptr};

    const QVector<QString> m_playBitmap = {
        "play_voice1.svg",
        "play_voice2.svg",
        "play_voice3.svg",
        "play_voice4.svg",
    };
};

class VoiceNoteItem : public DetailItemWidget
    , public PlayAnimInferface
{
    Q_OBJECT
public:
    explicit VoiceNoteItem(VNoteBlock *noteBlock, QWidget *parent = nullptr);

    //UI同步数据
    void initData();
    //显示播放按钮
    void showPlayBtn();
    //显示暂停按钮
    void showPauseBtn();
    //显示转文字开始提示内容
    void showAsrStartWindow();
    //显示转文字结果
    void showAsrEndWindow(const QString &strResult);
    //设置播放按钮是否可用
    void enblePlayBtn(bool enable);
    //转文字内容是否为空
    bool asrTextNotEmpty();
    //开始播放动画
    void updateAnim() override;
    //停止播放动画
    void stopAnim() override;
    //重写基类函数
    VNoteBlock *getNoteBlock() override;
    QTextCursor getTextCursor() override;
    void setTextCursor(const QTextCursor &cursor) override;
    bool textIsEmpty() override;
    QRect getCursorRect() override;
    //选中操作相关
    void selectText(const QPoint &globalPos, QTextCursor::MoveOperation op) override;
    void selectText(QTextCursor::MoveOperation op) override;
    void removeSelectText() override;
    void selectAllText() override;
    void clearSelection() override;
    void setFocus(bool hasVoice) override;
    bool hasFocus() override;
    bool hasSelection() override;
    bool isSelectAll() override;
    bool isTextContainsPos(const QPoint &globalPos) override;
    QTextDocumentFragment getSelectFragment() override;
    QTextDocument *getTextDocument() override;

signals:
    //播放相关信号
    void sigPlayBtnClicked(VoiceNoteItem *item);
    void sigPauseBtnClicked(VoiceNoteItem *item);
    //光标坐标改变
    void sigCursorHeightChange(DetailItemWidget *widget, int height);

public slots:
    //播放/暂停
    void onPlayBtnClicked();
    //转文字内容改变
    void onAsrTextChange();
    //主题切换
    void onChangeTheme();
    //刷新创建时间
    void refreshCreateTime();

private:
    void initUi();
    void initConnection();
    bool m_adjustBar {true};
    bool m_selectAll {false};
    DLabel *m_hornLab {nullptr};
    DLabel *m_createTimeLab {nullptr};
    DLabel *m_voiceSizeLab {nullptr};
    DLabel *m_voiceNameLab {nullptr};
    DFrame *m_bgWidget {nullptr};
    TextNoteEdit *m_asrText {nullptr};
    VNoteBlock *m_noteBlock {nullptr};
    VNote2SIconButton *m_playBtn {nullptr};
    DFrame *m_coverWidget {nullptr};
};

#endif // VOICENOTEITEM_H
