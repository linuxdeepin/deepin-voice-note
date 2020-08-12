/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#ifndef GSTREAMRECORDER_H
#define GSTREAMRECORDER_H

#include <QObject>
#include <QMutex>
#include <QAudioBuffer>
#include <QAudioDeviceInfo>

#include <gst/gst.h>

class GstreamRecorder : public QObject
{
    Q_OBJECT
public:
    explicit GstreamRecorder(QObject *parent = nullptr);
    ~GstreamRecorder();

    bool startRecord();
    void pauseRecord();
    void stopRecord();

    void setDevice(QString device);
    void setOutputFile(QString path);

    bool doBusMessage(GstMessage *message);
    bool doBufferProbe(GstBuffer *buffer);

private slots:
    void bufferProbed();
Q_SIGNALS:
    void errorMsg(QString msg);
    void audioBufferProbed(const QAudioBuffer &buffer);

private:
    bool createPipe();
    void objectUnref(gpointer object);
    void setStateToNull();
    void initFormat();
    void deinit();
    void GetGstState(int *state, int *pending);

    GstElement *m_pipeline {nullptr};
    QString m_outputFile {""};
    QString m_currentDevice;
    QAudioBuffer m_pendingBuffer;
    QMutex m_bufferMutex;
    QAudioFormat m_format;
};

#endif // GSTREAMRECORDER_H
