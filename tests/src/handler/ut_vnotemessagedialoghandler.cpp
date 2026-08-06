// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VNoteMessageDialogHandler.

#include "vnote_message_dialog_handler.h"
#include <gtest/gtest.h>
#include <QVariant>

TEST(VNoteMessageDialogHandlerUT, allMessageTypes)
{
    VNoteMessageDialogHandler h;
    EXPECT_EQ(VNoteMessageDialogHandler::None, h.messageType());
    h.setMessageData(2);
    h.setMessageType(VNoteMessageDialogHandler::DeleteNote);   // deleteNotes > 1 branch
    h.setMessageData(1);
    h.setMessageType(VNoteMessageDialogHandler::DeleteNote);   // single note branch
    h.setMessageType(VNoteMessageDialogHandler::DeleteFolder);
    h.setMessageType(VNoteMessageDialogHandler::AbortRecord);
    h.setMessageType(VNoteMessageDialogHandler::AsrTimeLimit);
    h.setMessageType(VNoteMessageDialogHandler::AborteAsr);
    h.setMessageType(VNoteMessageDialogHandler::VolumeTooLow);
    h.setMessageType(VNoteMessageDialogHandler::CutNote);
    h.setMessageType(VNoteMessageDialogHandler::SaveFailed);
    h.setMessageType(VNoteMessageDialogHandler::NoPermission);
    h.setMessageType(VNoteMessageDialogHandler::VoicePathNoAvail);
    h.setMessageType(VNoteMessageDialogHandler::NoNetwork);
    h.setMessageType(VNoteMessageDialogHandler::None);
    // exercise getters (values are translated strings)
    h.mainMessage();
    h.detailMessage();
    h.warnConfirm();
    h.singleButton();
    h.messageData();
    SUCCEED();
}
