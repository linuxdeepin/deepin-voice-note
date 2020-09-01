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

#ifndef VNOTEPLAYWIDGET_H
#define VNOTEPLAYWIDGET_H

#include "common/vlcpalyer.h"

#include <DFloatingWidget>
#include <DLabel>
#include <DSlider>
#include <DIconButton>
#include <DWidget>

DWIDGET_USE_NAMESPACE

struct VNVoiceBlock;
class VNoteIconButton;
class VNote2SIconButton;

class QPainter;
class QWidget;
class VNotePlayWidget : public DFloatingWidget
{
    Q_OBJECT
public:
    explicit VNotePlayWidget(QWidget *parent = nullptr);
    //绑定数据
    void setVoiceBlock(VNVoiceBlock *voiceData);
    //暂停播放
    void pauseVideo();
    //播放
    void playVideo();
    //停止播放
    void stopVideo();
    //获取状态
    VlcPalyer::VlcState getPlayerStatus();
    //获取绑定的数据
    VNVoiceBlock *getVoiceData();
signals:
    void sigPlayVoice(VNVoiceBlock *voiceData);
    void sigPauseVoice(VNVoiceBlock *voiceData);
    void sigWidgetClose(VNVoiceBlock *voiceData);
public slots:
    //当前播放位置改变
    void onVoicePlayPosChange(qint64 pos);
    //进度条单击
    void onSliderPressed();
    //进度条单击释放
    void onSliderReleased();
    //进度条移动
    void onSliderMove(int pos);
    //播放
    void onPlayBtnClicked();
    //暂停
    void onPauseBtnClicked();
    //播放/暂停
    void onPlayerBtnClicked();
    //播放结束，关闭播放窗口
    void onCloseBtnClicked();
    //播放文件总时长改变
    void onDurationChanged(qint64 duration);

protected:
    //事件过滤器
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void initUI();
    void initConnection();
    //初始化播放库
    void initPlayer();
    bool m_sliderReleased {true};
    DLabel *m_timeLab {nullptr};
    DLabel *m_nameLab {nullptr};
    DSlider *m_slider {nullptr};
    DWidget *m_sliderHover {nullptr};
    DIconButton *m_closeBtn {nullptr};
    VNVoiceBlock *m_voiceBlock {nullptr};
    VlcPalyer *m_player {nullptr};
    VNote2SIconButton *m_playerBtn {nullptr};
};

#endif // VNOTEPLAYWIDGET_H
