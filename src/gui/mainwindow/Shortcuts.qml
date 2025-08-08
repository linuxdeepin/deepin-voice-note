import QtQuick 2.15
import org.deepin.dtk 1.0 as D

Item {
    id: item

    signal copy
    signal createFolder
    signal createNote
    signal playPauseVoice
    signal renameFolder
    signal renameNote
    signal saveNote
    signal saveVoice
    signal showShortCutView
    signal startRecording

    Shortcut {
        id: ctrl_F1

        //帮助手册
        autoRepeat: false
        enabled: true
        sequence: "F1"

        onActivated: {
            D.ApplicationHelper.handleHelpAction();
        }
    }

    Shortcut {
        id: ctrl_Shift_H

        //快捷键界面
        sequence: "Ctrl+Shift+/"

        onActivated: {
            showShortCutView();
        }
    }

    Shortcut {
        id: ctrl_S

        sequence: "Ctrl+S"

        onActivated: {
            saveNote();
        }
    }

    Shortcut {
        id: ctrl_D

        sequence: "Ctrl+D"

        onActivated: {
            saveVoice();
        }
    }

    Shortcut {
        id: ctrl_N

        sequence: "Ctrl+N"

        onActivated: {
            createFolder();
        }
    }

    Shortcut {
        id: rename

        sequence: "F2"

        onActivated: {
            renameFolder();
        }
    }

    Shortcut {
        id: renameNoteShort

        sequence: "F3"

        onActivated: {
            renameNote();
        }
    }

    Shortcut {
        id: ctrl_R

        sequence: "Ctrl+R"

        onActivated: {
            startRecording();
        }
    }

    Shortcut {
        id: playPause

        sequence: "Space"

        onActivated: {
            playPauseVoice();
        }
    }

    Shortcut {
        id: ctrl_B

        sequence: "Ctrl+B"

        onActivated: {
            createNote();
        }
    }

    Shortcut {
        id: ctrl_A

        sequence: "Ctrl+A"

        onActivated: {}
    }

    Shortcut {
        id: ctrl_C

        sequence: "Ctrl+C"

        onActivated: {
            copy();
        }
    }

    Shortcut {
        id: ctrl_X

        sequence: "Ctrl+X"

        onActivated: {}
    }

    Shortcut {
        id: ctrl_V

        sequence: "Ctrl+V"

        onActivated: {}
    }

    Shortcut {
        id: ctrl_Z

        sequence: "Ctrl+Z"

        onActivated: {}
    }

    Shortcut {
        id: ctrl_Shift_Z

        sequence: "Ctrl+Shift+Z"

        onActivated: {}
    }
}
