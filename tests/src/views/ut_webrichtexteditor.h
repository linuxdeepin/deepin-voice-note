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
#ifndef UT_WEBRICHTEXTEDITOR_H
#define UT_WEBRICHTEXTEDITOR_H

#include "webrichtexteditor.h"
#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class UT_WebRichTextEditor : public QObject
    , public ::testing::Test
{
    Q_OBJECT
protected:
    virtual void SetUp() override;
    virtual void TearDown() override;

public:
    UT_WebRichTextEditor();
    WebRichTextEditor *m_web {nullptr};
    QWebEngineContextMenuData m_data;
    QWebEnginePage *stub_page();
    QWidget *stub_focusProxy();
    QWebEngineContextMenuData &stub_contextMenuData();
};

#endif // UT_WEBRICHTEXTEDITOR_H
