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
#ifndef UT_LOADFOLDERWORKER_H
#define UT_LOADFOLDERWORKER_H

#include <QObject>
#include "gtest/gtest.h"
struct VNOTE_FOLDERS_MAP;

class ut_loadfolderworker_test : public QObject , public ::testing::Test
{
    Q_OBJECT
public:
    explicit ut_loadfolderworker_test(QObject *parent = nullptr);

signals:

public slots:
    void onFolderLoad(VNOTE_FOLDERS_MAP *folders);
};

#endif // UT_LOADFOLDERWORKER_H
