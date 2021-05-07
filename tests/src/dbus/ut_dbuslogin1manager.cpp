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

#include "ut_dbuslogin1manager.h"
#include "dbuslogin1manager.h"

ut_dbuslogin1manager_test::ut_dbuslogin1manager_test()
{
}

void ut_dbuslogin1manager_test::SetUp()
{
    m_loginManager = new DBusLogin1Manager(
        "org.freedesktop.logintest",
        "/org/freedesktop/logintest",
        QDBusConnection::systemBus(), this);
}

void ut_dbuslogin1manager_test::TearDown()
{
    delete m_loginManager;
    m_loginManager = nullptr;
}

TEST_F(ut_dbuslogin1manager_test, staticInterfaceName)
{
    m_loginManager->staticInterfaceName();
}

TEST_F(ut_dbuslogin1manager_test, blockInhibited)
{
    m_loginManager->blockInhibited();
}

TEST_F(ut_dbuslogin1manager_test, controlGroupHierarchy)
{
    m_loginManager->controlGroupHierarchy();
}

TEST_F(ut_dbuslogin1manager_test, controllers)
{
    m_loginManager->controllers();
}

TEST_F(ut_dbuslogin1manager_test, delayInhibited)
{
    m_loginManager->delayInhibited();
}

TEST_F(ut_dbuslogin1manager_test, handleHibernateKey)
{
    m_loginManager->handleHibernateKey();
}

TEST_F(ut_dbuslogin1manager_test, handleLidSwitch)
{
    m_loginManager->handleLidSwitch();
}

TEST_F(ut_dbuslogin1manager_test, handlePowerKey)
{
    m_loginManager->handlePowerKey();
}

TEST_F(ut_dbuslogin1manager_test, handleSuspendKey)
{
    m_loginManager->handleSuspendKey();
}

TEST_F(ut_dbuslogin1manager_test, idleAction)
{
    m_loginManager->idleAction();
}

TEST_F(ut_dbuslogin1manager_test, idleActionUSec)
{
    m_loginManager->idleActionUSec();
}

TEST_F(ut_dbuslogin1manager_test, idleHint)
{
    m_loginManager->idleHint();
}

TEST_F(ut_dbuslogin1manager_test, idleSinceHint)
{
    m_loginManager->idleSinceHint();
}

TEST_F(ut_dbuslogin1manager_test, idleSinceHintMonotonic)
{
    m_loginManager->idleSinceHintMonotonic();
}

TEST_F(ut_dbuslogin1manager_test, inhibitDelayMaxUSec)
{
    m_loginManager->inhibitDelayMaxUSec();
}

TEST_F(ut_dbuslogin1manager_test, killExcludeUsers)
{
    m_loginManager->killExcludeUsers();
}

TEST_F(ut_dbuslogin1manager_test, killOnlyUsers)
{
    m_loginManager->killOnlyUsers();
}

TEST_F(ut_dbuslogin1manager_test, killUserProcesses)
{
    m_loginManager->killUserProcesses();
}

TEST_F(ut_dbuslogin1manager_test, nAutoVTs)
{
    m_loginManager->nAutoVTs();
}

TEST_F(ut_dbuslogin1manager_test, preparingForShutdown)
{
    m_loginManager->preparingForShutdown();
}

TEST_F(ut_dbuslogin1manager_test, preparingForSleep)
{
    m_loginManager->preparingForSleep();
}

TEST_F(ut_dbuslogin1manager_test, resetControllers)
{
    m_loginManager->resetControllers();
}

TEST_F(ut_dbuslogin1manager_test, ActivateSession)
{
    m_loginManager->ActivateSession("");
}

TEST_F(ut_dbuslogin1manager_test, ActivateSessionOnSeat)
{
    m_loginManager->ActivateSessionOnSeat("", "");
}

TEST_F(ut_dbuslogin1manager_test, AttachDevice)
{
    m_loginManager->AttachDevice("", "", false);
}

TEST_F(ut_dbuslogin1manager_test, CanHibernate)
{
    m_loginManager->CanHibernate();
}

TEST_F(ut_dbuslogin1manager_test, CanHybridSleep)
{
    m_loginManager->CanHybridSleep();
}

TEST_F(ut_dbuslogin1manager_test, CanPowerOff)
{
    m_loginManager->CanPowerOff();
}

TEST_F(ut_dbuslogin1manager_test, CanReboot)
{
    m_loginManager->CanReboot();
}

TEST_F(ut_dbuslogin1manager_test, CanSuspend)
{
    m_loginManager->CanSuspend();
}

TEST_F(ut_dbuslogin1manager_test, FlushDevices)
{
    m_loginManager->FlushDevices(false);
}
TEST_F(ut_dbuslogin1manager_test, GetSeat)
{
    m_loginManager->GetSeat("");
}

TEST_F(ut_dbuslogin1manager_test, GetSession)
{
    m_loginManager->GetSession("");
}

TEST_F(ut_dbuslogin1manager_test, GetSessionByPID)
{
    m_loginManager->GetSessionByPID(0);
}

TEST_F(ut_dbuslogin1manager_test, GetUser)
{
    m_loginManager->GetUser(0);
}

TEST_F(ut_dbuslogin1manager_test, Hibernate)
{
    m_loginManager->Hibernate(false);
}

TEST_F(ut_dbuslogin1manager_test, HybridSleep)
{
    m_loginManager->HybridSleep(false);
}

TEST_F(ut_dbuslogin1manager_test, Inhibit)
{
    m_loginManager->Inhibit("", "", "", "");
}

TEST_F(ut_dbuslogin1manager_test, KillSession)
{
    m_loginManager->KillSession("", "", "");
}

TEST_F(ut_dbuslogin1manager_test, KillUser)
{
    m_loginManager->KillUser(0, "");
}

TEST_F(ut_dbuslogin1manager_test, ListInhibitors)
{
    m_loginManager->ListInhibitors();
}

TEST_F(ut_dbuslogin1manager_test, ListSeats)
{
    m_loginManager->ListSeats();
}

TEST_F(ut_dbuslogin1manager_test, ListSessions)
{
    m_loginManager->ListSessions();
}

TEST_F(ut_dbuslogin1manager_test, ListUsers)
{
    m_loginManager->ListUsers();
}

TEST_F(ut_dbuslogin1manager_test, LockSession)
{
    m_loginManager->LockSession("");
}

TEST_F(ut_dbuslogin1manager_test, LockSessions)
{
    m_loginManager->LockSessions();
}

TEST_F(ut_dbuslogin1manager_test, PowerOff)
{
    m_loginManager->PowerOff(false);
}

TEST_F(ut_dbuslogin1manager_test, Reboot)
{
    m_loginManager->Reboot(false);
}

TEST_F(ut_dbuslogin1manager_test, ReleaseSession)
{
    m_loginManager->ReleaseSession("");
}

TEST_F(ut_dbuslogin1manager_test, SetUserLinger)
{
    m_loginManager->SetUserLinger(0, false, false);
}

TEST_F(ut_dbuslogin1manager_test, Suspend)
{
    m_loginManager->Suspend(false);
}

TEST_F(ut_dbuslogin1manager_test, TerminateSeat)
{
    m_loginManager->TerminateSeat("");
}

TEST_F(ut_dbuslogin1manager_test, TerminateSession)
{
    m_loginManager->TerminateSession("");
}

TEST_F(ut_dbuslogin1manager_test, TerminateUser)
{
    m_loginManager->TerminateUser(0);
}

TEST_F(ut_dbuslogin1manager_test, UnlockSession)
{
    m_loginManager->UnlockSession("");
}

TEST_F(ut_dbuslogin1manager_test, UnlockSessions)
{
    m_loginManager->UnlockSessions();
}

TEST_F(ut_dbuslogin1manager_test, __propertyChanged__)
{
    QDBusMessage message;
    QList<QVariant> param;

    param << "org.freedesktop.login1.Manager"
          << "test"
          << "PrepareForShutdown";

    message.setArguments(param);

    m_loginManager->__propertyChanged__(message);
}
