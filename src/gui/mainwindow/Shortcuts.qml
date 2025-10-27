import QtQuick 2.15
import org.deepin.dtk 1.0 as D

Item {
    id: item


    property bool blockCreateKeys: false
    // 初始页面仅允许 Ctrl+N / F1，其它快捷键全部禁用
    property bool initialOnlyCreateFolder: false
    // 录音过程中禁用录音快捷键，避免重复启动录音导致崩溃
    property bool blockRecordingKey: false

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
    signal showJsContextMenu

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
        enabled: !item.initialOnlyCreateFolder

        //快捷键界面
        sequence: "Ctrl+Shift+/"

        onActivated: {
            showShortCutView();
        }
    }

    Shortcut {
        id: ctrl_S
        enabled: !item.initialOnlyCreateFolder

        sequence: "Ctrl+S"

        onActivated: {
            saveNote();
        }
    }

    Shortcut {
        id: ctrl_D
        enabled: !item.initialOnlyCreateFolder

        sequence: "Ctrl+D"

        onActivated: {
            saveVoice();
        }
    }

    Shortcut {
        id: ctrl_N

        sequence: "Ctrl+N"

        enabled: !item.blockCreateKeys

        onActivated: {
            createFolder();
        }
    }

    Shortcut {
        id: rename
        enabled: !item.initialOnlyCreateFolder

        sequence: "F2"

        onActivated: {
            renameFolder();
        }
    }

    Shortcut {
        id: renameNoteShort
        enabled: !item.initialOnlyCreateFolder

        sequence: "F3"

        onActivated: {
            renameNote();
        }
    }

    Shortcut {
        id: ctrl_R
        enabled: !item.initialOnlyCreateFolder && !item.blockRecordingKey

        sequence: "Ctrl+R"

        onActivated: {
            startRecording();
        }
    }

    Shortcut {
        id: playPause
        enabled: !item.initialOnlyCreateFolder

        sequence: "Space"

        onActivated: {
            playPauseVoice();
        }
    }

    Shortcut {
        id: ctrl_B

        sequence: "Ctrl+B"

        enabled: !item.blockCreateKeys && !item.initialOnlyCreateFolder

        onActivated: {
            createNote();
        }
    }

    Shortcut {
        id: ctrl_A

        sequence: "Ctrl+A"

        enabled: !item.initialOnlyCreateFolder
        onActivated: {}
    }

    Shortcut {
        id: ctrl_C

        sequence: "Ctrl+C"

        enabled: !item.initialOnlyCreateFolder
        onActivated: {
            copy();
        }
    }

    Shortcut {
        id: ctrl_X

        sequence: "Ctrl+X"

        enabled: !item.initialOnlyCreateFolder
        onActivated: {}
    }

    Shortcut {
        id: ctrl_V

        sequence: "Ctrl+V"

        enabled: !item.initialOnlyCreateFolder
        onActivated: {}
    }

    Shortcut {
        id: alt_M

        sequence: "Alt+M"

        enabled: !item.initialOnlyCreateFolder
        onActivated: {
            showJsContextMenu();
        }
    }

    Shortcut {
        id: ctrl_Z

        sequence: "Ctrl+Z"

        enabled: !item.initialOnlyCreateFolder
        onActivated: {}
    }

    Shortcut {
        id: ctrl_Shift_Z

        sequence: "Ctrl+Shift+Z"

        enabled: !item.initialOnlyCreateFolder
        onActivated: {}
    }
}
