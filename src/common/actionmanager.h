/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#ifndef ACTIONFACTORY_H
#define ACTIONFACTORY_H

#include <QObject>
#include <QMap>

#include <DMenu>

DWIDGET_USE_NAMESPACE

class ActionManager : public QObject
{
     Q_OBJECT
public:
    ActionManager();

    static ActionManager* Instance();

    enum ActionKind {
        Invalid = 0,
        //Notepad
        NotebookMenuBase,
        NotebookRename = NotebookMenuBase,
        NotebookDelete,
        NotebookAddNew,
        //Add notebook menu item begin {

        //Add notebook menu item end }
        NotebookMenuMax,

        //notes
        NoteMenuBase,
        NoteRename = NoteMenuBase,
        NoteDelete,
        NoteSaveText,
        NoteSaveVoice,
        NoteAddNew,
        //Add note menu item begin {

        //Add note menu item end }
        NoteMenuMax,

        //NoteDetail
        NoteDetailMenuBase,
        DetailVoiceSave = NoteDetailMenuBase,
        DetailVoice2Text,
        DetailDelete,
        DetailSelectAll,
        DetailCopy,
        DetailCut,
        DetailPaste,
        //Add NoteDetail menu item begin {

        //Add NoteDetail menu item end }
        NoteDetailMenuMax,

        MenuMaxId
    };

    Q_ENUM(ActionKind)


    //Menu types
    enum MenuType {
        NotebookCtxMenu,
        NoteCtxMenu,
        NoteDetailCtxMenu,
    };
    Q_ENUM(MenuType)

    DMenu* notebookContextMenu();
    DMenu* noteContextMenu();
    DMenu* detialContextMenu();
    ActionKind getActionKind(QAction *action);
    QAction* getActionById(ActionKind id);

    void enableAction(ActionKind actionId, bool enable);

    void resetCtxMenu(MenuType type, bool enable = true);
protected:

    void initMenu();

    static ActionManager * _instance;

    QScopedPointer<DMenu>  m_notebookContextMenu;
    QScopedPointer<DMenu>  m_noteContextMenu;
    QScopedPointer<DMenu>  m_detialContextMenu;

    QMap<ActionKind, QAction*> m_actionsMap;
};

#endif // ACTIONFACTORY_H
