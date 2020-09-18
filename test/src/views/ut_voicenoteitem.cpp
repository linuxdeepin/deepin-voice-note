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

#include "ut_voicenoteitem.h"

#define protected public
#include "voicenoteitem.h"
#undef protected

#include "textnoteedit.h"
#include "vnoteitem.h"
#include "utils.h"
#include "actionmanager.h"
#include "vnote2siconbutton.h"

#include <QTimer>

ut_voicenoteitem_test::ut_voicenoteitem_test()
{

}

TEST_F(ut_voicenoteitem_test, onPlayBtnClicked)
{
    QPoint pos;
    QTimer* timer = new QTimer();
    timer->setInterval(300);
    VNOTE_DATAS datas;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Voice);
    VoiceNoteItem voicenoteitem(block);
    voicenoteitem.onPlayBtnClicked();
    voicenoteitem.showAsrStartWindow();
    voicenoteitem.showAsrEndWindow("test");
    voicenoteitem.enblePlayBtn(true);
    voicenoteitem.asrTextNotEmpty();
    voicenoteitem.selectText(pos, QTextCursor::Start);
    voicenoteitem.selectText(QTextCursor::Start);
    voicenoteitem.selectAllText();
    voicenoteitem.clearSelection();
    voicenoteitem.getSelectFragment();
    voicenoteitem.getTextDocument();
    voicenoteitem.hasSelection();
    voicenoteitem.removeSelectText();
    voicenoteitem.getTextCursor();
    voicenoteitem.textIsEmpty();
    voicenoteitem.getCursorRect();
    voicenoteitem.setFocus();
    voicenoteitem.hasFocus();
    voicenoteitem.isTextContainsPos(pos);

    PlayAnimInferface* playaniminferface = new VoiceNoteItem(block);
    playaniminferface->setAnimTimer(timer);
    playaniminferface->startAnim();
    playaniminferface->stopAnim();
    playaniminferface->updateAnim();
}
