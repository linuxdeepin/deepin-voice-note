// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

import org.deepin.dtk 1.0
import VNote 1.0

// 迁移升级覆盖层，避免与旧 QWidget UpgradeView 类型名冲突。
// 使用 DTK Declarative 原生 WaterProgressBar 复刻原始 QWidget UpgradeView。
// 迁移期间作为全窗口遮罩显示，覆盖包括 QML titlebar 在内的全部内容。
Item {
    id: root


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
        anchors.fill: parent
        color: Window.window
               ? Window.window.palette.window
               : (DTK.themeType === ApplicationHelper.LightType ? "#FFFFFF" : "#242424")
    }

    // 全窗口阻止迁移期间的所有交互，包括 QML titlebar 区域。
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        z: 1
    }

    ColumnLayout {
        anchors.fill: parent
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
            // 复用旧 QWidget UpgradeView 已有翻译上下文，避免 QML 文件名
            // 变更导致运行时找不到既有 ts/qm 翻译。
            text: qsTranslate("UpgradeView", "Importing notes from the old version, please wait...")
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
