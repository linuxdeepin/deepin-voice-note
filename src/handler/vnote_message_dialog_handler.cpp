// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnote_message_dialog_handler.h"

#include <QApplication>

/**
   @class VNoteMessageDialogHandler
   @brief 配合 QML 组件 VNoteMessageDialogLoader ，提供不同类型消息对话框的提示文本
 */
VNoteMessageDialogHandler::VNoteMessageDialogHandler(QObject *parent)
    : QObject(parent)
{
}

VNoteMessageDialogHandler::MessageType VNoteMessageDialogHandler::messageType() const
{
    return m_messageType;
}

void VNoteMessageDialogHandler::setMessageType(MessageType type)
{
    if (type != m_messageType) {
        m_messageType = type;

        initMessage();

        Q_EMIT messageTypeChanged();
    }
}

QVariant VNoteMessageDialogHandler::messageData() const
{
    return m_messageData;
}

void VNoteMessageDialogHandler::setMessageData(const QVariant &data)
{
    if (data != m_messageData) {
        m_messageData = data;

        Q_EMIT messageDataChanged();

        switch (m_messageType) {
            case DeleteNote:
                initMessage();
            default:
                break;
        }
    }
}

QString VNoteMessageDialogHandler::mainMessage() const
{
    return m_mainMessage;
}

QString VNoteMessageDialogHandler::detailMessage() const
{
    return m_detailMessage;
}

QString VNoteMessageDialogHandler::warnConfirm() const
{
    return m_warnConfirm;
}

bool VNoteMessageDialogHandler::singleButton() const
{
    return m_singleButton;
}

void VNoteMessageDialogHandler::initMessage()
{
    m_mainMessage = initMainMessage();
    Q_EMIT mainMessageChanged();

    m_detailMessage = initDetailMessage();
    Q_EMIT detailMessageChanged();

    m_warnConfirm = initWarnConfirm();
    Q_EMIT warnConfirmChanged();

    const bool singleBtn = initSingleButton();
    if (singleBtn != m_singleButton) {
        m_singleButton = singleBtn;
        Q_EMIT singleButtonChanged();
    }
}

QString VNoteMessageDialogHandler::initMainMessage() const
{
    switch (m_messageType) {
        case DeleteFolder:
            return QApplication::translate("VNoteMessageDialogHandler", "Are you sure you want to delete this notebook?");
        case AbortRecord:
            return QApplication::translate("VNoteMessageDialogHandler", "Do you want to stop the current recording?");
        case DeleteNote: {
            const int deleteNotes = m_messageData.toInt();
            if (deleteNotes > 1) {
                return QApplication::translate("VNoteMessageDialogHandler",
                                               "Are you sure you want to delete the selected %1 notes?")
                    .arg(deleteNotes);
            }
            return QApplication::translate("VNoteMessageDialogHandler", "Are you sure you want to delete this note?");
        }
        case AsrTimeLimit:
            return QApplication::translate(
                "VNoteMessageDialogHandler",
                "Cannot convert this voice note, as notes over 20 minutes are not supported at present.");
        case AborteAsr:
            return QApplication::translate("VNoteMessageDialogHandler", "Converting a voice note now. Do you want to stop it?");
        case VolumeTooLow:
            return QApplication::translate("VNoteMessageDialogHandler",
                                           "The low input volume may result in bad recordings. Do you want to continue?");
        case CutNote:
            return QApplication::translate(
                "VNoteMessageDialogHandler",
                "The clipped recordings and converted text will not be pasted. Do you want to continue?");
        case SaveFailed:
            return QApplication::translate("VNoteMessageDialogHandler", "Save failed");
        case NoPermission:
            return QApplication::translate("VNoteMessageDialogHandler", "You do not have permission to save files there");
        case VoicePathNoAvail:
            return QApplication::translate("VNoteMessageDialogHandler", "The voice note has been deleted");
        case NoNetwork:
            return QApplication::translate("VNoteMessageDialogHandler", "The voice conversion failed due to the poor network connection, please have a check");
        default:
            break;
    }

    return {};
}

QString VNoteMessageDialogHandler::initDetailMessage() const
{
    switch (m_messageType) {
        case DeleteFolder:
            return QApplication::translate("VNoteMessageDialogHandler", "All notes in it will be deleted");
        default:
            break;
    }

    return {};
}

QString VNoteMessageDialogHandler::initWarnConfirm() const
{
    switch (m_messageType) {
        case DeleteNote:
        case DeleteFolder:
            return QApplication::translate("VNoteMessageDialogHandler", "Delete");
        default:
            break;
    }

    return {};
}

bool VNoteMessageDialogHandler::initSingleButton() const
{
    switch (m_messageType) {
        case AsrTimeLimit:
        case NoPermission:
        case VoicePathNoAvail:
            return true;
        default:
            break;
    }

    return false;
}
