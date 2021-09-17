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
#include "ut_jscontent.h"
#include "jscontent.h"

UT_JsContent::UT_JsContent()
{
}

TEST_F(UT_JsContent, UT_JsContent_instance_001)
{
    JsContent *instance = JsContent::instance();
    EXPECT_EQ(instance, JsContent::instance());
}

TEST_F(UT_JsContent, UT_JsContent_insertImages_001)
{
    JsContent *instance = JsContent::instance();
    EXPECT_FALSE(instance->insertImages(QStringList("/tmp/1.jpg"))) << "1.jpg";
    EXPECT_FALSE(instance->insertImages(QStringList("1.bmp"))) << "1.bmp";
    EXPECT_FALSE(instance->insertImages(QStringList("1.mp3"))) << "1.mp3";
}

TEST_F(UT_JsContent, UT_JsContent_insertImages_002)
{
    JsContent *instance = JsContent::instance();
    QImage image;
    EXPECT_FALSE(instance->insertImages(image));
}

TEST_F(UT_JsContent, UT_JsContent_jsCallTxtChange_001)
{
    JsContent::instance()->jsCallTxtChange();
}

TEST_F(UT_JsContent, UT_JsContent_jsCallChannleFinish_001)
{
    JsContent::instance()->jsCallChannleFinish();
}

TEST_F(UT_JsContent, UT_JsContent_jsCallPopupMenu_001)
{
    JsContent::instance()->jsCallPopupMenu(1, QVariant());
}

TEST_F(UT_JsContent, UT_JsContent_jsCallPlayVoice_001)
{
    JsContent::instance()->jsCallPlayVoice(QVariant(), true);
}

TEST_F(UT_JsContent, UT_JsContent_jsCallGetTranslation_001)
{
    EXPECT_FALSE(JsContent::instance()->jsCallGetTranslation().isEmpty());
}

TEST_F(UT_JsContent, UT_JsContent_callJsSynchronous_001)
{
    EXPECT_TRUE(JsContent::instance()->callJsSynchronous(nullptr, "").isNull());
}

TEST_F(UT_JsContent, UT_JsContent_jsCallSetDataFinsh_001)
{
    JsContent::instance()->jsCallSetDataFinsh();
}

TEST_F(UT_JsContent, UT_JsContent_jsCallPaste_001)
{
    JsContent::instance()->jsCallPaste();
}

TEST_F(UT_JsContent, UT_JsContent_jsCallViewPicture_001)
{
    JsContent::instance()->jsCallViewPicture("1.jpg");
}

TEST_F(UT_JsContent, UT_JsContent_jsCallCreateNote_001)
{
    JsContent::instance()->jsCallCreateNote();
}
