// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_vnotebasedialog.h"
#include "vnotebasedialog.h"
#include <QVBoxLayout>

UT_VNoteBaseDialog::UT_VNoteBaseDialog()
{
}

TEST_F(UT_VNoteBaseDialog, UT_VNoteBaseDialog_setTitle_001)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.setTitle("test");
    EXPECT_EQ("test", vnotebasedialog.m_tileText->text());
}

TEST_F(UT_VNoteBaseDialog, UT_VNoteBaseDialog_getContentLayout_001)
{
    VNoteBaseDialog vnotebasedialog;
    EXPECT_TRUE(vnotebasedialog.getContentLayout());
}

TEST_F(UT_VNoteBaseDialog, UT_VNoteBaseDialog_setLogoVisable_001)
{
    VNoteBaseDialog vnotebasedialog;
    vnotebasedialog.setLogoVisable(true);
    EXPECT_FALSE(vnotebasedialog.m_logoIcon->isVisible()) << "visible is true";
    vnotebasedialog.setLogoVisable(false);
    EXPECT_FALSE(vnotebasedialog.m_logoIcon->isVisible()) << "visible is false";
}

TEST_F(UT_VNoteBaseDialog, UT_VNoteBaseDialog_setIconPixmap_001)
{
    VNoteBaseDialog vnotebasedialog;
    QPixmap iconpixmap;
    vnotebasedialog.setIconPixmap(iconpixmap);
    EXPECT_EQ(iconpixmap, *(vnotebasedialog.m_logoIcon->pixmap()));
}

TEST_F(UT_VNoteBaseDialog, UT_VNoteBaseDialog_addContent_001)
{
    VNoteBaseDialog vnotebasedialog;
    QWidget *content = new QWidget;
    vnotebasedialog.addContent(content);
    EXPECT_EQ(1, vnotebasedialog.m_contentLayout->count());
    delete content;
}

TEST_F(UT_VNoteBaseDialog, UT_VNoteBaseDialog_closeEvent_001)
{
    VNoteBaseDialog vnotebasedialog;
    QCloseEvent *event = new QCloseEvent;
    vnotebasedialog.closeEvent(event);
    delete event;
}

TEST_F(UT_VNoteBaseDialog, UT_VNoteBaseDialog_showEvent_001)
{
    VNoteBaseDialog vnotebasedialog;
    QShowEvent *event = new QShowEvent;
    vnotebasedialog.showEvent(event);
    EXPECT_EQ(QSize(380, 140), vnotebasedialog.size());
    delete event;
}
