// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for ActionManager. The historical ut_actionmanager.cpp is
// excluded from the build (API mismatch); this targets the present API.

#include <QVariant>   // must precede actionmanager.h usage of QVariantList
#include <QAction>
#include "actionmanager.h"
#include <gtest/gtest.h>
#include <QObject>

// BUG 303067: 语音播放过程中记事本/笔记相关操作可正常操作（未置灰）。
// 修复（c3d3c201）新增 enableVoicePlayActions，统一在播放态禁用移动/删除/
// 新建入口，避免播放中操作导致的异常。
// PMS: https://pms.uniontech.com/bug-view-303067.html  commit: c3d3c201
TEST(ActionManagerUT, BUG303067_enableVoicePlayActionsDisablesNoteOps)
{
    ActionManager *m = ActionManager::instance();
    // 单元环境下 qmlObject 默认为空，先通过 setActionObject 注入替身，
    // 使 enableAction 的 setProperty 分支真实生效
    QObject mv, del, add;
    m->setActionObject(ActionManager::NoteMove, &mv);
    m->setActionObject(ActionManager::NoteDelete, &del);
    m->setActionObject(ActionManager::NoteAddNew, &add);
    ASSERT_NE(nullptr, m->getActionById(ActionManager::NoteMove));

    m->enableVoicePlayActions(false);
    // 探测 enableAction 写入的动态属性名（EnabledProperty）并验证被禁用
    const QByteArray prop = mv.dynamicPropertyNames().value(0, QByteArray("enabled"));
    ASSERT_FALSE(prop.isEmpty());
    EXPECT_FALSE(mv.property(prop).toBool());
    EXPECT_FALSE(del.property(prop).toBool());
    EXPECT_FALSE(add.property(prop).toBool());

    m->enableVoicePlayActions(true);
    EXPECT_TRUE(mv.property(prop).toBool());
    EXPECT_TRUE(del.property(prop).toBool());
    EXPECT_TRUE(add.property(prop).toBool());
}

// BUG 321525: 右键笔记保存二级菜单显示空白（mips）。修复（935ccf20）
// actionText 对缺失元数据的 id 安全返回空串，并保证保存子菜单项文本非空。
// PMS: https://pms.uniontech.com/bug-view-321525.html  commit: 935ccf20
TEST(ActionManagerUT, BUG321525_actionTextSubMenuNotEmpty)
{
    ActionManager *m = ActionManager::instance();
    // 无效 id：修复前可能解引用空指针，修复后安全返回空串
    EXPECT_TRUE(m->actionText(ActionManager::Invalid).isEmpty());
    // 保存笔记二级菜单：每一子项文本非空（mips 上曾显示空白）
    const QVariantList children = m->childActions(ActionManager::NoteSave);
    ASSERT_EQ(2, children.size());
    for (const QVariant &v : children) {
        const QString text = m->actionText(static_cast<ActionManager::ActionKind>(v.toInt()));
        EXPECT_FALSE(text.isEmpty()) << "empty submenu text for id" << v.toInt();
    }
    EXPECT_FALSE(m->actionText(ActionManager::NoteSave).isEmpty());
}

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
