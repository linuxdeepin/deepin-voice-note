#ifndef ACTIONFACTORY_H
#define ACTIONFACTORY_H

#include <QObject>
#include <DMenu>
DWIDGET_USE_NAMESPACE

class ActionManager : public QObject
{
     Q_OBJECT
public:
    ActionManager();
    enum ActionKind{
        Invalid = 0,
        //Notepad
        NotepadRename,
        NotepadDelete,
        NotepadAddNew,

        //notes
        NoteRename,
        NoteDelete,
        NoteSaveVoice,
        NoteSaveText,

        //Voice
        VoiceDelete,
        VoiceSave,
        VoiceConversion
    };
    Q_ENUM(ActionKind)
    static DMenu* getNotepadRMenu(QWidget *parent);
    static DMenu* getNotesRMenu(QWidget *parent);
    static DMenu* getVoiceRMenu(QWidget *parent);
    static ActionKind getActionKind(QAction *action);
};

#endif // ACTIONFACTORY_H
