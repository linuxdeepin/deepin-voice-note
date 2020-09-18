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

#include "ut_textnoteitem.h"

#define private public
#define protected public
#include "datatypedef.h"
#include "textnoteitem.h"
#include "textnoteedit.h"
#include "utils.h"
#undef private
#undef protected

ut_textnoteitem_test::ut_textnoteitem_test()
{

}

TEST_F(ut_textnoteitem_test, getNoteBlock)
{
    QPoint pos;
    VNOTE_DATAS datas;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Text);
    TextNoteItem textnoteitem(block);
    textnoteitem.getNoteBlock();
    textnoteitem.textIsEmpty();
    textnoteitem.getCursorRect();
    textnoteitem.selectText(pos, QTextCursor::Start);
    textnoteitem.selectText(QTextCursor::Start);
    textnoteitem.selectAllText();
    textnoteitem.removeSelectText();
    textnoteitem.clearSelection();
    textnoteitem.hasSelection();
    textnoteitem.getSelectFragment();
    textnoteitem.setFocus();
    textnoteitem.hasFocus();
    textnoteitem.isSelectAll();
    textnoteitem.getTextDocument();
    textnoteitem.pasteText();
    textnoteitem.onTextChange();
    textnoteitem.onTextCursorChange();
    textnoteitem.setLastCursorHeight(12);
    textnoteitem.onChangeTheme();
}
