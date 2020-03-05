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
    menuTexts << DApplication::translate("NotepadRMenu", "Rename")
              << DApplication::translate("NotepadRMenu", "Delete")
              << DApplication::translate("NotepadRMenu", "New note");
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
    menuTexts << DApplication::translate("NotesRMenu", "Rename")
              << DApplication::translate("NotesRMenu", "Delete")
              << DApplication::translate("NotesRMenu", "Save as TXT")
              << DApplication::translate("NotesRMenu", "Save voice recording")
              << DApplication::translate("NotesRMenu", "New note");
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
    menuTexts << DApplication::translate("VoiceRMenu", "Delete")
              << DApplication::translate("VoiceRMenu", "Save as MP3")
              << DApplication::translate("VoiceRMenu", "Voice to Text");
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
