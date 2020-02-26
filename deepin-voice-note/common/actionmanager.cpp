#include "actionmanager.h"

#include <DApplication>
#include <QSignalMapper>

#define KindName "kind"

ActionManager::ActionManager()
{

}

DMenu* ActionManager::getNotepadRMenu(QWidget *parent)
{
    DMenu *menu = new DMenu(parent);
    QStringList menuTexts;
    menuTexts << DApplication::translate("NotepadRMenu", "rename")
              << DApplication::translate("NotepadRMenu", "delete")
              << DApplication::translate("NotepadRMenu", "newNote");
    for(int i = 0; i < menuTexts.size(); i++)
    {
        QAction* pAction = new QAction(menuTexts[i], menu);
        pAction->setProperty(KindName, NotepadRename + i);
        menu->addAction(pAction);
    }
    return menu;
}

DMenu* ActionManager::getNotesRMenu(QWidget *parent)
{
    DMenu *menu = new DMenu(parent);
    QStringList menuTexts;
    menuTexts << DApplication::translate("NotesRMenu", "rename")
              << DApplication::translate("NotesRMenu", "delete")
              << DApplication::translate("NotesRMenu", "save as TXT")
              << DApplication::translate("NotesRMenu", "save attachment")
              << DApplication::translate("NotesRMenu", "new Note");
    for(int i = 0; i < menuTexts.size(); i++)
    {
        QAction* pAction = new QAction(menuTexts[i], menu);
        menu->addAction(pAction);
        pAction->setProperty(KindName, NoteRename + i);
    }
    return menu;
}

DMenu* ActionManager::getVoiceRMenu(QWidget *parent)
{
    DMenu *menu = new DMenu(parent);
    QStringList menuTexts;
    menuTexts << DApplication::translate("VoiceRMenu", "delete")
              << DApplication::translate("VoiceRMenu", "save as mp3")
              << DApplication::translate("VoiceRMenu", "voice turn words");
    for(int i = 0; i < menuTexts.size(); i++)
    {
        QAction* pAction = new QAction(menuTexts[i], menu);
        pAction->setProperty(KindName, VoiceDelete + i);
        menu->addAction(pAction);
    }
    return menu;
}

ActionManager::ActionKind ActionManager::getActionKind(QAction *action)
{
     return action ->property(KindName).value<ActionKind>();
}
