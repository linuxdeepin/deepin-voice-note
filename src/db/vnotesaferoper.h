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
#ifndef VNOTESAFEROPER_H
#define VNOTESAFEROPER_H

#include "common/datatypedef.h"
//录音的缓存表操作
class VNoteSaferOper
{
public:
    VNoteSaferOper();
    explicit VNoteSaferOper(const VDataSafer &safer);
    //加载表数据
    SafetyDatas *loadSafers();
    //添加记录
    void addSafer(const VDataSafer &safer);
    //删除记录
    void rmSafer(const VDataSafer &safer);

protected:
    VDataSafer m_dataSafer;
};

#endif // VNOTESAFEROPER_H
