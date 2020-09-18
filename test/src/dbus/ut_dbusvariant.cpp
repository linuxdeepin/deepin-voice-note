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

#include "ut_dbusvariant.h"
#include "dbusvariant.h"

ut_dbusvariant_test::ut_dbusvariant_test()
{

}

TEST_F(ut_dbusvariant_test, testout)
{
    Inhibit inhibit;
    inhibit.what = "test";
    inhibit.who = "test1";
    inhibit.why = "test2";
    inhibit.mode = "test3";
    inhibit.uid = 1;
    inhibit.pid = 2;
    QDBusArgument argument;
    argument << inhibit;
    argument >> inhibit;
}

TEST_F(ut_dbusvariant_test, UserInfo)
{
    UserInfo userinfo;
    userinfo.pid = 1;
    userinfo.id = "test";
    userinfo.userName = "test1";
    QDBusArgument argument;
    argument << userinfo;
    argument >> userinfo;
}

TEST_F(ut_dbusvariant_test, SeatInfo)
{
    SeatInfo seatinfo;
    seatinfo.id = "test";
    seatinfo.seat = "test1";
    QDBusArgument argument;
    argument << seatinfo;
    argument >> seatinfo;
}

TEST_F(ut_dbusvariant_test, SessionInfo)
{
    SessionInfo sessioninfo;
    sessioninfo.session = "test";
    sessioninfo.pid = 1;
    sessioninfo.user = "test1";
    sessioninfo.id = "test2";
    sessioninfo.seat = "test3";
    QDBusArgument argument;
    argument << sessioninfo;
    argument >> sessioninfo;
}

