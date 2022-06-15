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
#ifndef UT_FILECLEANUPWORKER_H
#define UT_FILECLEANUPWORKER_H

#include "filecleanupworker.h"
#include "common/vnoteitem.h"
#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class UT_FileCleanupWorker : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_FileCleanupWorker();

protected:
    virtual void SetUp() override;
    virtual void TearDown() override;

private:
    VNOTE_ALL_NOTES_MAP *qspAllNotesMap {nullptr};
    VNOTE_ITEMS_MAP *voiceItem {nullptr};
};

#endif // UT_FILECLEANUPWORKER_H
