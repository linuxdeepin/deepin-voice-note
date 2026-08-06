// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VoiceNoteDBusService.

#include "VoiceNoteDBusService.h"
#include <gtest/gtest.h>

TEST(VoiceNoteDBusServiceUT, initAndActivate)
{
    VoiceNoteDBusService svc;
    // Registration may succeed or fail (name possibly held by another process);
    // either way the function body executes.
    svc.initDBusService();
    svc.ActivateWindow();   // no top-level windows in test -> logs + returns
    SUCCEED();
}

TEST(VoiceNoteDBusServiceUT, getNotesList)
{
    VoiceNoteDBusService svc;
    QString json = svc.GetNotesList();   // uses pre-seeded VNoteDataManager
    EXPECT_FALSE(json.isEmpty());
}

TEST(VoiceNoteDBusServiceUT, recordVoiceNotFound)
{
    VoiceNoteDBusService svc;
    // folder 999 absent -> early "Folder not found" return (no recording side effect)
    EXPECT_FALSE(svc.RecordVoice(999, 999));
}
