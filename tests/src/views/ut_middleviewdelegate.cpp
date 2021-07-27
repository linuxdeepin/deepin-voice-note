/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ut_middleviewdelegate.h"
#include "middleview.h"
#include "common/vnotedatamanager.h"
#include "db/vnoteitemoper.h"
#include "common/vnoteforlder.h"
#include "middleviewdelegate.h"

#include <QLineEdit>
#include <QPainter>

ut_middleviewdelegate_test::ut_middleviewdelegate_test(QObject *parent)
    : QObject(parent)
{
}

TEST_F(ut_middleviewdelegate_test, ModifyTextAndPaint)
{
    VNoteFolder *folder = VNoteDataManager::instance()->getNoteFolders()->folders[0];
    VNoteItemOper noteOper;
    VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(folder->id);
    MiddleView view;
    if (notes) {
        notes->lock.lockForRead();
        for (auto it : notes->folderNotes) {
            view.appendRow(it);
        }
        notes->lock.unlock();
    }
    view.setCurrentIndex(0);
    QLineEdit edit;
    MiddleViewDelegate *delegate = view.m_pItemDelegate;
    QStyleOptionViewItem option;
    option.state.setFlag(QStyle::State_Enabled, true);
    option.rect = QRect(0, 0, 140, 84);
    delegate->createEditor(&view, option, view.currentIndex());
    delegate->updateEditorGeometry(&edit, option, view.currentIndex());
    delegate->setEditorData(&edit, view.currentIndex());
    edit.setText(QString("111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"));
    delegate->setModelData(&edit, view.model(), view.currentIndex());
    QPainter paint;
    bool isSelect = false;
    delegate->paintItemBase(&paint, option, option.rect, isSelect);
    option.state.setFlag(QStyle::State_MouseOver, true);
    delegate->paintItemBase(&paint, option, option.rect, isSelect);
    delegate->setEnableItem(false);
    delegate->paintItemBase(&paint, option, option.rect, isSelect);
    option.state.setFlag(QStyle::State_Selected, true);
    delegate->paintItemBase(&paint, option, option.rect, isSelect);
    delegate->setTabFocus(true);
    delegate->paintItemBase(&paint, option, option.rect, isSelect);
}
