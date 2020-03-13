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
        NoteSaveText,
        NoteSaveVoice,
        NoteAddNew,

        //NoteDetail
        NoteDetailMenuBase,
        DetailVoiceSave = NoteDetailMenuBase,
        DetailVoice2Text,
        DetailDelete,
        DetailSelectAll,
        DetailCopy,
        DetailCut,
        DetailPaste,
    };

    Q_ENUM(ActionKind)


    DMenu* notebookContextMenu();
    DMenu* noteContextMenu();
    DMenu* detialContextMenu();
    ActionKind getActionKind(QAction *action);
    QAction* getActionById(ActionKind id);
protected:

    void initMenu();

    static ActionManager * _instance;

    QScopedPointer<DMenu>  m_notebookContextMenu;
    QScopedPointer<DMenu>  m_noteContextMenu;
    QScopedPointer<DMenu>  m_detialContextMenu;

    QMap<ActionKind, QAction*> m_actionsMap;
};

#endif // ACTIONFACTORY_H
