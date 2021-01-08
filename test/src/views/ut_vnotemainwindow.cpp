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
#include "rightview.h"
#include "homepage.h"
#include "splashview.h"
#include "voicenoteitem.h"
#include "middleviewsortfilter.h"
#include "standarditemcommon.h"
#include "vnotedatamanager.h"
#include "vnotea2tmanager.h"
#include "vnoteitem.h"
#include "vnoteforlder.h"
#include "actionmanager.h"
#include "vnotedatasafefymanager.h"
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

#ifdef IMPORT_OLD_VERSION_DATA
#include "upgradeview.h"
#include "upgradedbutil.h"
#endif

#include "vnoteapplication.h"

ut_vnotemainwindow_test::ut_vnotemainwindow_test()
{
}

TEST_F(ut_vnotemainwindow_test, holdHaltLock)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.holdHaltLock();
    vnotemainwindow.releaseHaltLock();
    vnotemainwindow.onVNoteFoldersLoaded();
}

TEST_F(ut_vnotemainwindow_test, onVNoteSearch)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.onVNoteSearch();
}

TEST_F(ut_vnotemainwindow_test, onVNoteSearchTextChange)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.onVNoteSearchTextChange("");
}

TEST_F(ut_vnotemainwindow_test, initSpliterView)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initSpliterView();
    vnotemainwindow.initSplashView();
}

TEST_F(ut_vnotemainwindow_test, initTitleBar)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initTitleBar();
}

TEST_F(ut_vnotemainwindow_test, initMiddleView)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initMiddleView();
}

TEST_F(ut_vnotemainwindow_test, initRightView)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initRightView();
}

TEST_F(ut_vnotemainwindow_test, initA2TManager)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initA2TManager();
}

TEST_F(ut_vnotemainwindow_test, initLogin1Manager)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initLogin1Manager();
}

TEST_F(ut_vnotemainwindow_test, loadNotepads)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.loadNotepads();
}

TEST_F(ut_vnotemainwindow_test, loadNotes)
{
    VNoteMainWindow vnotemainwindow;
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
    VNoteDataManager::instance()->m_qspNoteFoldersMap.reset(new VNOTE_FOLDERS_MAP());
    VNoteDataManager::instance()->m_qspAllNotesMap.reset(new VNOTE_ALL_NOTES_MAP());
    vnotemainwindow.loadNotes(vnotefolder);
}

TEST_F(ut_vnotemainwindow_test, loadSearchNotes)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.loadSearchNotes("本");
}

TEST_F(ut_vnotemainwindow_test, initDeviceExceptionErrMessage)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initDeviceExceptionErrMessage();
}

TEST_F(ut_vnotemainwindow_test, showAsrErrMessage)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.showAsrErrMessage("error Message");
}

TEST_F(ut_vnotemainwindow_test, showDeviceExceptionErrMessage)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.showDeviceExceptionErrMessage();
}

TEST_F(ut_vnotemainwindow_test, onCursorChange)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.onCursorChange(150, true);
}

TEST_F(ut_vnotemainwindow_test, initMenuExtension)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initMenuExtension();
}

TEST_F(ut_vnotemainwindow_test, setSpecialStatus)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.setSpecialStatus(vnotemainwindow.SearchStart);
    vnotemainwindow.setSpecialStatus(vnotemainwindow.SearchEnd);
    vnotemainwindow.setSpecialStatus(vnotemainwindow.PlayVoiceStart);
    vnotemainwindow.setSpecialStatus(vnotemainwindow.PlayVoiceEnd);
    vnotemainwindow.setSpecialStatus(vnotemainwindow.RecordStart);
    vnotemainwindow.setSpecialStatus(vnotemainwindow.RecordEnd);
    vnotemainwindow.setSpecialStatus(vnotemainwindow.VoiceToTextStart);
    vnotemainwindow.setSpecialStatus(vnotemainwindow.VoiceToTextEnd);
}

TEST_F(ut_vnotemainwindow_test, initAsrErrMessage)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.initAsrErrMessage();
}

TEST_F(ut_vnotemainwindow_test, onStartRecord)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.onStartRecord("/usr/share/music/bensound-sunny.mp3");
    vnotemainwindow.onFinshRecord("/usr/share/music/bensound-sunny.mp3", 2650);
}

TEST_F(ut_vnotemainwindow_test, onA2TStartAgain)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.onChangeTheme();
    vnotemainwindow.onA2TStartAgain();
    vnotemainwindow.onA2TError(1);
    vnotemainwindow.onA2TSuccess("/usr/share/music/bensound-sunny.mp3");
}

TEST_F(ut_vnotemainwindow_test, onPreviewShortcut)
{
    VNoteMainWindow vnotemainwindow;
    vnotemainwindow.onPreviewShortcut();
}

TEST_F(ut_vnotemainwindow_test, event)
{
    VNoteMainWindow vnotemainwindow;
    QSize size;
    QResizeEvent *event1 = new QResizeEvent(size, size);
    vnotemainwindow.resizeEvent(event1);
    QKeyEvent *event2 = new QKeyEvent(QEvent::KeyPress, 0x58, Qt::ControlModifier, "test");
    vnotemainwindow.keyPressEvent(event2);
}

TEST_F(ut_vnotemainwindow_test, handleMultipleOption)
{
    VNoteMainWindow vnotemainwindow;
    MiddleView middleview;
    middleview.setCurrentIndex(0);
    int id = 1;
    vnotemainwindow.handleMultipleOption(id);
    int id2 = 2;
    vnotemainwindow.handleMultipleOption(id2);
    int id3 = 3;
    vnotemainwindow.handleMultipleOption(id3);
}

TEST_F(ut_vnotemainwindow_test, onDropNote)
{
    VNoteMainWindow vnotemainwindow;
    bool dropCancel = true;
    vnotemainwindow.onDropNote(dropCancel);
    bool dropCancels = false;
    vnotemainwindow.onDropNote(dropCancels);
}
