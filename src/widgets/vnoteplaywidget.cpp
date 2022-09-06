// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnoteplaywidget.h"
#include "vnote2siconbutton.h"
#include "common/vnoteitem.h"
#include "common/utils.h"

#include <DDialogCloseButton>

#include <QGridLayout>
#include <QDebug>

/**
 * @brief VNotePlayWidget::VNotePlayWidget
 * @param parent
 */
VNotePlayWidget::VNotePlayWidget(QWidget *parent)
    : DFloatingWidget(parent)
{
    initUI();
    initPlayer();
    initConnection();
}

/**
 * @brief VNotePlayWidget::initUI
 */
void VNotePlayWidget::initUI()
{
    m_playerBtn = new VNote2SIconButton("play.svg", "pause.svg", this);
    m_playerBtn->setIconSize(QSize(28, 28));
    m_playerBtn->setFixedSize(QSize(48, 48));

    m_timeLab = new DLabel(this);
    m_timeLab->setText("00:00/00:00");
    m_timeLab->setFixedWidth(124);
    m_sliderHover = new DWidget(this);
    m_nameLab = new DLabel(m_sliderHover);
    m_nameLab->setContentsMargins(8, 0, 0, 0);
    m_slider = new DSlider(Qt::Horizontal, m_sliderHover);
    m_slider->setMinimum(0);
    m_slider->setValue(0);

    m_closeBtn = new DIconButton(DStyle::SP_CloseButton, this);
    m_closeBtn->setIconSize(QSize(26, 26));
    m_closeBtn->setFixedSize(QSize(26, 26));
    m_closeBtn->setFlat(true);
    m_closeBtn->setEnabledCircle(true);

    QVBoxLayout *sliderLayout = new QVBoxLayout;
    sliderLayout->addWidget(m_nameLab, Qt::AlignLeft);
    sliderLayout->addWidget(m_slider, Qt::AlignTop);
    sliderLayout->setSpacing(0);
    m_sliderHover->setLayout(sliderLayout);
    //语音名和进度条上下边距
    sliderLayout->setContentsMargins(0, 1, 0, 1);
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_playerBtn);
    mainLayout->addWidget(m_sliderHover);
    mainLayout->addWidget(m_timeLab, 0, Qt::AlignLeft);
    mainLayout->addWidget(m_closeBtn);
    mainLayout->setContentsMargins(13, 0, 13, 0);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    this->setLayout(mainLayout);
    m_sliderHover->installEventFilter(this);
    m_slider->installEventFilter(this);
}

/**
 * @brief VNotePlayWidget::initPlayer
 */
void VNotePlayWidget::initPlayer()
{
    m_player = new VlcPalyer(this);
}

/**
 * @brief VNotePlayWidget::initConnection
 */
void VNotePlayWidget::initConnection()
{
    connect(m_player, &VlcPalyer::positionChanged,
            this, &VNotePlayWidget::onVoicePlayPosChange, Qt::QueuedConnection);
    connect(m_player, &VlcPalyer::durationChanged,
            this, &VNotePlayWidget::onDurationChanged, Qt::QueuedConnection);
    connect(m_player, &VlcPalyer::playEnd,
            this, &VNotePlayWidget::onCloseBtnClicked, Qt::QueuedConnection);

    connect(m_playerBtn, &VNote2SIconButton::clicked,
            this, &VNotePlayWidget::onPlayerBtnClicked);

    connect(m_closeBtn, &DIconButton::clicked,
            this, &VNotePlayWidget::onCloseBtnClicked);

    connect(m_slider, &DSlider::sliderPressed,
            this, &VNotePlayWidget::onSliderPressed);
    connect(m_slider, &DSlider::sliderReleased,
            this, &VNotePlayWidget::onSliderReleased);
    connect(m_slider, &DSlider::sliderMoved,
            this, &VNotePlayWidget::onSliderMove);
}

/**
 * @brief VNotePlayWidget::onVoicePlayPosChange
 * @param pos
 */
void VNotePlayWidget::onVoicePlayPosChange(qint64 pos)
{
    if (m_sliderReleased == true) {
        onSliderMove(static_cast<int>(pos));
    }
}

/**
 * @brief VNotePlayWidget::onCloseBtnClicked
 */
void VNotePlayWidget::onCloseBtnClicked()
{
    m_slider->setValue(0);
    m_player->stop();
    m_sliderReleased = true;
    emit sigWidgetClose(m_voiceBlock);
}

/**
 * @brief VNotePlayWidget::onSliderPressed
 */
void VNotePlayWidget::onSliderPressed()
{
    m_sliderReleased = false;
}

/**
 * @brief VNotePlayWidget::onSliderReleased
 */
void VNotePlayWidget::onSliderReleased()
{
    m_sliderReleased = true;
    VlcPalyer::VlcState state = m_player->getState();
    if (state == VlcPalyer::Playing || state == VlcPalyer::Paused) {
        int pos = m_slider->value();
        if (pos >= m_slider->maximum()) {
            onCloseBtnClicked();
        } else {
            if (m_player->getState() == VlcPalyer::Playing) {
                m_player->setPosition(pos);
            }
        }
    }
}

/**
 * @brief VNotePlayWidget::onSliderMove
 * @param pos
 */
void VNotePlayWidget::onSliderMove(int pos)
{
    if (m_voiceBlock) {
        qint64 tmpPos = pos > m_voiceBlock->voiceSize ? m_voiceBlock->voiceSize : pos;
        m_timeLab->setText(Utils::formatMillisecond(tmpPos, 0) + "/" + Utils::formatMillisecond(m_voiceBlock->voiceSize));
    }

    if (m_sliderReleased == true) {
        m_slider->setValue(pos);
    }
}

/**
 * @brief VNotePlayWidget::eventFilter
 * @param o
 * @param e
 * @return false 不过滤
 */
bool VNotePlayWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_slider && e->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_Left) {
            int pos = m_slider->value() - 5000;
            if (pos < 0) {
                pos = 0;
            }
            if (m_player->getState() == VlcPalyer::Playing) {
                m_player->setPosition(pos);
            } else {
                onSliderMove(pos);
            }
        } else if (keyEvent->key() == Qt::Key_Right) {
            int pos = m_slider->value() + 5000;
            if (pos > m_slider->maximum()) {
                pos = m_slider->maximum();
            }
            if (m_player->getState() == VlcPalyer::Playing) {
                m_player->setPosition(pos);
            } else {
                onSliderMove(pos);
            }
        } else {
            return false;
        }
        return true;
    } else {
        if (e->type() == QEvent::Enter) {
            m_nameLab->setVisible(false);
        } else if (e->type() == QEvent::Leave) {
            m_nameLab->setVisible(true);
        }
    }
    return false;
}

/**
 * @brief VNotePlayWidget::getPlayerStatus
 * @return 播放状态
 */
VlcPalyer::VlcState VNotePlayWidget::getPlayerStatus()
{
    return m_player->getState();
}

/**
 * @brief VNotePlayWidget::onDurationChanged
 * @param duration
 */
void VNotePlayWidget::onDurationChanged(qint64 duration)
{
    if (duration && m_slider->maximum() != duration) {
        m_slider->setMaximum(static_cast<int>(duration));
    }
}

void VNotePlayWidget::onPlayerBtnClicked()
{
    playVoice(m_voiceBlock, true);
}

void VNotePlayWidget::playVoice(VNVoiceBlock *voiceData, bool bIsSame)
{
    if (bIsSame && nullptr != m_voiceBlock) { //与上一次语音相同，执行继续/暂停操作
        VlcPalyer::VlcState status = getPlayerStatus();
        if (status == VlcPalyer::Playing) { //当前语音播放中则执行暂停操作
            m_player->pause();
            m_playerBtn->setState(VNote2SIconButton::Normal); //当前语音暂停中则执行播放操作
            emit sigPauseVoice(m_voiceBlock);
        } else if (status == VlcPalyer::Paused) {
            m_player->play();
            m_playerBtn->setState(VNote2SIconButton::Press);
            emit sigPlayVoice(m_voiceBlock);
        }
    } else if (nullptr != voiceData) { //与上一次语音不相同，重新播放语音
        m_slider->setValue(0);
        m_voiceBlock = voiceData;
        m_player->setFilePath(m_voiceBlock->voicePath);
        m_nameLab->setText(voiceData->voiceTitle);
        m_timeLab->setText(Utils::formatMillisecond(0, 0) + "/" + Utils::formatMillisecond(voiceData->voiceSize));
        m_playerBtn->setState(VNote2SIconButton::Press);
        m_player->play();
        emit sigPlayVoice(m_voiceBlock);
    } else {
        qInfo() << "paly voice param is error";
    }
}
