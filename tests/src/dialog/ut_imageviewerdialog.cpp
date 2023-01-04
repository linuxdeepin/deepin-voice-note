/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     leilong <leilong@uniontech.com>
*
* Maintainer: leilong <leilong@uniontech.com>
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
#include "ut_imageviewerdialog.h"
#include "dialog/imageviewerdialog.h"

UT_Imageviewerdialog::UT_Imageviewerdialog()
{
}

TEST_F(UT_Imageviewerdialog, ut_imageviewerdialog_open_001)
{
    ImageViewerDialog image;
    image.open("/tmp/1.jpg");
}

TEST_F(UT_Imageviewerdialog, UT_Imageviewerdialog_initUI_001)
{
    ImageViewerDialog image;
    EXPECT_TRUE(image.m_imgLabel) << "m_imgLabel";
    EXPECT_EQ("ImageLabel", image.m_imgLabel->objectName()) << "m_imgLabel.objectName";
    EXPECT_TRUE(image.m_closeButton) << "m_closeButton";
    EXPECT_EQ("CloseButton", image.m_closeButton->objectName()) << "m_closeButton.objectName";
}

TEST_F(UT_Imageviewerdialog, ut_imageviewerdialog_paintEvent_001)
{
    ImageViewerDialog image;
    image.paintEvent(nullptr);
}
