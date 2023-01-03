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
#ifndef UT_VNOTEFOLDEROPER_H
#define UT_VNOTEFOLDEROPER_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class VNoteFolderOper;
class UT_VNoteFolderOper : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_VNoteFolderOper();

    virtual void SetUp() override;
    virtual void TearDown() override;

protected:
    VNoteFolderOper *m_vnotefolderoper {nullptr};
};

#endif // UT_VNOTEFOLDEROPER_H
