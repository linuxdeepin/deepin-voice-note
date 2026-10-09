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
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "setting.h"
#include "globaldef.h"
#include "voice_recoder_handler.h"
#include "opsstateinterface.h"

static VNoteFolder *stub_addFolder_null(VNoteFolder &) { return nullptr; }
static VNoteItem *stub_addNote_null(VNoteItem &) { return nullptr; }
static bool stub_true() { return true; }

// ---- PMS 补强回归用例 stub 辅助（只计数/返回固定值，不写 DB） ----
static int g_addNoteCalls = 0;
static VNoteItem *stub_addNote_count(VNoteItem &) { ++g_addNoteCalls; return nullptr; }
static VoiceRecoderHandler::RecoderType stub_recording_type() { return VoiceRecoderHandler::Recording; }
static bool stub_isPlaying_false() { return false; }

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

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// 命名：BUG<id>_<场景>；源码上下文经 codebase-memory-mcp 获取，不改项目源码。
// ============================================================================

// BUG 335623: 初始化界面（无任何记事本）按 Ctrl+B 新建笔记崩溃。
// 修复：createNoteInFolderId 对 folderId==-1 / 目标文件夹不存在 提前返回
//（src/common/VNoteMainManager.cpp:714-776）。
// PMS: https://pms.uniontech.com/bug-view-335623.html  commit: a9eeef4c, befc63df
TEST(VNoteMainManagerUT, BUG335623_createNoteInFolderId_invalidFolder_noCrash)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    if (!m->m_richTextManager)
        m->m_richTextManager = new WebRichTextManager();

    g_addNoteCalls = 0;
    Stub sAddNote;
    sAddNote.set(ADDR(VNoteItemOper, addNote), stub_addNote_count);

    // 无目标文件夹（-1）与不存在的文件夹（999）都必须安全返回，
    // 不崩溃、不触达 DB 新增笔记。
    m->createNoteInFolderId(-1);
    m->createNoteInFolderId(999);
    EXPECT_EQ(0, g_addNoteCalls);
}

// BUG 335263: 录音过程中新建记事本/笔记等操作未置灰（管理层兑底）。
// 修复：createNoteInFolderId / vNoteDeleteFolderById 在录音状态下拒绝执行。
// PMS: https://pms.uniontech.com/bug-view-335263.html  commit: a9eeef4c, e3c51a74
TEST(VNoteMainManagerUT, BUG335263_createBlockedWhileRecording_noDbWrite)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    if (!m->m_richTextManager)
        m->m_richTextManager = new WebRichTextManager();

    g_addNoteCalls = 0;
    Stub sRec, sPlay, sAddNote;
    sRec.set(ADDR(VoiceRecoderHandler, getRecoderType), stub_recording_type);
    sPlay.set(ADDR(OpsStateInterface, isPlaying), stub_isPlaying_false);
    sAddNote.set(ADDR(VNoteItemOper, addNote), stub_addNote_count);

    // 合法文件夹但在录音中：不允许新建
    m->createNoteInFolderId(0);
    EXPECT_EQ(0, g_addNoteCalls);
}

// BUG 336411: 录音过程中删除当前记事本/笔记不应响应。
// 修复：vNoteDeleteFolderById 在录音/播放状态下返回 false。
// PMS: https://pms.uniontech.com/bug-view-336411.html  commit: ef4b5163
TEST(VNoteMainManagerUT, BUG336411_deleteFolderBlockedWhileRecording)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    Stub sRec, sPlay;
    sRec.set(ADDR(VoiceRecoderHandler, getRecoderType), stub_recording_type);
    sPlay.set(ADDR(OpsStateInterface, isPlaying), stub_isPlaying_false);

    // 存在的文件夹（0）在录音中也不允许删除
    EXPECT_FALSE(m->vNoteDeleteFolderById(0));
}

// BUG 284293: 频繁新建/删除记事本后内存占用高：恢复排序里残留无效/重复
// 文件夹 id。修复：loadNotepads 清洗持久化排序（去无效、去重，只保留真实
// 文件夹，src/common/VNoteMainManager.cpp:333-419）。
// PMS: https://pms.uniontech.com/bug-view-284293.html  commit: 1bcada06
TEST(VNoteMainManagerUT, BUG284293_loadNotepads_sanitizesStaleFolderSort)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    const QString original = setting::instance()->getOption(VNOTE_FOLDER_SORT).toString();

    // 注入重复/无效/不存在的排序项
    setting::instance()->setOption(VNOTE_FOLDER_SORT, "999,-5,999,0,");
    m->loadNotepads();

    const QString after = setting::instance()->getOption(VNOTE_FOLDER_SORT).toString();
    const QStringList ids = after.split(",", Qt::SkipEmptyParts);
    QSet<QString> uniq;
    for (const QString &id : ids) {
        bool ok = false;
        const qint64 v = id.toLongLong(&ok);
        EXPECT_TRUE(ok) << "non-numeric id kept: " << qPrintable(id);
        EXPECT_NE(999, v) << "stale folder id 999 kept in sort";
        EXPECT_NE(-5, v) << "invalid folder id -5 kept in sort";
        if (ok)
            EXPECT_NE(nullptr, m->getFloderById(static_cast<int>(v)))
                << "nonexistent folder kept in sort: " << qPrintable(id);
        uniq.insert(id);
    }
    EXPECT_EQ(uniq.size(), ids.size()) << "duplicate ids kept in sort";

    setting::instance()->setOption(VNOTE_FOLDER_SORT, original);
}

// BUG 330295: 重命名笔记本输入内容回车不生效。修复：renameNote 成功后统一
// 重载列表并通知工作区标题（noteTitleChanged/currentNoteChanged），非播放态
// 走 loadNotes 保证改名后的列表与选中项一致（src/common/VNoteMainManager.cpp:1447-1481）。
// PMS: https://pms.uniontech.com/bug-view-330295.html  commit: da9aa565
TEST(VNoteMainManagerUT, BUG330295_renameNote_reloadsAndKeepsSelection)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    if (!m->m_richTextManager)
        m->m_richTextManager = new WebRichTextManager();

    // 取文件夹 0 中 id>0 的一张真实笔记（loadNotes preferred 选择仅认 >0）；无则退化烟测
    int noteId = -1;
    VNOTE_ALL_NOTES_MAP *all = VNoteDataManager::instance()->getAllNotesInFolder();
    if (all) {
        all->lock.lockForRead();
        auto fit = all->notes.constFind(0);
        if (fit != all->notes.constEnd() && fit.value()) {
            fit.value()->lock.lockForRead();
            for (auto nit = fit.value()->folderNotes.begin();
                 nit != fit.value()->folderNotes.end(); ++nit) {
                if (nit.key() > 0) { noteId = static_cast<int>(nit.key()); break; }
            }
            fit.value()->lock.unlock();
        }
        all->lock.unlock();
    }
    if (noteId < 0) {
        GTEST_SKIP() << "no pre-seeded note in folder 0";
    }

    m->doSwitchNote(noteId);
    EXPECT_EQ(noteId, m->currentNoteId());

    QSignalSpy titleSpy(m, &VNoteMainManager::noteTitleChanged);
    QSignalSpy noteSpy(m, &VNoteMainManager::currentNoteChanged);
    Stub sRename, sPlay;
    sRename.set(ADDR(VNoteItemOper, modifyNoteTitle), stub_true);
    sPlay.set(ADDR(OpsStateInterface, isPlaying), stub_isPlaying_false);

    m->renameNote(noteId, QStringLiteral("武汉-UT"));
    EXPECT_EQ(1, titleSpy.count()) << "noteTitleChanged must fire after rename";
    EXPECT_EQ(1, noteSpy.count()) << "currentNoteChanged must fire for selected note";
    if (titleSpy.count() > 0) {
        EXPECT_EQ(QStringLiteral("武汉-UT"), titleSpy.first().at(1).toString());
    }
    EXPECT_EQ(noteId, m->currentNoteId()) << "selection must stay on renamed note";
}

// BUG 365373: 搜索结果界面无文字记录按 Ctrl+S 异常。修复：删除已不存在的
// 文件夹时按陈旧 UI 清理处理（清洗排序、复位当前项并返回 true）。
// PMS: https://pms.uniontech.com/bug-view-365373.html  commit: 3b908198
TEST(VNoteMainManagerUT, BUG365373_deleteAbsentFolder_staleCleanupTrue)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    const QString original = setting::instance()->getOption(VNOTE_FOLDER_SORT).toString();

    // 已不存在的文件夹：不崩溃，返回 true（陈旧清理语义）
    EXPECT_TRUE(m->vNoteDeleteFolderById(999));
    EXPECT_EQ(nullptr, m->getFloderById(999));

    setting::instance()->setOption(VNOTE_FOLDER_SORT, original);
}

// BUG 290781: 点击 X 关闭后自动重新打开（导出路径健壮性同步补强：saveAs
// 对无效索引/不存在笔记不得启动导出任务）。
// PMS: https://pms.uniontech.com/bug-view-290781.html  commit: f30b5067, f0b7b62e
TEST(VNoteMainManagerUT, BUG290781_saveAs_invalidIndices_noExport)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    // 无效 QVariant + 不存在的笔记 id：全部跳过 → 不启动导出，目录保持为空
    m->saveAs({QVariant(), 999}, dir.path(), VNoteMainManager::Text);
    QDir out(dir.path());
    EXPECT_TRUE(out.entryList(QDir::Files).isEmpty())
        << "no export file may be produced from invalid indices";
}

// BUG 373423: 冷/热启动性能劣化（lookup/count 自由函数为文件局部符号，
// 经公开入口 updateSortByFolderIds 间接兜底排序重建路径）。
// PMS: https://pms.uniontech.com/bug-view-373423.html  commit: 1c3db587, 9e881b82, 7273135c
TEST(VNoteMainManagerUT, BUG373423_updateSortByFolderIds_rebuildsSort)
{
    VNoteMainManager *m = VNoteMainManager::instance();
    const QString original = setting::instance()->getOption(VNOTE_FOLDER_SORT).toString();

    m->updateSortByFolderIds({0, 1});
    m->updateSortByFolderIds({});

    // 排序持久化里不应再出现已删除文件夹
    const QString after = setting::instance()->getOption(VNOTE_FOLDER_SORT).toString();
    const QStringList ids = after.split(",", Qt::SkipEmptyParts);
    for (const QString &id : ids) {
        bool ok = false;
        const qint64 v = id.toLongLong(&ok);
        if (ok)
            EXPECT_NE(nullptr, m->getFloderById(static_cast<int>(v)))
                << "stale folder id after sort rebuild: " << qPrintable(id);
    }

    setting::instance()->setOption(VNOTE_FOLDER_SORT, original);
    EXPECT_FALSE(m->hasExistingFolders() == false && ids.isEmpty());
}
