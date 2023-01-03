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
#ifndef UT_LOADNOTEITEMSWORKER_H
#define UT_LOADNOTEITEMSWORKER_H

#include <QObject>
#include "gtest/gtest.h"
struct VNOTE_ALL_NOTES_MAP;
class UT_LoadNoteItemsWorker : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    explicit UT_LoadNoteItemsWorker(QObject *parent = nullptr);

signals:

public slots:
    void onNoteLoad(VNOTE_ALL_NOTES_MAP *foldesMap);
};

#endif // UT_LOADNOTEITEMSWORKER_H
