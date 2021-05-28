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
#include "actionmanager.h"
#include "common/opsstateinterface.h"

#include <DApplication>
#include <DLog>

#include <QSignalMapper>

#define MenuId "_menuid_"

ActionManager *ActionManager::_instance = nullptr;

/**
 * @brief ActionManager::ActionManager
 */
ActionManager::ActionManager()
{
    initMenu();
}

/**
 * @brief ActionManager::Instance
 * @return 单例对象
 */
ActionManager *ActionManager::Instance()
{
    if (nullptr == _instance) {
        _instance = new ActionManager();
    }

    return _instance;
}

/**
 * @brief ActionManager::notebookContextMenu
 * @return 记事本右键菜单
 */
VNoteRightMenu *ActionManager::notebookContextMenu()
{
    return m_notebookContextMenu.get();
}

/**
 * @brief ActionManager::noteContextMenu
 * @return 记事项右键菜单
 */
VNoteRightMenu *ActionManager::noteContextMenu()
{
    return m_noteContextMenu.get();
}

/**
 * @brief ActionManager::detialContextMenu
 * @return 详情页右键菜单
 */
DMenu *ActionManager::detialContextMenu()
{
    return m_detialContextMenu.get();
}

/**
 * @brief ActionManager::getActionKind
 * @param action 菜单项
 * @return id
 */
ActionManager::ActionKind ActionManager::getActionKind(QAction *action)
{
    return action->property(MenuId).value<ActionKind>();
}

/**
 * @brief ActionManager::getActionById
 * @param id
 * @return 菜单项
 */
QAction *ActionManager::getActionById(ActionManager::ActionKind id)
{
    QAction *menuAction = nullptr;

    QMap<ActionKind, QAction *>::iterator it = m_actionsMap.find(id);

    if (it != m_actionsMap.end()) {
        menuAction = *it;
    }

    return menuAction;
}

/**
 * @brief ActionManager::enableAction
 * @param actionId
 * @param enable true 可用
 */
void ActionManager::enableAction(ActionManager::ActionKind actionId, bool enable)
{
    QMap<ActionKind, QAction *>::iterator it = m_actionsMap.find(actionId);

    if (it != m_actionsMap.end()) {
        (*it)->setEnabled(enable);
    }
}

/**
 * @brief ActionManager::visibleAction
 * @param actionId
 * @param enable true显示
 */
void ActionManager::visibleAction(ActionManager::ActionKind actionId, bool enable)
{
    QMap<ActionKind, QAction *>::iterator it = m_actionsMap.find(actionId);

    if (it != m_actionsMap.end()) {
        (*it)->setVisible(enable);
    }
}

/**
 * @brief ActionManager::resetCtxMenu
 * @param type 右键菜单类型
 * @param enable true可用
 */
void ActionManager::resetCtxMenu(ActionManager::MenuType type, bool enable)
{
    int startMenuId = MenuMaxId;
    int endMenuId = MenuMaxId;

    if (MenuType::NotebookCtxMenu == type) {
        startMenuId = NotebookMenuBase;
        endMenuId = NotebookMenuMax;
    } else if (MenuType::NoteCtxMenu == type) {
        startMenuId = NoteMenuBase;
        endMenuId = NoteMenuMax;
    } else if (MenuType::NoteDetailCtxMenu == type) {
        startMenuId = NoteDetailMenuBase;
        endMenuId = NoteDetailMenuMax;
    }

    QAction *pAction = nullptr;

    for (; startMenuId < endMenuId; startMenuId++) {
        pAction = m_actionsMap[static_cast<ActionKind>(startMenuId)];

        if (nullptr != pAction) {
            pAction->setEnabled(enable);
        }
    }
}
/**
 * @brief ActionManager::initMenu
 */
void ActionManager::initMenu()
{
    bool isAISrvAvailable = OpsStateInterface::instance()->isAiSrvExist();
    //Notebook context menu
    QStringList notebookMenuTexts;
    notebookMenuTexts << DApplication::translate("NotebookContextMenu", "Rename")
                      << DApplication::translate("NotebookContextMenu", "Delete")
                      << DApplication::translate("NotebookContextMenu", "New note");
    //初始化记事本右键菜单
    m_notebookContextMenu.reset(new VNoteRightMenu());

    int notebookMenuIdStart = ActionKind::NotebookMenuBase;

    for (auto it : notebookMenuTexts) {
        QAction *pAction = new QAction(it, m_notebookContextMenu.get());
        pAction->setProperty(MenuId, notebookMenuIdStart);

        m_notebookContextMenu->addAction(pAction);
        m_actionsMap.insert(static_cast<ActionKind>(notebookMenuIdStart), pAction);

        notebookMenuIdStart++;
    }

    //Note context menu
    QStringList noteMenuTexts;
    noteMenuTexts << DApplication::translate("NotesContextMenu", "Rename")
                  << DApplication::translate("NotesContextMenu", "")
                  << DApplication::translate("NotesContextMenu", "Move")
                  << DApplication::translate("NotesContextMenu", "Delete")
                  << DApplication::translate("NotesContextMenu", "Save as TXT")
                  << DApplication::translate("NotesContextMenu", "Save voice recording")
                  << DApplication::translate("NotesContextMenu", "New note");

    //初始化笔记右键菜单
    m_noteContextMenu.reset(new VNoteRightMenu());

    int noteMenuIdStart = ActionKind::NoteMenuBase;

    for (auto it : noteMenuTexts) {
        QAction *pAction = new QAction(it, m_noteContextMenu.get());
        pAction->setProperty(MenuId, noteMenuIdStart);

        m_noteContextMenu->addAction(pAction);
        m_actionsMap.insert(static_cast<ActionKind>(noteMenuIdStart), pAction);

        noteMenuIdStart++;

        if (noteMenuIdStart == NoteAddNew) {
            m_noteContextMenu->addSeparator();
        }
    }

    //Voice context menu
    QStringList noteDetailMenuTexts;
    noteDetailMenuTexts << DApplication::translate("NoteDetailContextMenu", "Save as MP3")
                        << DApplication::translate("NoteDetailContextMenu", "Voice to Text")
                        << DApplication::translate("NoteDetailContextMenu", "Delete")
                        << DApplication::translate("NoteDetailContextMenu", "Select all")
                        << DApplication::translate("NoteDetailContextMenu", "Copy")
                        << DApplication::translate("NoteDetailContextMenu", "Cut")
                        << DApplication::translate("NoteDetailContextMenu", "Paste")
                        << DApplication::translate("NoteDetailContextMenu", "Text to Speech")
                        << DApplication::translate("NoteDetailContextMenu", "Stop reading")
                        << DApplication::translate("NoteDetailContextMenu", "Speech to Text")
                        << DApplication::translate("NoteDetailContextMenu", "Translate");

    //初始化详情页右键菜单
    m_detialContextMenu.reset(new VNoteRightMenu());

    int detailMenuIdStart = ActionKind::NoteDetailMenuBase;

    for (auto it : noteDetailMenuTexts) {
        QAction *pAction = new QAction(it, m_detialContextMenu.get());
        pAction->setProperty(MenuId, detailMenuIdStart);

        m_detialContextMenu->addAction(pAction);
        m_actionsMap.insert(static_cast<ActionKind>(detailMenuIdStart), pAction);
        if (!isAISrvAvailable) {
            if (detailMenuIdStart == DetailVoice2Text
                || detailMenuIdStart > DetailPaste) {
                pAction->setVisible(false);
            }
        } else if (detailMenuIdStart == DetailPaste) {
            m_detialContextMenu->addSeparator();
        }
        detailMenuIdStart++;
    }
}
