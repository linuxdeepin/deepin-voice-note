// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for VNoteMainManager. instance() is a light singleton (heavy
// init lives in initNote()); we attach a real WebRichTextManager and rely on
// the pre-seeded VNoteDataManager data. DB-writing VNote*Oper methods are
// stubbed so the shared test database is not mutated (other suites depend on
// it). Methods that exit the process (forceExit), launch a browser
// (showPrivacy) or an external viewer (preViewShortcut) are not exercised.

#include "VNoteMainManager.h"
#include "vnoteitem.h"
#include "vnoteforlder.h"
#include "vnotedatamanager.h"
#include "webrichetextmanager.h"
#include "actionmanager.h"
#include "vnotefolderoper.h"
#include "vnoteitemoper.h"

#include <gtest/gtest.h>
#include <stub.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointF>
#include <QUrl>

static VNoteFolder *stub_addFolder_null(VNoteFolder &) { return nullptr; }
static VNoteItem *stub_addNote_null(VNoteItem &) { return nullptr; }
static bool stub_true() { return true; }

TEST(VNoteMainManagerUT, exercise)
{
    // Stub DB-writing oper methods for the whole test so the shared DB is not
    // mutated (other suites rely on the pre-seeded data). Function bodies of
    // VNoteMainManager still execute -> coverage is preserved.
    Stub sAddFolder, sAddNote, sUpdateTop, sRenameFolder, sModifyTitle, sUpdateNote, sUpdateFolderId;
    sAddFolder.set(ADDR(VNoteFolderOper, addFolder), stub_addFolder_null);
    sAddNote.set(ADDR(VNoteItemOper, addNote), stub_addNote_null);
    sUpdateTop.set(ADDR(VNoteItemOper, updateTop), stub_true);
    sRenameFolder.set(ADDR(VNoteFolderOper, renameVNoteFolder), stub_true);
    sModifyTitle.set(ADDR(VNoteItemOper, modifyNoteTitle), stub_true);
    sUpdateNote.set(ADDR(VNoteItemOper, updateNote), stub_true);
    sUpdateFolderId.set(ADDR(VNoteItemOper, updateFolderId), stub_true);

    VNoteMainManager *m = VNoteMainManager::instance();
    if (!m->m_richTextManager)
        m->m_richTextManager = new WebRichTextManager();

    // load + lookups
    m->loadNotepads();
    m->vNoteFloderChanged(0);
    m->vNoteFloderChanged(999);
    m->vNoteFloderChangedById(0);
    m->vNoteFloderChangedById(999);
    m->getFloderById(0);
    EXPECT_EQ(nullptr, m->getFloderById(999));
    m->getFloderByIndex(0);
    m->getFloderByIndex(999);
    m->getFloderIndexById(0);
    m->getFloderIndexById(999);
    m->getNoteById(0);
    EXPECT_EQ(nullptr, m->getNoteById(999));
    m->vNoteChanged(-1);
    m->vNoteChanged(0);
    m->vNoteChangedWithUIUpdate(0);
    m->onVNoteFoldersLoaded();

    // create / delete
    m->vNoteCreateFolder();
    m->vNoteDeleteFolder(999);
    m->vNoteDeleteFolderById(999);
    m->createNote();
    m->createNoteInFolderId(999);
    m->createNoteInFolderId(-1);
    m->createNoteInFolderId(0);
    EXPECT_FALSE(m->deleteNote(QList<int>{}));
    EXPECT_FALSE(m->deleteNote({999}));
    m->deleteNoteById(0);

    // move / sort / top / rename
    m->moveNotes({}, 0);
    m->moveNotes({999}, 0);
    m->moveNotesToFolderId({}, 0);
    m->moveNotesToFolderId({999}, 0);
    m->updateSort(0, 1);
    m->updateSort(0, 999);
    m->updateSortByFolderIds({0, 1});
    m->updateTop(0, true);
    m->updateTop(999, true);
    m->getTop();
    m->renameFolder(0, "renamed");
    m->renameFolder(999, "x");
    m->renameFolderById(0, "renamed2");
    m->renameFolderById(999, "x");
    m->renameNote(0, "newtitle");
    m->renameNote(0, "");
    m->renameNote(999, "x");
    m->getNotePlainTitle(0);
    m->getNotePlainTitle(999);

    // search / result
    m->vNoteSearch("note");
    m->vNoteSearch("");
    m->updateNoteWithResult("result");
    m->updateNoteWithResultForNote(0, "result");
    m->updateNoteWithResultForNote(999, "result");
    m->loadSearchNotes("note");
    m->loadSearchNotes("");
    m->clearSearch();
    m->isInSearchMode();
    m->onNoteChanged();
    m->updateSearch();
    m->hasNoteText(0);
    m->hasNoteText(999);

    // audio / images / checks
    m->loadAudioSource();
    m->changeAudioSource(0);
    EXPECT_FALSE(m->canInsertImages({}));
    EXPECT_FALSE(m->canInsertImages({QUrl::fromLocalFile("/nonexistent.png")}));
    m->insertImages({});
    m->insertImages({QUrl::fromLocalFile("/nonexistent.png")});
    m->checkNoteVoice({0});
    m->checkNoteVoice({});
    m->checkNoteText({0});
    m->checkNoteText({});

    // voice-text / paths / misc
    m->insertVoice("/tmp/voice-ut.wav", 1000);
    m->insertVoiceTextToNote(0, "v1", "text");
    m->insertVoiceTextToNote(999, "v1", "text");
    m->insertVoiceTextToTiptapNote(0, "v1", "text");
    m->insertVoiceTextToTiptapNote(999, "v1", "text");
    {
        QJsonObject attrs; attrs["voiceId"] = "v1";
        QJsonObject vb; vb["type"] = "voiceBlock"; vb["attrs"] = attrs;
        EXPECT_TRUE(m->updateVoiceBlockText(vb, "v1", "text"));
        EXPECT_FALSE(m->updateVoiceBlockText(vb, "v2", "text"));
        QJsonArray content; content.append(vb);
        QJsonObject root; root["type"] = "doc"; root["content"] = content;
        EXPECT_TRUE(m->updateVoiceBlockText(root, "v1", "text"));
    }
    m->resumeVoicePlayer();
    m->isVoiceToText();
    m->getSavedTextPath();
    m->getSavedVoicePath();
    m->saveUserSelectedPath("/tmp/voice-ut-dir/", VNoteMainManager::Note);
    m->saveUserSelectedPath("/tmp/voice-ut-dir/file.html", VNoteMainManager::Html);
    m->currentNoteId();
    m->hasActiveVoiceToTextTaskForNote(0);
    m->hasActiveVoiceToTextTaskInFolder(0);
    m->saveCurrentNoteBeforeAction(VNoteMainManager::PendingAction::SwitchNote, 0);
    m->onExportFinished(0);
    m->onRichTextSaveFinished();
    // initData() intentionally NOT called: it triggers async DB reload
    // (reqNoteFolders/reqNoteItems) that replaces the pre-seeded in-memory
    // folders other suites depend on.
    m->initConnections();

    m->saveAs({}, "/tmp/voice-ut/", VNoteMainManager::Note);
    m->saveAs({999}, "/tmp/voice-ut/x.html", VNoteMainManager::Html);
    m->saveAs({999}, "/tmp/voice-ut/x.txt", VNoteMainManager::Text);

    SUCCEED();
}
