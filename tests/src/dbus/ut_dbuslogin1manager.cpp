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
    EXPECT_TRUE(m_loginManager->blockInhibited().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, controlGroupHierarchy)
{
    EXPECT_TRUE(m_loginManager->controlGroupHierarchy().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, controllers)
{
    EXPECT_TRUE(m_loginManager->controllers().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, delayInhibited)
{
    EXPECT_TRUE(m_loginManager->delayInhibited().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, handleHibernateKey)
{
    EXPECT_TRUE(m_loginManager->handleHibernateKey().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, handleLidSwitch)
{
    EXPECT_TRUE(m_loginManager->handleLidSwitch().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, handlePowerKey)
{
    EXPECT_TRUE(m_loginManager->handlePowerKey().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, handleSuspendKey)
{
    EXPECT_TRUE(m_loginManager->handleSuspendKey().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, idleAction)
{
    EXPECT_TRUE(m_loginManager->idleAction().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, idleActionUSec)
{
    EXPECT_EQ(0, m_loginManager->idleActionUSec());
}

TEST_F(ut_dbuslogin1manager_test, idleHint)
{
    EXPECT_FALSE(m_loginManager->idleHint());
}

TEST_F(ut_dbuslogin1manager_test, idleSinceHint)
{
    EXPECT_FALSE(m_loginManager->idleSinceHint());
}

TEST_F(ut_dbuslogin1manager_test, idleSinceHintMonotonic)
{
    EXPECT_FALSE(m_loginManager->idleSinceHintMonotonic());
}

TEST_F(ut_dbuslogin1manager_test, inhibitDelayMaxUSec)
{
    EXPECT_FALSE(m_loginManager->inhibitDelayMaxUSec());
}

TEST_F(ut_dbuslogin1manager_test, killExcludeUsers)
{
    EXPECT_TRUE(m_loginManager->killExcludeUsers().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, killOnlyUsers)
{
    EXPECT_TRUE(m_loginManager->killOnlyUsers().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, killUserProcesses)
{
    EXPECT_FALSE(m_loginManager->killUserProcesses());
}

TEST_F(ut_dbuslogin1manager_test, nAutoVTs)
{
    EXPECT_FALSE(m_loginManager->nAutoVTs());
}

TEST_F(ut_dbuslogin1manager_test, preparingForShutdown)
{
    EXPECT_FALSE(m_loginManager->preparingForShutdown());
}

TEST_F(ut_dbuslogin1manager_test, preparingForSleep)
{
    EXPECT_FALSE(m_loginManager->preparingForSleep());
}

TEST_F(ut_dbuslogin1manager_test, resetControllers)
{
    EXPECT_TRUE(m_loginManager->resetControllers().isEmpty());
}

TEST_F(ut_dbuslogin1manager_test, ActivateSession)
{
    EXPECT_FALSE(m_loginManager->ActivateSession("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, ActivateSessionOnSeat)
{
    EXPECT_FALSE(m_loginManager->ActivateSessionOnSeat("", "").isValid());
}

TEST_F(ut_dbuslogin1manager_test, AttachDevice)
{
    EXPECT_FALSE(m_loginManager->AttachDevice("", "", false).isValid());
}

TEST_F(ut_dbuslogin1manager_test, CanHibernate)
{
    EXPECT_FALSE(m_loginManager->CanHibernate().isValid());
}

TEST_F(ut_dbuslogin1manager_test, CanHybridSleep)
{
    EXPECT_FALSE(m_loginManager->CanHybridSleep().isValid());
}

TEST_F(ut_dbuslogin1manager_test, CanPowerOff)
{
    EXPECT_FALSE(m_loginManager->CanPowerOff().isValid());
}

TEST_F(ut_dbuslogin1manager_test, CanReboot)
{
    EXPECT_FALSE(m_loginManager->CanReboot().isValid());
}

TEST_F(ut_dbuslogin1manager_test, CanSuspend)
{
    EXPECT_FALSE(m_loginManager->CanSuspend().isValid());
}

TEST_F(ut_dbuslogin1manager_test, FlushDevices)
{
    EXPECT_FALSE(m_loginManager->FlushDevices(false).isValid());
}
TEST_F(ut_dbuslogin1manager_test, GetSeat)
{
    EXPECT_FALSE(m_loginManager->GetSeat("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, GetSession)
{
    EXPECT_FALSE(m_loginManager->GetSession("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, GetSessionByPID)
{
    EXPECT_FALSE(m_loginManager->GetSessionByPID(0).isValid());
}

TEST_F(ut_dbuslogin1manager_test, GetUser)
{
    EXPECT_FALSE(m_loginManager->GetUser(0).isValid());
}

TEST_F(ut_dbuslogin1manager_test, Hibernate)
{
    EXPECT_FALSE(m_loginManager->Hibernate(false).isValid());
}

TEST_F(ut_dbuslogin1manager_test, HybridSleep)
{
    EXPECT_FALSE(m_loginManager->HybridSleep(false).isValid());
}

TEST_F(ut_dbuslogin1manager_test, Inhibit)
{
    EXPECT_FALSE(m_loginManager->Inhibit("", "", "", "").isValid());
}

TEST_F(ut_dbuslogin1manager_test, KillSession)
{
    EXPECT_FALSE(m_loginManager->KillSession("", "", "").isValid());
}

TEST_F(ut_dbuslogin1manager_test, KillUser)
{
    EXPECT_FALSE(m_loginManager->KillUser(0, "").isValid());
}

TEST_F(ut_dbuslogin1manager_test, ListInhibitors)
{
    EXPECT_FALSE(m_loginManager->ListInhibitors().isValid());
}

TEST_F(ut_dbuslogin1manager_test, ListSeats)
{
    EXPECT_FALSE(m_loginManager->ListSeats().isValid());
}

TEST_F(ut_dbuslogin1manager_test, ListSessions)
{
    EXPECT_FALSE(m_loginManager->ListSessions().isValid());
}

TEST_F(ut_dbuslogin1manager_test, ListUsers)
{
    EXPECT_FALSE(m_loginManager->ListUsers().isValid());
}

TEST_F(ut_dbuslogin1manager_test, LockSession)
{
    EXPECT_FALSE(m_loginManager->LockSession("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, LockSessions)
{
    EXPECT_FALSE(m_loginManager->LockSessions().isValid());
}

TEST_F(ut_dbuslogin1manager_test, PowerOff)
{
    EXPECT_FALSE(m_loginManager->PowerOff(false).isValid());
}

TEST_F(ut_dbuslogin1manager_test, Reboot)
{
    EXPECT_FALSE(m_loginManager->Reboot(false).isValid());
}

TEST_F(ut_dbuslogin1manager_test, ReleaseSession)
{
    EXPECT_FALSE(m_loginManager->ReleaseSession("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, SetUserLinger)
{
    EXPECT_FALSE(m_loginManager->SetUserLinger(0, false, false).isValid());
}

TEST_F(ut_dbuslogin1manager_test, Suspend)
{
    EXPECT_FALSE(m_loginManager->Suspend(false).isValid());
}

TEST_F(ut_dbuslogin1manager_test, TerminateSeat)
{
    EXPECT_FALSE(m_loginManager->TerminateSeat("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, TerminateSession)
{
    EXPECT_FALSE(m_loginManager->TerminateSession("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, TerminateUser)
{
    EXPECT_FALSE(m_loginManager->TerminateUser(0).isValid());
}

TEST_F(ut_dbuslogin1manager_test, UnlockSession)
{
    EXPECT_FALSE(m_loginManager->UnlockSession("").isValid());
}

TEST_F(ut_dbuslogin1manager_test, UnlockSessions)
{
    EXPECT_FALSE(m_loginManager->UnlockSessions().isValid());
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
