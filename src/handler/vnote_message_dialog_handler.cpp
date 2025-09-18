// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnote_message_dialog_handler.h"

#include <QApplication>
#include <QDebug>

/**
   @class VNoteMessageDialogHandler
   @brief 配合 QML 组件 VNoteMessageDialogLoader ，提供不同类型消息对话框的提示文本
 */
VNoteMessageDialogHandler::VNoteMessageDialogHandler(QObject *parent)
    : QObject(parent)
{
    qInfo() << "VNoteMessageDialogHandler constructor called";
}

VNoteMessageDialogHandler::MessageType VNoteMessageDialogHandler::messageType() const
{
    // qInfo() << "Getting message type:" << m_messageType;
    return m_messageType;
}

void VNoteMessageDialogHandler::setMessageType(MessageType type)
{
    // qInfo() << "Setting message type to:" << type;
    if (type != m_messageType) {
        qInfo() << "Setting message type to:" << type;
        m_messageType = type;

        initMessage();

        Q_EMIT messageTypeChanged();
    }
    // qInfo() << "Message type setting finished";
}

QVariant VNoteMessageDialogHandler::messageData() const
{
    // qInfo() << "Getting message data";
    return m_messageData;
}

void VNoteMessageDialogHandler::setMessageData(const QVariant &data)
{
    // qInfo() << "Setting message data";
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
    // qInfo() << "Message data setting finished";
}

QString VNoteMessageDialogHandler::mainMessage() const
{
    // qInfo() << "Getting main message:" << m_mainMessage;
    return m_mainMessage;
}

QString VNoteMessageDialogHandler::detailMessage() const
{
    // qInfo() << "Getting detail message:" << m_detailMessage;
    return m_detailMessage;
}

QString VNoteMessageDialogHandler::warnConfirm() const
{
    // qInfo() << "Getting warn confirm message:" << m_warnConfirm;
    return m_warnConfirm;
}

bool VNoteMessageDialogHandler::singleButton() const
{
    // qInfo() << "Getting single button flag:" << m_singleButton;
    return m_singleButton;
}

void VNoteMessageDialogHandler::initMessage()
{
    qInfo() << "Initializing message";
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
    qInfo() << "Message initialization finished";
}

QString VNoteMessageDialogHandler::initMainMessage() const
{
    qInfo() << "Initializing main message for type:" << m_messageType;
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

    qInfo() << "message type is unknown, return empty string";
    return {};
}

QString VNoteMessageDialogHandler::initDetailMessage() const
{
    qInfo() << "Initializing detail message for type:" << m_messageType;
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
    qInfo() << "Initializing warn confirm message for type:" << m_messageType;
    switch (m_messageType) {
        case DeleteNote:
        case DeleteFolder:
            qInfo() << "type is DeleteNote or DeleteFolder";
            return QApplication::translate("VNoteMessageDialogHandler", "Delete");
        default:
            break;
    }

    qInfo() << "type is not DeleteNote or DeleteFolder";
    return {};
}

bool VNoteMessageDialogHandler::initSingleButton() const
{
    qInfo() << "Initializing single button flag for type:" << m_messageType;
    switch (m_messageType) {
        case AsrTimeLimit:
        case NoPermission:
        case VoicePathNoAvail:
            qInfo() << "type is AsrTimeLimit or NoPermission or VoicePathNoAvail";
            return true;
        default:
            break;
    }

    qInfo() << "type is not AsrTimeLimit or NoPermission or VoicePathNoAvail";
    return false;
}
