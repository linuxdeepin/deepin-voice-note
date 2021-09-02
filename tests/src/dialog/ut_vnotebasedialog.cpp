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
#include "vnotebasedialog.h"
#include <QVBoxLayout>

ut_vnotebasedialog_test::ut_vnotebasedialog_test()
{
}

TEST_F(ut_vnotebasedialog_test, setTitle)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.setTitle("test");
    EXPECT_EQ("test", vnotebasedialog.m_tileText->text());
}

TEST_F(ut_vnotebasedialog_test, getContentLayout)
{
    VNoteBaseDialog vnotebasedialog;
    EXPECT_TRUE(vnotebasedialog.getContentLayout());
}

TEST_F(ut_vnotebasedialog_test, setLogoVisable)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.setLogoVisable(true);
    EXPECT_FALSE(vnotebasedialog.m_logoIcon->isVisible()) << "visible is true";
    vnotebasedialog.setLogoVisable(false);
    EXPECT_FALSE(vnotebasedialog.m_logoIcon->isVisible()) << "visible is false";
}

TEST_F(ut_vnotebasedialog_test, setIconPixmap)
{
    VNoteBaseDialog vnotebasedialog;
    QPixmap iconpixmap;
    vnotebasedialog.setIconPixmap(iconpixmap);
    EXPECT_EQ(iconpixmap, *(vnotebasedialog.m_logoIcon->pixmap()));
}

TEST_F(ut_vnotebasedialog_test, addContent)
{
    VNoteBaseDialog vnotebasedialog;
    QWidget *content = new QWidget;
    vnotebasedialog.addContent(content);
    EXPECT_EQ(1, vnotebasedialog.m_contentLayout->count());
    delete content;
}

TEST_F(ut_vnotebasedialog_test, closeEvent)
{
    VNoteBaseDialog vnotebasedialog;
    QCloseEvent *event = new QCloseEvent;
    vnotebasedialog.closeEvent(event);
    delete event;
}

TEST_F(ut_vnotebasedialog_test, showEvent)
{
    VNoteBaseDialog vnotebasedialog;
    QShowEvent *event = new QShowEvent;
    vnotebasedialog.showEvent(event);
    EXPECT_EQ(QSize(380, 140), vnotebasedialog.size());
    delete event;
}
