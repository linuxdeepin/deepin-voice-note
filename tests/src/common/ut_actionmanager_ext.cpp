// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for ActionManager. The historical ut_actionmanager.cpp is
// excluded from the build (API mismatch); this targets the present API.

#include <QVariant>   // must precede actionmanager.h usage of QVariantList
#include "actionmanager.h"
#include <gtest/gtest.h>
#include <QObject>

TEST(ActionManagerUT, instanceAndMeta)
{
    ActionManager *m = ActionManager::instance();
    ASSERT_NE(nullptr, m);
    EXPECT_EQ(nullptr, m->getActionById(ActionManager::NoteRename));
    EXPECT_FALSE(m->actionText(ActionManager::NoteRename).isEmpty());
    EXPECT_EQ(ActionManager::MenuItemComponent, m->actionCompType(ActionManager::NoteRename));
    EXPECT_EQ(ActionManager::MenuSeparatorComponent, m->actionCompType(ActionManager::NoteSeparator));
    // child actions of the save-note submenu
    QVariantList children = m->childActions(ActionManager::NoteSave);
    EXPECT_EQ(2, children.size());
    EXPECT_EQ(0, m->childActions(ActionManager::NoteRename).size());
}

TEST(ActionManagerUT, enableVisibleActions)
{
    ActionManager *m = ActionManager::instance();
    m->enableAction(ActionManager::NoteRename, true);
    m->enableAction(ActionManager::NoteRename, false);
    m->enableAction(ActionManager::Invalid, true);   // non-existent -> warning path
    m->visibleAction(ActionManager::NoteRename, true);
    m->visibleAction(ActionManager::NoteRename, false);
    m->visibleAction(ActionManager::Invalid, true);  // non-existent -> warning path
    SUCCEED();
}

TEST(ActionManagerUT, resetCtxMenus)
{
    ActionManager *m = ActionManager::instance();
    for (int t = ActionManager::UnknownMenu; t <= ActionManager::SaveNoteCtxMenu; ++t)
        m->resetCtxMenu(static_cast<ActionManager::MenuType>(t), true);
    m->resetCtxMenu(ActionManager::NoteCtxMenu, false);
    SUCCEED();
}

TEST(ActionManagerUT, groupVisibilityHelpers)
{
    ActionManager *m = ActionManager::instance();
    m->visibleAiActions(true);
    m->visibleAiActions(false);
    m->visibleMulChoicesActions(true);
    m->visibleMulChoicesActions(false);
    m->enableVoicePlayActions(true);
    m->enableVoicePlayActions(false);
    SUCCEED();
}

TEST(ActionManagerUT, setActionObjectAndTrigger)
{
    ActionManager *m = ActionManager::instance();
    QObject obj;
    m->setActionObject(ActionManager::PictureView, &obj);   // first set -> passes Q_ASSERT
    m->setActionObject(ActionManager::Invalid, &obj);        // non-existent -> warning path
    m->actionTriggerFromQuick(ActionManager::NoteRename);    // emits actionTriggered
    SUCCEED();
}
