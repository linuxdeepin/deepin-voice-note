#include "actionmanager.h"

#include <DApplication>
#include <QSignalMapper>

#include <DLog>

#define MenuId "_menuid_"

ActionManager *ActionManager::_instance = nullptr;

ActionManager::ActionManager()
{
    initMenu();
}

ActionManager *ActionManager::Instance()
{
    if (nullptr == _instance) {
        _instance = new ActionManager();
    }

    return _instance;
}

DMenu* ActionManager::notebookContextMenu()
{
    return m_notebookContextMenu.get();
}

DMenu* ActionManager::noteContextMenu()
{
    return m_noteContextMenu.get();
}

DMenu* ActionManager::detialContextMenu()
{
    return m_detialContextMenu.get();
}

ActionManager::ActionKind ActionManager::getActionKind(QAction *action)
{
    return action->property(MenuId).value<ActionKind>();
}

QAction *ActionManager::getActionById(ActionManager::ActionKind id)
{
    QAction* menuAction = nullptr;

    QMap<ActionKind, QAction*>::iterator it = m_actionsMap.find(id);

    if (it != m_actionsMap.end()) {
        menuAction = *it;
    }

    return menuAction;
}

void ActionManager::initMenu()
{
    //Notebook context menu
    QStringList notebookMenuTexts;
    notebookMenuTexts << DApplication::translate("NotebookContextMenu", "Rename")
                      << DApplication::translate("NotebookContextMenu", "Delete")
                      << DApplication::translate("NotebookContextMenu", "New note");

    m_notebookContextMenu.reset(new DMenu());

    int notebookMenuIdStart = ActionKind::NotebookMenuBase;

    for (auto it : notebookMenuTexts) {
        QAction* pAction = new QAction(it, m_notebookContextMenu.get());
        pAction->setProperty(MenuId, notebookMenuIdStart);

        m_notebookContextMenu->addAction(pAction);
        m_actionsMap.insert(static_cast<ActionKind>(notebookMenuIdStart), pAction);

        notebookMenuIdStart++;
    }

    //Note context menu
    QStringList noteMenuTexts;
    noteMenuTexts << DApplication::translate("NotesContextMenu", "Rename")
                  << DApplication::translate("NotesContextMenu", "Delete")
                  << DApplication::translate("NotesContextMenu", "Save as TXT")
                  << DApplication::translate("NotesContextMenu", "Save voice recording")
                  << DApplication::translate("NotesContextMenu", "New note");

    m_noteContextMenu.reset(new DMenu());

    int noteMenuIdStart = ActionKind::NoteMenuBase;

    for (auto it : noteMenuTexts) {
        QAction* pAction = new QAction(it, m_noteContextMenu.get());
        pAction->setProperty(MenuId, noteMenuIdStart);

        m_noteContextMenu->addAction(pAction);
        m_actionsMap.insert(static_cast<ActionKind>(noteMenuIdStart), pAction);

        noteMenuIdStart++;
    }

    //Voice context menu
    QStringList noteDetailMenuTexts;
    noteDetailMenuTexts << DApplication::translate("NoteDetailContextMenu", "Save as MP3")
                        << DApplication::translate("NoteDetailContextMenu", "Voice to Text")
                        << DApplication::translate("NoteDetailContextMenu", "Delete")
                        << DApplication::translate("NoteDetailContextMenu", "Select all")
                        << DApplication::translate("NoteDetailContextMenu", "Copy")
                        << DApplication::translate("NoteDetailContextMenu", "Cut")
                        << DApplication::translate("NoteDetailContextMenu", "Paste");

    m_detialContextMenu.reset(new DMenu());

    int detailMenuIdStart = ActionKind::NoteDetailMenuBase;

    for (auto it : noteDetailMenuTexts) {
        QAction* pAction = new QAction(it, m_detialContextMenu.get());
        pAction->setProperty(MenuId, detailMenuIdStart);

        m_detialContextMenu->addAction(pAction);
        m_actionsMap.insert(static_cast<ActionKind>(detailMenuIdStart), pAction);

        detailMenuIdStart++;
    }
}
