/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
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
#ifndef OLDDBVISISTORS_H
#define OLDDBVISISTORS_H

#include "db/dbvisitor.h"

//加载老记事本数据
class OldFolderQryDbVisitor : public DbVisitor
{
public:
    explicit OldFolderQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result);
    //处理结果
    virtual bool visitorData() override;
    //生成sql
    virtual bool prepareSqls() override;
};

//加载老数据库记事项数据
class OldNoteQryDbVisitor : public DbVisitor
{
public:
    explicit OldNoteQryDbVisitor(QSqlDatabase &db, const void *inParam, void *result);

    enum OldNoteType {
        Voice = 0,
        Text,
    };
    //处理结果
    virtual bool visitorData() override;
    //生成sql
    virtual bool prepareSqls() override;
};

#endif // OLDDBVISISTORS_H
