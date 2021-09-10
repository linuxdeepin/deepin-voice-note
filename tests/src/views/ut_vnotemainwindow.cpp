/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_vnotemainwindow.h"
#include "vnotemainwindow.h"
#include "leftview.h"
#include "middleview.h"
#include "homepage.h"
#include "splashview.h"
#include "middleviewsortfilter.h"
#include "standarditemcommon.h"
#include "vnotedatamanager.h"
#include "vnotea2tmanager.h"
#include "vnoteitem.h"
#include "vnoteforlder.h"
#include "actionmanager.h"
#include "utils.h"
#include "actionmanager.h"
#include "setting.h"
#include "vnotefolderoper.h"
#include "vnoteitemoper.h"
#include "vnotedbmanager.h"
#include "dbuslogin1manager.h"
#include "vnotemessagedialog.h"
#include "vnoterecordbar.h"
#include "vnoteiconbutton.h"
#include "vnmainwnddelayinittask.h"
#include "richtextedit.h"

#include "upgradeview.h"
#include "upgradedbutil.h"

#include "vnoteapplication.h"
#include "stub.h"

#include <DFileDialog>

static void stub_vnotemainwindow()
{
}

static int stub_int()
{
    return 1;
}

static int stub_fileDialog()
{
    return 0;
}

ut_vnotemainwindow_test::ut_vnotemainwindow_test()
{
}

void ut_vnotemainwindow_test::SetUp()
{
    Stub stub;
    stub.set(ADDR(VNoteMainWindow, initData), stub_vnotemainwindow);
    stub.set(ADDR(VNoteMainWindow, delayInitTasks), stub_vnotemainwindow);
    stub.set(ADDR(RichTextEdit, initWebView), stub_vnotemainwindow);
    m_mainWindow = new VNoteMainWindow;
    m_mainWindow->switchWidget(VNoteMainWindow::WndNoteShow);
}

void ut_vnotemainwindow_test::TearDown()
{
    delete m_mainWindow;
}

TEST_F(ut_vnotemainwindow_test, holdHaltLock)
{
    m_mainWindow->holdHaltLock();
    m_mainWindow->releaseHaltLock();
    EXPECT_FALSE(m_mainWindow->m_lockFd.isValid());
    m_mainWindow->onVNoteFoldersLoaded();
    EXPECT_FALSE(m_mainWindow->m_stackedWidget->currentIndex() != VNoteMainWindow::WndNoteShow);
}

//TEST_F(ut_vnotemainwindow_test, onVNoteSearch)
//{
//    VNoteMainWindow vnotemainwindow;
//     m_mainWindow->onVNoteSearch();
//}

TEST_F(ut_vnotemainwindow_test, onVNoteSearchTextChange)
{
    m_mainWindow->onVNoteSearchTextChange("");
    EXPECT_EQ(OpsStateInterface::instance()->isSearching(), false);
}

TEST_F(ut_vnotemainwindow_test, loadNotepads)
{
    EXPECT_TRUE(m_mainWindow->loadNotepads());
}

TEST_F(ut_vnotemainwindow_test, loadNotes)
{
    VNoteFolder *vnotefolder = new VNoteFolder;
    vnotefolder->id = 0;
    vnotefolder->category = 1;
    vnotefolder->notesCount = 2;
    vnotefolder->defaultIcon = 3;
    vnotefolder->folder_state = vnotefolder->Normal;
    vnotefolder->name = "test";
    vnotefolder->iconPath = "/home/zhangteng/works/deepin-voice-note/assets/icons/deepin/builtin/default_folder_icons";
    vnotefolder->sortNumber = 4;
    vnotefolder->createTime = QDateTime::currentDateTime();
    vnotefolder->modifyTime = QDateTime::currentDateTime();
    vnotefolder->deleteTime = QDateTime::currentDateTime();
    m_mainWindow->loadNotes(vnotefolder);
    EXPECT_EQ(m_mainWindow->m_middleView->getCurrentId(), vnotefolder->id);
    delete vnotefolder;
}

TEST_F(ut_vnotemainwindow_test, loadSearchNotes)
{
    EXPECT_FALSE(m_mainWindow->loadSearchNotes("本"));
}

TEST_F(ut_vnotemainwindow_test, initDeviceExceptionErrMessage)
{
    m_mainWindow->initDeviceExceptionErrMessage();
    EXPECT_TRUE(m_mainWindow->m_pDeviceExceptionMsg);
}

TEST_F(ut_vnotemainwindow_test, showAsrErrMessage)
{
    m_mainWindow->showAsrErrMessage("error Message");
    EXPECT_TRUE(m_mainWindow->m_asrErrMeassage);
}

TEST_F(ut_vnotemainwindow_test, showDeviceExceptionErrMessage)
{
    m_mainWindow->showDeviceExceptionErrMessage();
    EXPECT_TRUE(m_mainWindow->m_pDeviceExceptionMsg);
}

TEST_F(ut_vnotemainwindow_test, initMenuExtension)
{
    m_mainWindow->initMenuExtension();
    EXPECT_TRUE(m_mainWindow->m_menuExtension);
}

TEST_F(ut_vnotemainwindow_test, setSpecialStatus)
{
    m_mainWindow->setSpecialStatus(m_mainWindow->SearchStart);
    EXPECT_EQ(OpsStateInterface::instance()->isSearching(), true);
    m_mainWindow->setSpecialStatus(m_mainWindow->SearchEnd);
    EXPECT_EQ(OpsStateInterface::instance()->isSearching(), false);
    m_mainWindow->setSpecialStatus(m_mainWindow->PlayVoiceStart);
    EXPECT_EQ(OpsStateInterface::instance()->isPlaying(), true);
    m_mainWindow->setSpecialStatus(m_mainWindow->PlayVoiceEnd);
    EXPECT_EQ(OpsStateInterface::instance()->isPlaying(), false);
    m_mainWindow->setSpecialStatus(m_mainWindow->RecordStart);
    EXPECT_EQ(OpsStateInterface::instance()->isRecording(), true);
    m_mainWindow->setSpecialStatus(m_mainWindow->RecordEnd);
    EXPECT_EQ(OpsStateInterface::instance()->isRecording(), false);
    m_mainWindow->setSpecialStatus(m_mainWindow->VoiceToTextStart);
    EXPECT_EQ(OpsStateInterface::instance()->isVoice2Text(), true);
    m_mainWindow->setSpecialStatus(m_mainWindow->VoiceToTextEnd);
    EXPECT_EQ(OpsStateInterface::instance()->isVoice2Text(), false);
}

TEST_F(ut_vnotemainwindow_test, initAsrErrMessage)
{
    m_mainWindow->initAsrErrMessage();
    EXPECT_TRUE(m_mainWindow->m_asrErrMeassage);
}

TEST_F(ut_vnotemainwindow_test, onStartRecord)
{
    m_mainWindow->onStartRecord("/usr/share/music/bensound-sunny.mp3");
    EXPECT_EQ(OpsStateInterface::instance()->isRecording(), true);
    m_mainWindow->onFinshRecord("/usr/share/music/bensound-sunny.mp3", 2650);
    EXPECT_EQ(OpsStateInterface::instance()->isRecording(), false);
}

TEST_F(ut_vnotemainwindow_test, onA2TStartAgain)
{
    m_mainWindow->onA2TError(1);
    m_mainWindow->onA2TSuccess("/usr/share/music/bensound-sunny.mp3");
    EXPECT_TRUE(m_mainWindow->m_asrErrMeassage);
}

TEST_F(ut_vnotemainwindow_test, onPreviewShortcut)
{
    m_mainWindow->onPreviewShortcut();
}

TEST_F(ut_vnotemainwindow_test, event)
{
    QSize size;
    QResizeEvent *event1 = new QResizeEvent(size, size);
    m_mainWindow->resizeEvent(event1);
    delete event1;
}

TEST_F(ut_vnotemainwindow_test, handleMultipleOption)
{
    typedef int (*fptr)();
    fptr A_foo = (fptr)(&DFileDialog::exec);
    Stub stub;
    stub.set(A_foo, stub_fileDialog);
    int id = 1;
    m_mainWindow->handleMultipleOption(id);
    int id2 = 2;
    m_mainWindow->handleMultipleOption(id2);
    int id3 = 3;
    m_mainWindow->handleMultipleOption(id3);
}

TEST_F(ut_vnotemainwindow_test, onDropNote)
{
    bool dropCancel = true;
    m_mainWindow->onDropNote(dropCancel);
    EXPECT_EQ(m_mainWindow->m_middleView->m_dragSuccess, false);
    bool dropCancels = false;
    m_mainWindow->onDropNote(dropCancels);
    EXPECT_EQ(m_mainWindow->m_middleView->m_dragSuccess, true);
}

TEST_F(ut_vnotemainwindow_test, onShortcut)
{
    m_mainWindow->onEscShortcut();
    //m_mainWindow->onDeleteShortcut();

    QScopedPointer<Stub> stub;
    stub.reset(new Stub);
    stub->set(ADDR(LeftView, hasFocus), stub_int);
    //m_mainWindow->onDeleteShortcut();
    m_mainWindow->onPoppuMenuShortcut();
    stub.reset(new Stub);
    stub->set(ADDR(MiddleView, hasFocus), stub_int);
    //m_mainWindow->onDeleteShortcut();
    m_mainWindow->onPoppuMenuShortcut();
    stub.reset(new Stub);
    stub->set(ADDR(QLineEdit, hasFocus), stub_int);
    //m_mainWindow->onDeleteShortcut();
    m_mainWindow->onPoppuMenuShortcut();

    m_mainWindow->onAddNotepadShortcut();
    m_mainWindow->onReNameNotepadShortcut();
    m_mainWindow->onAddNoteShortcut();
    m_mainWindow->onRenameNoteShortcut();
    m_mainWindow->onPlayPauseShortcut();
    m_mainWindow->onSaveMp3Shortcut();
    m_mainWindow->onSaveTextShortcut();
    m_mainWindow->onSaveVoicesShortcut();
}

TEST_F(ut_vnotemainwindow_test, setviewNext)
{
    QKeyEvent e(QEvent::KeyPress, 0x58, Qt::ControlModifier, "test");
    m_mainWindow->setTitleBarTabFocus(&e);
    m_mainWindow->setMiddleviewNext(&e);
    m_mainWindow->setTitleCloseButtonNext(&e);
    m_mainWindow->setAddnoteButtonNext(&e);
    m_mainWindow->setAddnotepadButtonNext(&e);
}

TEST_F(ut_vnotemainwindow_test, onNewNote)
{
    m_mainWindow->onNewNote();
    m_mainWindow->delNote();
    EXPECT_EQ(m_mainWindow->m_middleView->rowCount(), 0);
}

TEST_F(ut_vnotemainwindow_test, onNewNotebook)
{
    m_mainWindow->onNewNotebook();
    m_mainWindow->delNotepad();
    EXPECT_EQ(m_mainWindow->m_leftView->folderCount(), 0);
}

TEST_F(ut_vnotemainwindow_test, onPlayPlugVoice)
{
    m_mainWindow->onPlayPlugVoicePlay(nullptr);
    m_mainWindow->onPlayPlugVoicePause(nullptr);
    m_mainWindow->onPlayPlugVoiceStop(nullptr);
}
