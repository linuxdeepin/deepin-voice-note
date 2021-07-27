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

#include "ut_leftviewdelegate.h"
#include "leftviewdelegate.h"
#include "leftview.h"

#include "common/vnotedatamanager.h"
#include "common/standarditemcommon.h"

#include <QLineEdit>
#include <QStyleOptionViewItem>
#include <QPainter>

ut_leftviewdelegate_test::ut_leftviewdelegate_test()
{
}

TEST_F(ut_leftviewdelegate_test, paintNoteItem)
{
    LeftView view;
    for (auto it : VNoteDataManager::instance()->getNoteFolders()->folders) {
        view.addFolder(it);
    }
    view.setDefaultNotepadItem();
    LeftViewDelegate *delegate = view.m_pItemDelegate;
    QLineEdit edit;
    edit.setText(QString("111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"));
    delegate->setModelData(&edit, view.model(), view.currentIndex());
    QStyleOptionViewItem option;
    option.state.setFlag(QStyle::State_MouseOver, true);
    option.rect = QRect(0, 0, 40, 30);
    delegate->setDragState(true);
    QPainter paint;
    delegate->paintNoteItem(&paint, option, view.currentIndex());
    delegate->setEnableItem(true);
    option.state.setFlag(QStyle::State_Enabled, true);
    delegate->paintNoteItem(&paint, option, view.currentIndex());
    delegate->setDragState(false);
    delegate->paintNoteItem(&paint, option, view.currentIndex());
    delegate->setDrawHover(true);
    delegate->paintNoteItem(&paint, option, view.currentIndex());
    delegate->paintTabFocusBackground(&paint, option, option.rect);
}
