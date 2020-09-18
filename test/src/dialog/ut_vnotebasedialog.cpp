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

#include "ut_vnotebasedialog.h"

#define protected public
#include "vnotebasedialog.h"
#undef protected

ut_vnotebasedialog_test::ut_vnotebasedialog_test()
{

}

TEST_F(ut_vnotebasedialog_test, initUI)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.initUI();
}

TEST_F(ut_vnotebasedialog_test, setTitle)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.setTitle("test");
}

TEST_F(ut_vnotebasedialog_test, getContentLayout)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.getContentLayout();
}

TEST_F(ut_vnotebasedialog_test, setLogoVisable)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.setLogoVisable();
}

TEST_F(ut_vnotebasedialog_test, setIconPixmap)
{
    VNoteBaseDialog vnotebasedialog;
    QPixmap iconpixmap;
    vnotebasedialog.setIconPixmap(iconpixmap);
}

TEST_F(ut_vnotebasedialog_test, addContent)
{
    VNoteBaseDialog vnotebasedialog;
    QWidget *content = new QWidget;
    vnotebasedialog.addContent(content);
}

TEST_F(ut_vnotebasedialog_test, closeEvent)
{
    VNoteBaseDialog vnotebasedialog;
    QCloseEvent *event = new QCloseEvent;
    vnotebasedialog.closeEvent(event);
}

TEST_F(ut_vnotebasedialog_test, showEvent)
{
    VNoteBaseDialog vnotebasedialog;
    QShowEvent *event = new QShowEvent;
    vnotebasedialog.showEvent(event);
}

