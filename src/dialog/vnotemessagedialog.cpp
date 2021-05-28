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

#include "vnotemessagedialog.h"

#include <DVerticalLine>
#include <DApplication>
#include <DFontSizeManager>
#include <DApplicationHelper>

#include <QVBoxLayout>

/**
 * @brief VNoteMessageDialog::VNoteMessageDialog
 * @param msgType 窗口类型
 * @param parent
 */
VNoteMessageDialog::VNoteMessageDialog(int msgType, QWidget *parent, int notesCount)
    : VNoteBaseDialog(parent)
    , m_notesCount(notesCount)
    , m_msgType(static_cast<MessageType>(msgType))
{
    initUI();
    initConnections();
    initMessage();
}

/**
 * @brief VNoteMessageDialog::setSingleButton
 */
void VNoteMessageDialog::setSingleButton()
{
    m_buttonSpliter->setVisible(false);
    m_confirmBtn->setVisible(false);
    m_cancelBtn->setText(DApplication::translate("VNoteMessageDialog", "OK", "button"));
}

/**
 * @brief VNoteMessageDialog::initUI
 */
void VNoteMessageDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(10, 0, 10, 10);

    QWidget *mainFrame = new QWidget(this);
    mainFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_pMessage = new DLabel(this);
    m_pMessage->setWordWrap(true);
    m_pMessage->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_pMessage, DFontSizeManager::T6);

    //Set Message color to TextTips
    DPalette paMsg = DApplicationHelper::instance()->palette(m_pMessage);
    paMsg.setBrush(DPalette::WindowText, paMsg.color(DPalette::BrightText));
    m_pMessage->setPalette(paMsg);

    QHBoxLayout *actionBarLayout = new QHBoxLayout();
    actionBarLayout->setSpacing(10);
    actionBarLayout->setContentsMargins(0, 0, 0, 0);

    m_cancelBtn = new DPushButton(this);
    m_confirmBtn = new DWarningButton(this);

    m_buttonSpliter = new DVerticalLine(this);
    DPalette pa = DApplicationHelper::instance()->palette(m_buttonSpliter);
    QColor splitColor = pa.color(DPalette::ItemBackground);
    pa.setBrush(DPalette::Background, splitColor);
    m_buttonSpliter->setPalette(pa);
    m_buttonSpliter->setBackgroundRole(QPalette::Background);
    m_buttonSpliter->setAutoFillBackground(true);
    m_buttonSpliter->setFixedSize(4, 28);

    actionBarLayout->addWidget(m_cancelBtn);
    //actionBarLayout->addSpacing(8);
    actionBarLayout->addWidget(m_buttonSpliter);
    // actionBarLayout->addSpacing(8);
    actionBarLayout->addWidget(m_confirmBtn);

    mainLayout->addWidget(m_pMessage, 1);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(actionBarLayout);
    mainFrame->setLayout(mainLayout);

    addContent(mainFrame);
}

/**
 * @brief VNoteMessageDialog::initConnections
 */
void VNoteMessageDialog::initConnections()
{
    connect(m_cancelBtn, &DPushButton::clicked, this, [=]() {
        this->reject();
        this->setEnabled(false);
    });

    connect(m_confirmBtn, &DPushButton::clicked, this, [=]() {
        this->accept();
        this->setEnabled(false);
    });
}

/**
 * @brief VNoteMessageDialog::initMessage
 */
void VNoteMessageDialog::initMessage()
{
    //TODO:
    //   The default button text is Cancel & Confirm
    //In some case OK button text is need to change
    m_cancelBtn->setText(DApplication::translate("VNoteMessageDialog", "Cancel", "button"));
    m_confirmBtn->setText(DApplication::translate("VNoteMessageDialog", "Confirm", "button"));
    switch (m_msgType) {
    case DeleteFolder: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "Are you sure you want to delete this notebook?\nAll notes in it will be deleted"));
    } break;
    case AbortRecord: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "Do you want to stop the current recording?"));
    } break;
    case DeleteNote: {
        if (m_notesCount > 1) {
            m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "Are you sure you want to delete the selected %1 notes?").arg(m_notesCount));
        } else {
            m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "Are you sure you want to delete this note?"));
        }
    } break;
    case AsrTimeLimit: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "Cannot convert this voice note, as notes over 20 minutes are not supported at present."));
        setSingleButton();
    } break;
    case AborteAsr: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "Converting a voice note now. Do you want to stop it?"));
    } break;
    case VolumeTooLow: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "The low input volume may result in bad recordings. Do you want to continue?"));
    } break;
    case CutNote: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog", "The clipped recordings and converted text will not be pasted. Do you want to continue?"));
    }
    }
}
