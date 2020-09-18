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
#define protected public
#include "datatypedef.h"
#undef protected
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

    QDate currDate1(2020, 8, 19);
    QDateTime currDateTime1 = QDateTime::currentDateTime();
    m_utils->convertDateTime(currDateTime1);

    QDate currDate2(2020, 8, 19);
    QDateTime currDateTime2(currDate2);
    m_utils->convertDateTime(currDateTime2);
}

TEST_F(ut_utils_test, loadSVG)
{
    QString fileName = "play.svg";
    m_utils->loadSVG(fileName, false);
    m_utils->loadSVG(fileName, true);
}

TEST_F(ut_utils_test, highTextEdit)
{
//    TextNoteEdit *m_textEdit = new TextNoteEdit();
//    m_textEdit->setContextMenuPolicy(Qt::NoContextMenu);
//    m_textEdit->document()->setDocumentMargin(0);
//    DStyle::setFocusRectVisible(m_textEdit, false);
//    DPalette pb = DApplicationHelper::instance()->palette(m_textEdit);
//    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
//    m_textEdit->setPalette(pb);
    QTextDocument *document = new QTextDocument("test");
    QString searchKey = "test";
    DPalette pb;
    QColor highColor = pb.color(DPalette::Highlight);
    m_utils->highTextEdit(document, searchKey, highColor, true);
}

TEST_F(ut_utils_test, setDefaultColor)
{
    QTextDocument *document = new QTextDocument("test");
    DPalette pb;
    QColor highColor = pb.color(DPalette::Highlight);
    m_utils->setDefaultColor(document, highColor);
}

TEST_F(ut_utils_test, blockToDocument)
{
    VNOTE_DATAS datas;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Text);
    QTextDocument *document = new QTextDocument("test");
    m_utils->blockToDocument(block, document);
}

TEST_F(ut_utils_test, documentToBlock)
{
    VNOTE_DATAS datas;
    VNoteBlock *block = datas.newBlock(VNoteBlock::Text);
    VNoteItem *m_noteItemData = new VNoteItem;
    m_noteItemData->addBlock(block);
    QTextDocument *document = new QTextDocument("test");
    m_utils->documentToBlock(block, document);
}

TEST_F(ut_utils_test, formatMillisecond)
{
    qint64 tmpvoicesize = 890;
    m_utils->formatMillisecond(tmpvoicesize, 4);
    tmpvoicesize = 18901111;
    m_utils->formatMillisecond(tmpvoicesize);
}


























