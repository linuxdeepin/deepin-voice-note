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

#include "ut_rightview.h"

#define protected public
#define private public
#include "rightview.h"
#include "datatypedef.h"
#undef protected
#undef private

#include "textnoteitem.h"
#include "voicenoteitem.h"
#include "globaldef.h"
#include "vnoteapplication.h"
#include "common/utils.h"

#include "vnoteforlder.h"
#include "vnoteitem.h"
#include "vnotedatamanager.h"
#include "actionmanager.h"
#include "exportnoteworker.h"
#include "opsstateinterface.h"
#include "vnoteitemoper.h"
#include "setting.h"

#include "vnotemessagedialog.h"

ut_rightview_test::ut_rightview_test()
{

}

TEST_F(ut_rightview_test, insertTextEdit)
{
    RightView rightview;
    VNOTE_DATAS datas;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Text);
    rightview.insertTextEdit(block, true, QTextCursor::Up, "test");
}

TEST_F(ut_rightview_test, insertVoiceItem)
{
    RightView rightview;
    QString tmppath = "/usr/share/music/bensound-sunny.mp3";
    rightview.insertVoiceItem(tmppath, 2650);
}

TEST_F(ut_rightview_test, onTextEditFocusIn)
{
    RightView rightview;
    rightview.onTextEditFocusIn();
}

TEST_F(ut_rightview_test, onTextEditFocusOut)
{
    RightView rightview;
    rightview.onTextEditFocusOut();
}

TEST_F(ut_rightview_test, onTextEditTextChange)
{
    RightView rightview;
    rightview.onTextEditTextChange();
}

TEST_F(ut_rightview_test, initData)
{
    RightView rightview;
    VNoteItem *vnoteitem = new VNoteItem;
    vnoteitem->noteId = 2;
    vnoteitem->folderId = 2;
    vnoteitem->noteTitle = "test";
    rightview.initData(vnoteitem, "test", true);
}

TEST_F(ut_rightview_test, onVoicePlay)
{
    RightView rightview;
    VNOTE_DATAS datas;
    VNVoiceBlock *vnvoiceblock = new VNVoiceBlock;
    vnvoiceblock->voicePath = "/usr/share/music/bensound-sunny.mp3";
    vnvoiceblock->voiceSize = 2650;
    vnvoiceblock->voiceTitle = "test";
    vnvoiceblock->state = true;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Voice);
    block->ptrVoice = vnvoiceblock;
    VoiceNoteItem *item = new VoiceNoteItem(block);
    rightview.onVoicePlay(item);
}

TEST_F(ut_rightview_test, onVoicePause)
{
    RightView rightview;
    VNOTE_DATAS datas;
    VNVoiceBlock *vnvoiceblock = new VNVoiceBlock;
    vnvoiceblock->voicePath = "/usr/share/music/bensound-sunny.mp3";
    vnvoiceblock->voiceSize = 2650;
    vnvoiceblock->voiceTitle = "test";
    vnvoiceblock->state = true;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Voice);
    block->ptrVoice = vnvoiceblock;
    VoiceNoteItem *item = new VoiceNoteItem(block);
    rightview.onVoicePause(item);
}

TEST_F(ut_rightview_test, onPlayUpdate)
{
    RightView rightview;
    rightview.onPlayUpdate();
}

TEST_F(ut_rightview_test, leaveEvent)
{
    RightView rightview;
    QEvent* event = new QEvent(QEvent::MouseMove);
    rightview.leaveEvent(event);
}

TEST_F(ut_rightview_test, mouseEvent)
{
    RightView rightview;
    VNoteItem *vnoteitem = new VNoteItem;
    vnoteitem->noteId = 2;
    vnoteitem->folderId = 2;
    vnoteitem->noteTitle = "test";
    rightview.m_noteItemData = vnoteitem;
    QPointF localPos;
    QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    rightview.mousePressEvent(event);
    QMouseEvent* event1 = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    rightview.mouseReleaseEvent(event1);
    rightview.mouseMoveEvent(event);
}

TEST_F(ut_rightview_test, keyPressEvent)
{
    RightView rightview;
    QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, 0x43, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event);
    QKeyEvent* event1 = new QKeyEvent(QEvent::KeyPress, 0x41, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event1);
    QKeyEvent* event2 = new QKeyEvent(QEvent::KeyPress, 0x58, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event2);
    QKeyEvent* event3 = new QKeyEvent(QEvent::KeyPress, 0x56, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event3);
    QKeyEvent* event4 = new QKeyEvent(QEvent::KeyPress, 0x59, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event4);
    QKeyEvent* event5 = new QKeyEvent(QEvent::KeyPress, 0x01000007, Qt::NoModifier, "test");
    rightview.keyPressEvent(event5);
}

TEST_F(ut_rightview_test, delSelectText)
{
    RightView rightview;
    rightview.delSelectText();
}

TEST_F(ut_rightview_test, setCurVoicePlay)
{
    RightView rightview;
    VNOTE_DATAS datas;
    VNVoiceBlock *vnvoiceblock = new VNVoiceBlock;
    vnvoiceblock->voicePath = "/usr/share/music/bensound-sunny.mp3";
    vnvoiceblock->voiceSize = 2650;
    vnvoiceblock->voiceTitle = "test";
    vnvoiceblock->state = true;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Voice);
    block->ptrVoice = vnvoiceblock;
    VoiceNoteItem *item = new VoiceNoteItem(block);
    rightview.setCurVoicePlay(item);
    rightview.setCurVoiceAsr(item);
    rightview.getCurVoicePlay();
    rightview.getCurVoiceAsr();
}

TEST_F(ut_rightview_test, saveMp3)
{
    RightView rightview;
    rightview.saveMp3();
}

TEST_F(ut_rightview_test, getOnlyOneSelectVoice)
{
    RightView rightview;
    rightview.getOnlyOneSelectVoice();
}

TEST_F(ut_rightview_test, closeMenu)
{
    RightView rightview;
    rightview.closeMenu();
}



