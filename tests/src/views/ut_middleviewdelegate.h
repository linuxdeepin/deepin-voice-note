/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
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
#ifndef UT_MIDDLEVIEWDELEGATE_H
#define UT_MIDDLEVIEWDELEGATE_H
#include "gtest/gtest.h"

#include <QObject>

class UT_MiddleViewDelegate : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    explicit UT_MiddleViewDelegate(QObject *parent = nullptr);

signals:

public slots:
};

#endif // UT_MIDDLEVIEWDELEGATE_H
