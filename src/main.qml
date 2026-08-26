// Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
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

    Accessible.name: "VoiceNoteMainWindow"
    Accessible.role: Accessible.Window

    property bool workspaceActive: workspaceLoader.active
    property bool createFirstNotebook: false
    property bool isRecording: workspaceLoader.item ? workspaceLoader.item.isRecording : false

    DWindow.enabled: true
    color: DTK.themeType === ApplicationHelper.LightType ? "#FFFFFF" : "#101010"
    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 681
    minimumHeight: 300
    minimumWidth: 685
    visible: true
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
    }

    Loader {
        id: settingDlgLoader
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
