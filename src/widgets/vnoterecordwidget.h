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

#ifndef VNOTERECORDWIDGET_H
#define VNOTERECORDWIDGET_H

#include "widgets/vnoteiconbutton.h"
#include "widgets/vnwaveform.h"
#include "common/gstreamrecorder.h"

#include <DFloatingWidget>
#include <DLabel>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class VNoteRecordWidget : public DFloatingWidget
{
    Q_OBJECT
public:
    explicit VNoteRecordWidget(QWidget *parent = nullptr);
    //开始录音
    bool startRecord();
    //设置录音设备
    void setAudioDevice(QString device);
    //取消录音
    void cancelRecord();
    //获取录音文件路径
    QString getRecordPath() const;

signals:
    void sigFinshRecord(const QString &voicePath, qint64 voiceSize);

public slots:
    //暂停录音
    void onPauseRecord();
    //继续录音
    bool onContinueRecord();
    //结束录音
    void onFinshRecord();
    //录音时长改变
    void onRecordDurationChange(qint64 duration);
    //录音数据改变
    void onAudioBufferProbed(const QAudioBuffer &buffer);

private:
    //初始化UI布局
    void initUi();
    //创建文件保存路径
    void initRecordPath();
    //初始化录音库
    void initRecord();
    //连接槽函数
    void initConnection();

private:
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_continueBtn {nullptr};
    VNoteIconButton *m_finshBtn {nullptr};
    DLabel *m_timeLabel {nullptr};
    VNWaveform *m_waveForm {nullptr};
    GstreamRecorder *m_audioRecoder {nullptr};
    QString m_recordDir {""};
    QString m_recordPath {""};
    qint64 m_recordMsec {0};
};

#endif // VNOTERECORDWIDGET_H
