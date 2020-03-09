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

DMenu* ActionManager::voiceContextMenu()
{
    return m_voiceContextMenu.get();
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
    QStringList voiceMenuTexts;
    voiceMenuTexts << DApplication::translate("VoiceContextMenu", "Delete")
                   << DApplication::translate("VoiceContextMenu", "Save as MP3")
                   << DApplication::translate("VoiceContextMenu", "Voice to Text");

    m_voiceContextMenu.reset(new DMenu());

    int voiceMenuIdStart = ActionKind::VoiceMenuBase;

    for (auto it : voiceMenuTexts) {
        QAction* pAction = new QAction(it, m_voiceContextMenu.get());
        pAction->setProperty(MenuId, voiceMenuIdStart);

        m_voiceContextMenu->addAction(pAction);
        m_actionsMap.insert(static_cast<ActionKind>(voiceMenuIdStart), pAction);

        voiceMenuIdStart++;
    }
}
