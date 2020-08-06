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
    bool startRecord();
    void setAudioDevice(QString device);
    void cancelRecord();
    QString getRecordPath() const;

signals:
    void sigFinshRecord(const QString &voicePath,qint64 voiceSize);

public slots:
    void onPauseRecord();
    bool onContinueRecord();
    void onFinshRecord();
    void onRecordDurationChange(qint64 duration);
    void onAudioBufferProbed(const QAudioBuffer &buffer);
private:
    void initUi();
    void initRecordPath();
    void initRecord();
    void initConnection();
private:
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_continueBtn {nullptr};
    VNoteIconButton *m_finshBtn {nullptr};
    DLabel          *m_timeLabel{nullptr};
    VNWaveform      *m_waveForm {nullptr};
    GstreamRecorder *m_audioRecoder{nullptr};
    QString          m_recordDir {""};
    QString          m_recordPath {""};
    qint64           m_recordMsec {0};
};

#endif // VNOTERECORDWIDGET_H
