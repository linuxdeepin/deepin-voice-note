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

#include "ut_actionmanager.h"
#include "actionmanager.h"
#include "opsstateinterface.h"

ut_actionmanager_test::ut_actionmanager_test()
{
}

TEST_F(ut_actionmanager_test, notebookContextMenu)
{
    ActionManager actionmanager;
    actionmanager.notebookContextMenu();
    EXPECT_FALSE(actionmanager.m_notebookContextMenu->isEmpty());
}

TEST_F(ut_actionmanager_test, noteContextMenu)
{
    ActionManager actionmanager;
    actionmanager.noteContextMenu();
    EXPECT_FALSE(actionmanager.m_noteContextMenu->isEmpty());
}

TEST_F(ut_actionmanager_test, getActionKind)
{
    ActionManager actionmanager;
    QAction *tmpact = actionmanager.getActionById(actionmanager.NotebookAddNew);
    EXPECT_EQ(actionmanager.getActionKind(tmpact), 3);
}

TEST_F(ut_actionmanager_test, getActionById)
{
    ActionManager actionmanager;
    QAction *tmpact = actionmanager.getActionById(actionmanager.NotebookRename);
    EXPECT_EQ(tmpact->text(), "Rename");
}

TEST_F(ut_actionmanager_test, enableAction)
{
    ActionManager actionmanager;
    QAction *tmpact = actionmanager.getActionById(actionmanager.NotebookRename);
    actionmanager.enableAction(actionmanager.NotebookRename, false);
    EXPECT_FALSE(tmpact->isEnabled());
}

TEST_F(ut_actionmanager_test, visibleAction)
{
    ActionManager actionmanager;
    QAction *tmpact = actionmanager.getActionById(actionmanager.NotebookRename);
    actionmanager.visibleAction(actionmanager.NotebookRename, false);
    EXPECT_FALSE(tmpact->isVisible());
}

TEST_F(ut_actionmanager_test, resetCtxMenu)
{
    ActionManager actionmanager;
    QAction *tmpact = actionmanager.getActionById(actionmanager.NotebookRename);
    actionmanager.resetCtxMenu(actionmanager.NotebookCtxMenu, false);
    EXPECT_FALSE(tmpact->isEnabled());

    tmpact = actionmanager.getActionById(actionmanager.NoteRename);
    actionmanager.resetCtxMenu(actionmanager.NoteCtxMenu, false);
    EXPECT_FALSE(tmpact->isEnabled());
}

TEST_F(ut_actionmanager_test, initMenu)
{
    OpsStateInterface::instance()->operState(OpsStateInterface::instance()->StateAISrvAvailable, false);
    ActionManager actionmanager;
}

TEST_F(ut_actionmanager_test, Instance)
{
    EXPECT_TRUE(ActionManager::Instance());
}
