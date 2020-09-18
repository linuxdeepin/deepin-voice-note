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

#include "ut_vnoterecordbar.h"

#define protected public
#define private public
#include "vnoterecordbar.h"
#undef protected
#undef private

#include "globaldef.h"
#include "vnoterecordwidget.h"
#include "vnoteiconbutton.h"
#include "vnoteplaywidget.h"
#include "vnoteitem.h"
#include "audiowatcher.h"
#include "setting.h"
#include "vnotemessagedialog.h"

ut_vnoterecordbar_test::ut_vnoterecordbar_test()
{

}

TEST_F(ut_vnoterecordbar_test, eventFilter)
{
    VNoteRecordBar vnoterecordbar;
    QEvent* event = new QEvent(QEvent::MouseButtonPress);
    QWidget* widget = new QWidget;
    vnoterecordbar.eventFilter(widget, event);
}

TEST_F(ut_vnoterecordbar_test, onStartRecord)
{
    VNoteRecordBar vnoterecordbar;
    vnoterecordbar.onStartRecord();
}

TEST_F(ut_vnoterecordbar_test, onFinshRecord)
{
    VNoteRecordBar vnoterecordbar;
    QString tmppath = "/usr/share/music/bensound-sunny.mp3";
    vnoterecordbar.onFinshRecord(tmppath, 2650);
}

TEST_F(ut_vnoterecordbar_test, cancelRecord)
{
    VNoteRecordBar vnoterecordbar;
}

TEST_F(ut_vnoterecordbar_test, onClosePlayWidget)
{
    VNoteRecordBar vnoterecordbar;
    VNVoiceBlock *vnvoiceblock = new VNVoiceBlock;
    vnvoiceblock->voicePath = "/usr/share/music/bensound-sunny.mp3";
    vnvoiceblock->voiceSize = 2650;
    vnvoiceblock->voiceTitle = "test";
    vnvoiceblock->state = true;
    vnoterecordbar.onClosePlayWidget(vnvoiceblock);
    vnoterecordbar.pauseVoice(vnvoiceblock);
    vnoterecordbar.playVoice(vnvoiceblock);
    vnoterecordbar.stopVoice(vnvoiceblock);
}

TEST_F(ut_vnoterecordbar_test, getVoiceData)
{
    VNoteRecordBar vnoterecordbar;
    vnoterecordbar.getVoiceData();
}

TEST_F(ut_vnoterecordbar_test, playOrPauseVoice)
{
    VNoteRecordBar vnoterecordbar;
    vnoterecordbar.playOrPauseVoice();
}

TEST_F(ut_vnoterecordbar_test, onAudioDeviceChange)
{
    VNoteRecordBar vnoterecordbar;
    vnoterecordbar.onAudioVolumeChange(vnoterecordbar.m_audioWatcher->Internal);
    vnoterecordbar.onAudioDeviceChange(vnoterecordbar.m_audioWatcher->Internal);
}

TEST_F(ut_vnoterecordbar_test, onAudioSelectChange)
{
    VNoteRecordBar vnoterecordbar;
    vnoterecordbar.onAudioSelectChange(1);
}

TEST_F(ut_vnoterecordbar_test, volumeToolow)
{
    VNoteRecordBar vnoterecordbar;
    vnoterecordbar.volumeToolow(1.1);
}
