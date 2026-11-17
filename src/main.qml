// Copyright (C) 2020 ~ 2026 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import VNote 1.0
import "./gui/mainwindow"
import "./gui/dialog"
import org.deepin.dtk 1.0

ApplicationWindow {
    id: rootWindow

    property bool workspaceActive: workspaceLoader.active
    property bool createFirstNotebook: false
    property bool isRecording: workspaceLoader.item ? workspaceLoader.item.isRecording : false

    DWindow.enabled: true
    // StyledBehindWindowBlur 只有在窗口启用 DTK 背景模糊并且窗口本身
    // 允许透明合成时才会真正采集窗口背后的内容；否则只会显示兜底纯色。
    DWindow.enableBlurWindow: true
    color: "transparent"
    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 681
    minimumHeight: 300
    minimumWidth: 690
    visible: false
    width: 1096

    function activateWorkspace(createFirstNotebook) {
        if (workspaceLoader.active)
            return;

        rootWindow.createFirstNotebook = createFirstNotebook;
        initialInterface.visible = false;
        VNoteMainManager.prepareWorkspace();
        workspaceLoader.active = true;
        workspaceLoader.source = "gui/mainwindow/MainWindow.qml";
    }

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2;
        y = Screen.height / 2 - height / 2;
        if (hasExistingFolders)
            activateWorkspace(false);
        else
            initialInterface.loadFinished(false);
    }

    onClosing: function(close) {
        close.accepted = false;
        if (isRecording) {
            messageDialogLoader.showDialog(VNoteMessageDialogHandler.AbortRecord, ret => {
                if (ret) {
                    workspaceLoader.item.stopAndClose();
                    VNoteMainManager.forceExit(true);
                }
            });
        } else if (VNoteMainManager.isVoiceToText()) {
            messageDialogLoader.showDialog(VNoteMessageDialogHandler.AborteAsr, ret => {
                if (ret)
                    VNoteMainManager.forceExit();
            });
        } else {
            VNoteMainManager.forceExit();
        }
    }

    // 初始界面（workspaceLoader 未激活）下注册 Ctrl+N 和 F1 快捷键
    // Shortcuts.qml 仅在 MainWindow.qml 内通过 workspaceLoader 实例化，
    // 无记事本时 workspaceLoader.active=false，Shortcuts.qml 不存在。
    // 此处通过 enabled: !workspaceLoader.active 与 Shortcuts.qml 互斥，
    // 工作区加载后由 Shortcuts.qml 接管（含拖拽/录音/搜索等屏蔽逻辑）。
    Shortcut {
        sequence: "Ctrl+N"
        enabled: !workspaceLoader.active
        onActivated: {
            rootWindow.activateWorkspace(true);
        }
    }

    Shortcut {
        sequence: "F1"
        enabled: !workspaceLoader.active
        onActivated: {
            ApplicationHelper.handleHelpAction();
        }
    }

    Loader {
        id: workspaceLoader

        anchors.fill: parent
        active: false
        asynchronous: false

        onLoaded: {
            initialInterface.visible = false;
            VNoteMainManager.start();
        }
        onStatusChanged: {
            if (status === Loader.Error) {
                console.error("Failed to load workspace:", source);
                workspaceLoader.active = false;
                initialInterface.loadFinished(false);
            }
        }
    }

    InitialInterface {
        id: initialInterface
        Accessible.name: "InitialInterface"
        Accessible.role: Accessible.Pane

        anchors.fill: parent
        visible: !workspaceLoader.active

        onCreateFolder: {
            rootWindow.activateWorkspace(true);
        }
        onTitleOpenSetting: {
            if (settingDlgLoader.status === Loader.Null)
                settingDlgLoader.setSource("gui/dialog/SettingDialog.qml");
            if (settingDlgLoader.status === Loader.Ready)
                settingDlgLoader.item.show();
        }
    }

    VNoteMessageDialogLoader {
        id: messageDialogLoader
        Accessible.name: "MessageDialogLoader"
        Accessible.role: Accessible.Dialog
    }

    Loader {
        id: settingDlgLoader
    }

    // 迁移期间覆盖整个主窗口，避免 titlebar 区域残留或可交互。
    MigrationUpgradeView {
        Accessible.name: "MainWindow_UpgradeView"
        Accessible.role: Accessible.Pane

        id: migrationView

        anchors.fill: parent
    }

    Connections {
        target: VNoteMainManager

        function onInitialDataReady() {
            if (!rootWindow.createFirstNotebook)
                return;

            rootWindow.createFirstNotebook = false;
            VNoteMainManager.vNoteCreateFolder();
        }
    }
}
