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
    void cancelRecord();
    void playOrPauseVoice();
    void playVoice(VNVoiceBlock *voiceData);
    void pauseVoice(VNVoiceBlock *voiceData);
    bool stopVoice(VNVoiceBlock *voiceData);
    VNVoiceBlock *getVoiceData();

private:
    void initAudioWatcher();
    void initSetting();
    void initUI();
    void initConnections();
    bool volumeToolow(const double &volume);

signals:
    void sigStartRecord(const QString &recordPath);
    void sigFinshRecord(const QString &voicePath, qint64 voiceSize);
    void sigPlayVoice(VNVoiceBlock *voiceData);
    void sigPauseVoice(VNVoiceBlock *voiceData);
    void sigWidgetClose(VNVoiceBlock *voiceData);
    void sigDeviceExceptionMsgShow();
    void sigDeviceExceptionMsgClose();

public slots:
    void onStartRecord();
    void onFinshRecord(const QString &voicePath, qint64 voiceSize);
    void onClosePlayWidget(VNVoiceBlock *voiceData);
    void onAudioVolumeChange(int mode);
    void onAudioDeviceChange(int mode);
    void onAudioSelectChange(QVariant value);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
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
