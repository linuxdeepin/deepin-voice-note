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

#ifndef VNOTERECORDBAR_H
#define VNOTERECORDBAR_H

#include <QWidget>
#include <QStackedLayout>

#include <DAnchors>
#include <DFloatingMessage>

DWIDGET_USE_NAMESPACE
class AudioWatcher;
class VNotePlayWidget;
class VNoteRecordWidget;
class VNoteIconButton;
struct VNVoiceBlock;

class VNoteRecordBar : public QWidget
{
    Q_OBJECT
public:
    explicit VNoteRecordBar(QWidget *parent = nullptr);

    static constexpr int REC_BTN_W = 68;
    static constexpr int REC_BTN_H = 68;
    //停止录音
    void stopRecord();
    //播放语音
    void playVoice(const VNVoiceBlock *voiceData, const bool &bIsSame);
private:
    //初始化设备检测
    void initAudioWatcher();
    //初始化参数配置
    void initSetting();
    //初始化UI布局
    void initUI();
    //连接槽函数
    void initConnections();
    //判断录音音量是否过低
    bool volumeToolow(const double &volume);

signals:
    //录音信号
    void sigStartRecord(const QString &recordPath);
    void sigFinshRecord(const QString &voicePath, qint64 voiceSize);
    //播放信号
    void sigPlayVoice();
    void sigPauseVoice();
    void sigStopVoice();
    //设备异常提示
    void sigDeviceExceptionMsgShow();
    void sigDeviceExceptionMsgClose();

public slots:
    //开始录音
    void onStartRecord();
    //结束录音
    void onFinshRecord(const QString &voicePath, qint64 voiceSize);
    //播放窗口关闭
    void onStopVoice();
    //设备音量改变
    void onAudioVolumeChange(int mode);
    //设备改变
    void onAudioDeviceChange(int mode);
    //输入/输出设备切换
    void onAudioSelectChange(QVariant value);

protected:
    //事件过滤器
    bool eventFilter(QObject *o, QEvent *e) override;
    //开始录音
    void startRecord();

protected:
    QStackedLayout *m_mainLayout {nullptr};
    VNotePlayWidget *m_playPanel {nullptr};
    VNoteRecordWidget *m_recordPanel {nullptr};
    VNoteIconButton *m_recordBtn {nullptr};
    QWidget *m_recordBtnHover {nullptr};
    QScopedPointer<DAnchorsBase> m_recBtnAnchor;

    QString m_recordPath {""};
    int m_currentMode {0};
    AudioWatcher *m_audioWatcher {nullptr};
    bool m_showVolumeWanning {false};
};

#endif // VNOTERECORDBAR_H
