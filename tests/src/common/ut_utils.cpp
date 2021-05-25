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

#include "ut_utils.h"
#include "utils.h"
#include "textnoteedit.h"
#include "datatypedef.h"
#include "vnoteitem.h"

#include <DStyle>
#include <DApplicationHelper>

ut_utils_test::ut_utils_test()
{
}

void ut_utils_test::SetUp()
{
    m_utils = new Utils;
}

void ut_utils_test::TearDown()
{
    delete m_utils;
}

TEST_F(ut_utils_test, convertDateTime)
{
    QDateTime currDateTime = QDateTime::currentDateTime();
    m_utils->convertDateTime(currDateTime);
    m_utils->convertDateTime(currDateTime.addDays(-1));
    m_utils->convertDateTime(currDateTime.addMonths(-1));
    m_utils->convertDateTime(currDateTime.addSecs(-3600));
    m_utils->convertDateTime(currDateTime.addYears(-1));
}

TEST_F(ut_utils_test, loadSVG)
{
    QString fileName = "play.svg";
    m_utils->loadSVG(fileName, false);
    m_utils->loadSVG(fileName, true);
}

TEST_F(ut_utils_test, highTextEdit)
{
    QTextDocument document;
    document.setPlainText("test");
    QString searchKey = "test";
    DPalette pb;
    QColor highColor = pb.color(DPalette::Highlight);
    m_utils->highTextEdit(&document, searchKey, highColor);
    m_utils->highTextEdit(&document, searchKey, highColor, true);
}

TEST_F(ut_utils_test, setDefaultColor)
{
    QTextDocument document;
    DPalette pb;
    QColor highColor = pb.color(DPalette::Highlight);
    m_utils->setDefaultColor(&document, highColor);
}

TEST_F(ut_utils_test, blockToDocument)
{
    VNTextBlock block;
    QTextDocument document;
    m_utils->blockToDocument(&block, &document);
}

TEST_F(ut_utils_test, documentToBlock)
{
    VNTextBlock block;
    QTextDocument document;
    document.setPlainText("test");
    m_utils->documentToBlock(&block, &document);
}

TEST_F(ut_utils_test, formatMillisecond)
{
    qint64 tmpvoicesize = 890;
    m_utils->formatMillisecond(tmpvoicesize, 4);
    tmpvoicesize = 18901111;
    m_utils->formatMillisecond(tmpvoicesize);
}
