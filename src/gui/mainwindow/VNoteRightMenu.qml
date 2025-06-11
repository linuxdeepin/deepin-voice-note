// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import VNote 1.0
import org.deepin.dtk 1.0

// 动态创建不同类型的右键菜单
Menu {
    id: rightMenu

    // 菜单类型 ActionManager::MenuType
    property int menuType: ActionManager.UnknownMenu

    // 动作项触发信号
    signal actionTrigger(int actionId)

    // 创建子菜单
    function addCustomMenu(actionId, parent) {
        if (Component.Ready !== menuCreator.status) {
            return;
        }
        var actionText = ActionManager.actionText(actionId);
        var menu = menuCreator.createObject(parent, {
            menuId: actionId,
            title: actionText
        });
        parent.addMenu(menu);
        ActionManager.setActionObject(actionId, menu);

        // 创建子菜单项
        var childActions = ActionManager.childActions(actionId);
        for (var i = 0; i < childActions.length; ++i) {
            addCustonItem(childActions[i], menu);
        }
    }

    // 创建子菜单项
    function addCustomMenuItem(actionId, parent) {
        var actionText = ActionManager.actionText(actionId);
        var menuItem = menuItemCreator.createObject(parent, {
            menuId: actionId,
            text: actionText
        });
        parent.addItem(menuItem);
        ActionManager.setActionObject(actionId, menuItem);
    }

    // 创建菜单分割线
    function addCustomMenuSeparator(actionId, parent) {
        if (Component.Ready !== menuSeparatorCreator.status) {
            return;
        }
        var menuSeparator = menuSeparatorCreator.createObject(parent, {
            menuId: actionId
        });
        parent.addItem(menuSeparator);
        ActionManager.setActionObject(actionId, menuSeparator);
    }

    // 创建 actionId 对应的菜单项，根据不同组件类型构建
    function addCustonItem(actionId, parent) {
        var type = ActionManager.actionCompType(actionId);
        switch (type) {
        case ActionManager.MenuComponent:
            addCustomMenu(actionId, parent);
            break;
        case ActionManager.MenuItemComponent:
            addCustomMenuItem(actionId, parent);
            break;
        case ActionManager.MenuSeparatorComponent:
            addCustomMenuSeparator(actionId, parent);
            break;
        default:
            break;
        }
    }

    // 递归移除菜单 menu 的子菜单项
    function clearMenuItems(menu) {
        while (menu.count > 0) {
            var item = menu.itemAt(0);
            // item instanceof Menu 效果不佳
            if (null !== item.subMenu && undefined !== item.subMenu) {
                clearMenuItems(item.subMenu);
                menu.removeMenu(item.subMenu);
            }
            menu.removeItem(item);
        }
    }

    // 创建菜单，遍历在  [begien - end) 间的菜单项
    function createMenu(begin, end, parent) {
        if (Component.Ready !== menuItemCreator.status) {
            return;
        }

        // 遍历追加菜单项
        for (var i = begin; i < end; ++i) {
            addCustonItem(i, parent);
        }
    }

    onMenuTypeChanged: {
        // 清空旧菜单项
        clearMenuItems(rightMenu);

        // 根据不同菜单类型创建不同范围的菜单项
        switch (menuType) {
        case ActionManager.NotebookCtxMenu:
            createMenu(ActionManager.NotebookMenuBase, ActionManager.NotebookMenuMax, rightMenu);
            break;
        case ActionManager.NoteCtxMenu:
            createMenu(ActionManager.NoteMenuBase, ActionManager.NoteMenuMax, rightMenu);
            break;
        case ActionManager.VoiceCtxMenu:
            createMenu(ActionManager.VoiceMenuBase, ActionManager.VoiceMenuMax, rightMenu);
            break;
        case ActionManager.PictureCtxMenu:
            createMenu(ActionManager.PictureMenuBase, ActionManager.PictureMenuMax, rightMenu);
            break;
        case ActionManager.TxtCtxMenu:
            createMenu(ActionManager.TxtMenuBase, ActionManager.TxtMenuMax, rightMenu);
            break;
        case ActionManager.SaveNoteCtxMenu:
            createMenu(ActionManager.SaveNoteMenuBase, ActionManager.SaveNoteMax, rightMenu);
            break;
        default:
            break;
        }
    }

    // 菜单构造器
    Component {
        id: menuCreator

        Menu {
            property int menuId: 0

            height: visible ? implicitHeight : 0
        }
    }

    Component {
        id: menuItemCreator

        MenuItem {
            property int menuId: 0

            height: visible ? implicitHeight : 0

            onTriggered: {
                rightMenu.actionTrigger(menuId);
                ActionManager.actionTriggerFromQuick(menuId);
            }
        }
    }

    Component {
        id: menuSeparatorCreator

        MenuSeparator {
            property int menuId: 0

            height: visible ? implicitHeight : 0
        }
    }
}
