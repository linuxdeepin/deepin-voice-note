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

        //notes
        NoteMenuBase,
        NoteRename = NoteMenuBase,
        NoteDelete,
        NoteSaveVoice,
        NoteSaveText,
        NoteAddNew,

        //Voice
        VoiceMenuBase,
        VoiceDelete = VoiceMenuBase,
        VoiceSave,
        VoiceConversion,
    };

    Q_ENUM(ActionKind)


    DMenu* notebookContextMenu();
    DMenu* noteContextMenu();
    DMenu* voiceContextMenu();
    ActionKind getActionKind(QAction *action);
    QAction* getActionById(ActionKind id);
protected:

    void initMenu();

    static ActionManager * _instance;

    QScopedPointer<DMenu>  m_notebookContextMenu;
    QScopedPointer<DMenu>  m_noteContextMenu;
    QScopedPointer<DMenu>  m_voiceContextMenu;

    QMap<ActionKind, QAction*> m_actionsMap;
};

#endif // ACTIONFACTORY_H
