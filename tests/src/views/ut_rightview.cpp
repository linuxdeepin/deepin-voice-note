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
#include "rightview.h"
#include "datatypedef.h"
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

#include "stub.h"

#include <DFileDialog>
DWIDGET_USE_NAMESPACE

ut_rightview_test::ut_rightview_test()
{
}

void ut_rightview_test::SetUp()
{
    VNOTE_ALL_NOTES_MAP *notes = VNoteDataManager::instance()->getAllNotesInFolder();
    if (notes && !notes->notes.isEmpty()) {
        VNOTE_ITEMS_MAP *tmp = notes->notes.first();
        if (tmp && !tmp->folderNotes.isEmpty()) {
            m_noteItem = tmp->folderNotes.first();
        }
    }
}

static void stub_onMenuShow(DetailItemWidget *widget)
{
    Q_UNUSED(widget)
}

static bool stub_checkFileExist(const QString &file)
{
    Q_UNUSED(file)
    return true;
}

static int stub_dialog()
{
    return 1;
}

TEST_F(ut_rightview_test, insertTextEdit)
{
    RightView rightview;
    VNTextBlock block;
    rightview.insertTextEdit(&block, true, QTextCursor::Up, "test");
    rightview.insertTextEdit(&block, false, QTextCursor::Up, "test");
}

TEST_F(ut_rightview_test, insertVoiceItem)
{
    RightView rightview;
    VNoteItem noteItem;
    rightview.initData(&noteItem, "");
    rightview.initAction(rightview.m_curItemWidget);
    if (rightview.m_curItemWidget) {
        QTextCursor cursor = rightview.m_curItemWidget->getTextCursor();
        cursor.insertText("test");
        cursor.setPosition(1);
        rightview.m_curItemWidget->setTextCursor(cursor);
    }
    QString tmppath = "/usr/share/music/bensound-sunny.mp3";
    DetailItemWidget *widget1 = rightview.insertVoiceItem(tmppath, 2650);
    DetailItemWidget *widget2 = rightview.insertVoiceItem(tmppath, 2650);
    rightview.delWidget(widget1, true);
    int index = rightview.layout()->indexOf(widget2);
    DetailItemWidget *widget3 = static_cast<DetailItemWidget *>(rightview.layout()->itemAt(index - 1)->widget());
    rightview.m_curItemWidget = widget3;
    rightview.setCurVoiceAsr(static_cast<VoiceNoteItem *>(widget3));
    rightview.setCurVoicePlay(static_cast<VoiceNoteItem *>(widget3));
    rightview.delWidget(widget3, true);
}

TEST_F(ut_rightview_test, onTextEditFocusIn)
{
    RightView rightview;
    VNTextBlock block;
    rightview.insertTextEdit(&block, false, QTextCursor::Up, "test");
    rightview.onTextEditFocusIn(Qt::TabFocusReason);
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

TEST_F(ut_rightview_test, mousePressEvent)
{
    Stub stub;
    stub.set(ADDR(RightView, onMenuShow), stub_onMenuShow);
    RightView rightview;
    QPointF localPos;
    rightview.initData(m_noteItem, "");
    QMouseEvent *mousePressEvent = new QMouseEvent(QEvent::MouseButtonPress, localPos, localPos, localPos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    rightview.mousePressEvent(mousePressEvent);
    delete mousePressEvent;
}

TEST_F(ut_rightview_test, removeSelectWidget)
{
    RightView rightview;
    VNTextBlock block;
    DetailItemWidget *widget = rightview.insertTextEdit(&block, true, QTextCursor::Up, "test");
    rightview.m_selectWidget.insert(TextEditPlugin, widget);
    rightview.removeSelectWidget(widget);
}

TEST_F(ut_rightview_test, removeCacheWidget)
{
    RightView rightview;
    rightview.initData(m_noteItem, "");
    rightview.initData(m_noteItem, "");
    rightview.removeCacheWidget(m_noteItem->folder());
    rightview.initData(nullptr, "");
}

TEST_F(ut_rightview_test, adjustVoiceVerticalScrollBar)
{
    RightView rightview;
    rightview.initData(m_noteItem, "");
    VNTextBlock block;
    DetailItemWidget *widget = rightview.insertTextEdit(&block, true, QTextCursor::Up, "test");
    rightview.adjustVoiceVerticalScrollBar(widget, 28);
}

TEST_F(ut_rightview_test, onVoicePlay)
{
    typedef int (*fptr)();
    fptr A_foo = (fptr)(&DFileDialog::exec);
    Stub stub;
    stub.set(ADDR(RightView, checkFileExist), stub_checkFileExist);
    stub.set(A_foo, stub_dialog);
    RightView rightview;
    rightview.initData(m_noteItem, "");
    QVBoxLayout *layout = static_cast<QVBoxLayout *>(rightview.layout());
    for (int i = 0; i < layout->count() - 1; i++) {
        QLayoutItem *layoutItem = layout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget && widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
            VoiceNoteItem *voiceWidget = static_cast<VoiceNoteItem *>(widget);
            rightview.onVoicePlay(voiceWidget);
            rightview.setCurVoicePlay(voiceWidget);
            rightview.getCurVoicePlay();
            rightview.onPlayUpdate();
            rightview.setCurVoiceAsr(voiceWidget);
            rightview.getCurVoiceAsr();
            rightview.m_selectWidget.insert(VoicePlugin, voiceWidget);
            voiceWidget->selectAllText();
            rightview.saveMp3();
            break;
        }
    }
}

TEST_F(ut_rightview_test, onVoicePause)
{
    Stub stub;
    stub.set(ADDR(RightView, checkFileExist), stub_checkFileExist);
    RightView rightview;
    rightview.initData(m_noteItem, "");
    QVBoxLayout *layout = static_cast<QVBoxLayout *>(rightview.layout());
    for (int i = 0; i < layout->count() - 1; i++) {
        QLayoutItem *layoutItem = layout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget && widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
            rightview.onVoicePause(static_cast<VoiceNoteItem *>(widget));
            break;
        }
    }
}

TEST_F(ut_rightview_test, leaveEvent)
{
    RightView rightview;
    QEvent *event = new QEvent(QEvent::MouseMove);
    rightview.leaveEvent(event);
    delete event;
}

TEST_F(ut_rightview_test, mouseEvent)
{
    RightView rightview;
    rightview.initData(m_noteItem, "");
    QPointF localPos;
    QPointF pointF(30, 15);
    QPoint point(30, 15);
    QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, pointF, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    rightview.m_curItemWidget = rightview.getWidgetByPos(point);
    rightview.mouseMoveEvent(event);
    QMouseEvent *event1 = new QMouseEvent(QEvent::MouseButtonPress, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    rightview.mouseReleaseEvent(event1);
    delete event;
    delete event1;
}

TEST_F(ut_rightview_test, keyPressEvent)
{
    RightView rightview;
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, 0x43, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event);
    QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, 0x41, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event1);
    QKeyEvent *event2 = new QKeyEvent(QEvent::KeyPress, 0x58, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event2);
    QKeyEvent *event3 = new QKeyEvent(QEvent::KeyPress, 0x56, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event3);
    QKeyEvent *event4 = new QKeyEvent(QEvent::KeyPress, 0x59, Qt::ControlModifier, "test");
    rightview.keyPressEvent(event4);
    QKeyEvent *event5 = new QKeyEvent(QEvent::KeyPress, 0x01000007, Qt::NoModifier, "test");
    rightview.keyPressEvent(event5);

    delete event;
    delete event1;
    delete event2;
    delete event3;
    delete event4;
    delete event5;
}

TEST_F(ut_rightview_test, delSelectText)
{
    RightView rightview;
    VNTextBlock block;
    block.blockText = "test";
    DetailItemWidget *widget = rightview.insertTextEdit(&block, false, QTextCursor::Up, "test");
    rightview.selectAllItem();
    rightview.delSelectText();
    QTextCursor cursor = widget->getTextCursor();
    cursor.insertText("blocktext test");
    cursor.setPosition(0);
    cursor.setPosition(2, QTextCursor::KeepAnchor);
    widget->setTextCursor(cursor);
    qInfo() << widget->getSelectFragment().toPlainText();
    rightview.m_selectWidget.insert(TextEditPlugin, widget);
    rightview.delSelectText();
}

TEST_F(ut_rightview_test, closeMenu)
{
    RightView rightview;
    rightview.closeMenu();
}

TEST_F(ut_rightview_test, setIsNormalView)
{
    RightView rightview;
    rightview.setIsNormalView(false);
    rightview.getIsNormalView();
}

TEST_F(ut_rightview_test, initAction)
{
    Stub stub;
    stub.set(ADDR(RightView, checkFileExist), stub_checkFileExist);

    RightView rightview;
    rightview.initData(m_noteItem, "");

    OpsStateInterface *stateInterface = OpsStateInterface::instance();
    stateInterface->operState(OpsStateInterface::StateAISrvAvailable, true);
    QList<VoiceNoteItem *> voiceItems;
    QList<TextNoteItem *> textItems;

    QVBoxLayout *layout = static_cast<QVBoxLayout *>(rightview.layout());
    for (int i = 0; i < layout->count() - 1; i++) {
        QLayoutItem *layoutItem = layout->itemAt(i);
        DetailItemWidget *widget = static_cast<DetailItemWidget *>(layoutItem->widget());
        if (widget && widget->getNoteBlock()->blockType == VNoteBlock::Voice) {
            voiceItems.push_back(static_cast<VoiceNoteItem *>(widget));
        } else {
            textItems.push_back(static_cast<TextNoteItem *>(widget));
        }
    }
    textItems[0]->setFocus(false);
    rightview.m_selectWidget.insert(TextEditPlugin, textItems[0]);
    rightview.initAction(textItems[0]);

    textItems[0]->selectAllText();
    rightview.initAction(textItems[0]);

    rightview.m_selectWidget.insert(VoicePlugin, voiceItems[0]);
    rightview.initAction(voiceItems[0]);

    rightview.m_selectWidget.clear();
    rightview.m_selectWidget.insert(VoicePlugin, voiceItems[0]);

    rightview.initAction(voiceItems[0]);
    voiceItems[0]->selectAllText();
    rightview.initAction(voiceItems[0]);
    rightview.m_selectWidget.insert(VoicePlugin, voiceItems[1]);
    rightview.initAction(voiceItems[0]);
}

TEST_F(ut_rightview_test, checkFileExist)
{
    typedef int (*fptr)();
    fptr A_foo = (fptr)(&DDialog::exec);
    Stub stub;
    stub.set(A_foo, stub_dialog);
    RightView rightview;
    rightview.checkFileExist("/test");
}

TEST_F(ut_rightview_test, copySelectText)
{
    RightView rightview;
    rightview.initData(m_noteItem, "");
    rightview.selectAllItem();
    rightview.copySelectText();
}

TEST_F(ut_rightview_test, setEnablePlayBtn)
{
    RightView rightview;
    rightview.initData(m_noteItem, "");
    rightview.setEnablePlayBtn(false);
}

TEST_F(ut_rightview_test, refreshVoiceCreateTime)
{
    RightView rightview;
    rightview.initData(m_noteItem, "");
    rightview.refreshVoiceCreateTime();
}
