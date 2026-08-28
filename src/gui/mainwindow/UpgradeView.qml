// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

import org.deepin.dtk 1.0
import VNote 1.0

// 使用 DTK Declarative 原生 WaterProgressBar 复刻原始 QWidget UpgradeView。
// 页面只覆盖 titlebar 下方的 central widget 区域。
Item {
    id: root

    property int titleBarHeight: 0

    readonly property int progressValue: {
        if (MigrationViewController.stage === "Migrating"
                && MigrationViewController.total > 0) {
            return Math.round(MigrationViewController.processed * 100.0
                              / MigrationViewController.total)
        }
        return 1
    }

    visible: MigrationViewController.migrationActive
    z: 9999

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: root.titleBarHeight
        anchors.bottom: parent.bottom
        color: Window.window
               ? Window.window.palette.window
               : (DTK.themeType === ApplicationHelper.LightType ? "#FFFFFF" : "#242424")
    }

    // central widget 区域阻止迁移期间的编辑器交互，但不覆盖 titlebar。
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: root.titleBarHeight
        anchors.bottom: parent.bottom
        acceptedButtons: Qt.AllButtons
        z: 1
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: root.titleBarHeight
        anchors.bottom: parent.bottom
        spacing: 0
        z: 2

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        WaterProgressBar {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 80
            Layout.preferredHeight: 80
            running: root.visible
            value: root.progressValue
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 10
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            color: Window.window
                   ? Window.window.palette.text
                   : (DTK.themeType === ApplicationHelper.LightType ? "#000000" : "#FFFFFF")
            font.pixelSize: 14
            text: qsTr("Importing notes from the old version, please wait...")
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // 原始 UpgradeView 在升级完成后通过 upgradeDone 切回主页面；
    // 这里保持相同语义，不展示额外的终态报告页面。
    Connections {
        target: MigrationViewController

        function onTerminalStateChanged() {
            if (MigrationViewController.terminalState !== "") {
                MigrationViewController.enterApp()
            }
        }
    }
}
