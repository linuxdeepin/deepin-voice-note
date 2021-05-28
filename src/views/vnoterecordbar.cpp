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

#include "vnoterecordbar.h"
#include "globaldef.h"
#include "widgets/vnoterecordwidget.h"
#include "widgets/vnote2siconbutton.h"
#include "widgets/vnoteplaywidget.h"
#include "common/vnoteitem.h"
#include "common/audiowatcher.h"
#include "common/setting.h"

#include "dialog/vnotemessagedialog.h"

#include <DApplication>
#include <DLog>

#include <QVBoxLayout>

/**
 * @brief VNoteRecordBar::VNoteRecordBar
 * @param parent
 */
VNoteRecordBar::VNoteRecordBar(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    initConnections();
    initSetting();
    initAudioWatcher();
    onAudioDeviceChange(m_currentMode);
}

/**
 * @brief VNoteRecordBar::initUI
 */
void VNoteRecordBar::initUI()
{
    m_mainLayout = new QStackedLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_recordPanel = new VNoteRecordWidget(this);
    m_recordPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_mainLayout->addWidget(m_recordPanel);

    m_recordBtnHover = new DWidget(this);
    m_recordBtn = new VNote2SIconButton("circle_record.svg", "circle_record.svg", m_recordBtnHover);
    m_recordBtn->setFlat(true);
    m_recordBtn->setIconSize(QSize(34, 34));
    m_recordBtn->setFixedSize(QSize(54, 54));
    QGridLayout *recordBtnHoverLayout = new QGridLayout;
    recordBtnHoverLayout->setContentsMargins(0, 8, 0, 0);
    recordBtnHoverLayout->addWidget(m_recordBtn, 0, 1);
    recordBtnHoverLayout->setColumnStretch(0, 1);
    recordBtnHoverLayout->setColumnStretch(1, 0);
    recordBtnHoverLayout->setColumnStretch(2, 1);
    m_recordBtnHover->setLayout(recordBtnHoverLayout);
    m_mainLayout->addWidget(m_recordBtnHover);

    m_playPanel = new VNotePlayWidget(this);
    m_mainLayout->addWidget(m_playPanel);
    onChangeTheme();
    m_mainLayout->setCurrentWidget(m_recordBtnHover);
}

/**
 * @brief VNoteRecordBar::initConnections
 */
void VNoteRecordBar::initConnections()
{
    //Install filter to filte MousePress
    //event.
    installEventFilter(this);
    m_recordBtn->installEventFilter(this);

    connect(m_recordBtn, &VNote2SIconButton::clicked, this, &VNoteRecordBar::onStartRecord);
    connect(m_recordPanel, SIGNAL(sigFinshRecord(const QString &, qint64)),
            this, SLOT(onFinshRecord(const QString &, qint64)));
    connect(m_playPanel, &VNotePlayWidget::sigWidgetClose,
            this, &VNoteRecordBar::onClosePlayWidget);
    connect(m_playPanel, SIGNAL(sigPlayVoice(VNVoiceBlock *)),
            this, SIGNAL(sigPlayVoice(VNVoiceBlock *)));
    connect(m_playPanel, SIGNAL(sigPauseVoice(VNVoiceBlock *)),
            this, SIGNAL(sigPauseVoice(VNVoiceBlock *)));
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged,
            this, &VNoteRecordBar::onChangeTheme);
}

/**
 * @brief VNoteRecordBar::eventFilter
 * @param o
 * @param e
 * @return false 不过滤事件
 */
bool VNoteRecordBar::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);
    //Let NoteItem lost focus when click
    //outside of Note

    if (e->type() == QEvent::MouseButtonPress) {
        setFocus(Qt::MouseFocusReason);
    }

    return false;
}

/**
 * @brief VNoteRecordBar::startRecord
 */
void VNoteRecordBar::startRecord()
{
    m_mainLayout->setCurrentWidget(m_recordPanel);
    if (m_recordPanel->startRecord()) {
        m_recordPanel->setFocus();
        emit sigStartRecord(m_recordPanel->getRecordPath());
    }
}

/**
 * @brief VNoteRecordBar::onStartRecord
 */
void VNoteRecordBar::onStartRecord()
{
    if (this->isVisible()
        && m_mainLayout->currentWidget() == m_recordBtnHover
        && m_recordBtn->isEnabled()) {
        m_recordPanel->setAudioDevice(m_audioWatcher->getDeviceName(
            static_cast<AudioWatcher::AudioMode>(m_currentMode)));
        double volume = m_audioWatcher->getVolume(
            static_cast<AudioWatcher::AudioMode>(m_currentMode));
        m_showVolumeWanning = volumeToolow(volume);
        if (m_showVolumeWanning) {
            VNoteMessageDialog volumeLowDialog(VNoteMessageDialog::VolumeTooLow, this);
            connect(&volumeLowDialog, &VNoteMessageDialog::accepted, this, [this]() {
                //User confirmed record when volume too low
                //start recording anyway.
                startRecord();
            });

            volumeLowDialog.exec();
        } else {
            //Volume normal
            startRecord();
        }
    }
}

/**
 * @brief VNoteRecordBar::onFinshRecord
 * @param voicePath
 * @param voiceSize
 */
void VNoteRecordBar::onFinshRecord(const QString &voicePath, qint64 voiceSize)
{
    m_mainLayout->setCurrentWidget(m_recordBtnHover);
    emit sigFinshRecord(voicePath, voiceSize);
}

/**
 * @brief VNoteRecordBar::cancelRecord
 */
void VNoteRecordBar::stopRecord()
{
    m_recordPanel->stopRecord();
}

/**
 * @brief VNoteRecordBar::onClosePlayWidget
 * @param voiceData
 */
void VNoteRecordBar::onClosePlayWidget(VNVoiceBlock *voiceData)
{
    m_mainLayout->setCurrentWidget(m_recordBtnHover);
    emit sigWidgetClose(voiceData);
}

/**
 * @brief VNoteRecordBar::playVoice
 * @param voiceData
 */
void VNoteRecordBar::playVoice(VNVoiceBlock *voiceData)
{
    m_mainLayout->setCurrentWidget(m_playPanel);
    m_playPanel->setVoiceBlock(voiceData);
}

/**
 * @brief VNoteRecordBar::pauseVoice
 * @param voiceData
 */
void VNoteRecordBar::pauseVoice(VNVoiceBlock *voiceData)
{
    if (m_mainLayout->currentWidget() == m_playPanel
        && m_playPanel->getVoiceData() == voiceData) {
        m_playPanel->onPauseBtnClicked();
    }
}

/**
 * @brief VNoteRecordBar::stopVoice
 * @param voiceData
 * @return true 成功
 */
bool VNoteRecordBar::stopVoice(VNVoiceBlock *voiceData)
{
    if (m_mainLayout->currentWidget() == m_playPanel
        && m_playPanel->getVoiceData() == voiceData) {
        m_playPanel->stopVideo();
        m_mainLayout->setCurrentWidget(m_recordBtnHover);
        return true;
    }
    return false;
}

/**
 * @brief VNoteRecordBar::getVoiceData
 * @return 绑定的播放语音数据
 */
VNVoiceBlock *VNoteRecordBar::getVoiceData()
{
    if (m_mainLayout->currentWidget() == m_playPanel) {
        return m_playPanel->getVoiceData();
    }
    return nullptr;
}

/**
 * @brief VNoteRecordBar::playOrPauseVoice
 */
void VNoteRecordBar::playOrPauseVoice()
{
    if (m_mainLayout->currentWidget() == m_playPanel) {
        VlcPalyer::VlcState status = m_playPanel->getPlayerStatus();
        if (status == VlcPalyer::Playing) {
            m_playPanel->onPauseBtnClicked();
        } else if (status == VlcPalyer::Paused) {
            m_playPanel->onPlayBtnClicked();
        }
    }
}

/**
 * @brief VNoteRecordBar::onAudioVolumeChange
 * @param mode
 */
void VNoteRecordBar::onAudioVolumeChange(int mode)
{
    if (m_mainLayout->currentWidget() == m_recordPanel
        && m_currentMode == mode) {
        double volume = m_audioWatcher->getVolume(
            static_cast<AudioWatcher::AudioMode>(mode));
        if (!m_showVolumeWanning) {
            m_showVolumeWanning = volumeToolow(volume);
            if (m_showVolumeWanning) {
                VNoteMessageDialog volumeLowDialog(VNoteMessageDialog::VolumeTooLow, this);
                connect(&volumeLowDialog, &VNoteMessageDialog::rejected, this, [this]() {
                    stopRecord();
                });

                volumeLowDialog.exec();
            }
        } else {
            if (!volumeToolow(volume)) {
                m_showVolumeWanning = false;
            }
        }
    }
}

/**
 * @brief VNoteRecordBar::onAudioDeviceChange
 * @param mode
 */
void VNoteRecordBar::onAudioDeviceChange(int mode)
{
    if (m_currentMode == mode) {
        static bool msgshow = false;
        QString info = m_audioWatcher->getDeviceName(
            static_cast<AudioWatcher::AudioMode>(mode));
        if (info.isEmpty()) { //切换后的设备异常
            m_recordBtn->setEnabled(false);
            m_recordBtn->setToolTip(
                DApplication::translate(
                    "VNoteRecordBar",
                    "No recording device detected"));
            if (m_mainLayout->currentWidget() == m_recordPanel) {
                stopRecord();
                if (!msgshow) {
                    emit sigDeviceExceptionMsgShow();
                    msgshow = true;
                }
            }
        } else { //切换后的设备正常
            if (msgshow) {
                emit sigDeviceExceptionMsgClose();
                msgshow = false;
            }
            m_recordBtn->setEnabled(true);
            m_recordBtn->setToolTip("");
            if (m_mainLayout->currentWidget() == m_recordPanel) {
                stopRecord();
            }
        }
    }
}

/**
 * @brief VNoteRecordBar::onAudioSelectChange
 * @param value
 */
void VNoteRecordBar::onAudioSelectChange(QVariant value)
{
    m_currentMode = value.toInt();
    onAudioDeviceChange(m_currentMode);
}

/**
 * @brief VNoteRecordBar::initSetting
 */
void VNoteRecordBar::initSetting()
{
    m_currentMode = setting::instance()->getOption(VNOTE_AUDIO_SELECT).toInt();
    auto option = setting::instance()->getSetting()->option(VNOTE_AUDIO_SELECT);
    connect(option, &DSettingsOption::valueChanged,
            this, &VNoteRecordBar::onAudioSelectChange);
}

/**
 * @brief VNoteRecordBar::initAudioWatcher
 */
void VNoteRecordBar::initAudioWatcher()
{
    m_audioWatcher = new AudioWatcher(this);
    connect(m_audioWatcher, &AudioWatcher::sigDeviceChange,
            this, &VNoteRecordBar::onAudioDeviceChange);
    connect(m_audioWatcher, &AudioWatcher::sigVolumeChange,
            this, &VNoteRecordBar::onAudioVolumeChange);
}

/**
 * @brief VNoteRecordBar::volumeToolow
 * @param volume
 * @return true 录音音量低
 */
bool VNoteRecordBar::volumeToolow(const double &volume)
{
    return (volume - 0.2 < 0.0) ? true : false;
}

void VNoteRecordBar::onChangeTheme()
{
    DPalette palette = DApplicationHelper::instance()->palette(m_recordBtn);
    DGuiApplicationHelper::ColorType theme =
        DGuiApplicationHelper::instance()->themeType();
    QColor highColor = theme == DGuiApplicationHelper::DarkType ? "#B82222" : "#F45959";
    palette.setBrush(DPalette::Highlight, highColor);
    highColor.setAlphaF(0.4);
    palette.setColor(DPalette::Disabled, DPalette::Highlight, highColor);
    m_recordBtn->setPalette(palette);
}
