// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import VNote 1.0

// TTP-021: 升级进度界面全屏覆盖层。
// 迁移期间（migrationActive）遮蔽主编辑界面；终态（terminalState 非空）展示
// 备份/报告路径与"进入应用"放行入口；NotNeeded 不展示直接放行。
// 仅绑定计数/阶段/路径属性，不展示笔记正文/信封/meta_data/失败 note id 清单。
Item {
    id: root

    // 覆盖层可见：迁移进行中 或 终态结果待用户放行。
    visible: MigrationViewController.migrationActive || MigrationViewController.terminalState !== ""
    z: 9999

    // 终态结果待放行（Completed/PartialCompleted/Failed）。
    readonly property bool terminalVisible: MigrationViewController.terminalState !== ""
    // 迁移进行中（含等待态/Migrating），非终态。
    readonly property bool runningActive: MigrationViewController.migrationActive && !terminalVisible

    Rectangle {
        id: background
        anchors.fill: parent
        color: "#FFFFFF"
        opacity: 0.97
    }

    // 吸收鼠标事件，迁移期间/终态放行前阻止与下层主编辑界面交互。
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: false
        // 不消费滚轮事件以外的内容；仅拦截点击，避免穿透到编辑器。
        onClicked: function(mouse) { mouse.accepted = true }
        onPressed: function(mouse) { mouse.accepted = true }
        onPositionChanged: function(mouse) { mouse.accepted = true }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        width: Math.min(420, parent.width - 80)

        // 标题（占位文案，视觉由 designer 定稿）。
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: terminalVisible ? terminalTitle() : runningTitle()
            font.pixelSize: 18
            font.bold: true
            color: "#303030"
        }

        // 阶段文案（运行态展示当前阶段）。
        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: runningActive
            text: stageText(MigrationViewController.stage)
            font.pixelSize: 14
            color: "#666666"
        }

        // 进度条（运行态）。
        ProgressBar {
            Layout.fillWidth: true
            visible: runningActive
            // Migrating 阶段按已处理/总数推进；其余阶段不确定进度。
            indeterminate: MigrationViewController.total === 0
            from: 0
            to: Math.max(1, MigrationViewController.total)
            value: MigrationViewController.processed
        }

        // Migrating 计数（已处理/总数/成功/失败）。
        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: runningActive && MigrationViewController.stage === "Migrating"
            text: qsTr("已处理 %1 / %2　成功 %3　失败 %4")
                  .arg(MigrationViewController.processed)
                  .arg(MigrationViewController.total)
                  .arg(MigrationViewController.success)
                  .arg(MigrationViewController.fail)
            font.pixelSize: 13
            color: "#888888"
        }

        // 终态信息：备份与报告路径（不渲染报告正文，仅展示路径供用户查阅）。
        ColumnLayout {
            Layout.fillWidth: true
            visible: terminalVisible
            spacing: 8

            Text {
                Layout.fillWidth: true
                visible: MigrationViewController.backupPath !== ""
                text: qsTr("备份位置：%1").arg(MigrationViewController.backupPath)
                font.pixelSize: 13
                color: "#555555"
                wrapMode: Text.WrapAnywhere
            }
            Text {
                Layout.fillWidth: true
                visible: MigrationViewController.reportPath !== ""
                text: qsTr("迁移报告：%1").arg(MigrationViewController.reportPath)
                font.pixelSize: 13
                color: "#555555"
                wrapMode: Text.WrapAnywhere
            }
            // 部分完成/失败的计数摘要（仅计数，不展示失败 note id 清单）。
            Text {
                Layout.fillWidth: true
                visible: terminalVisible && MigrationViewController.terminalState !== "Completed"
                text: qsTr("成功 %1　失败 %2").arg(MigrationViewController.success).arg(MigrationViewController.fail)
                font.pixelSize: 13
                color: "#888888"
            }
        }

        // 取消按钮（运行态可见；cancelling 时置灰）。
        Button {
            Accessible.name: "CancelMigrationButton"
            Layout.alignment: Qt.AlignHCenter
            visible: runningActive
            enabled: !MigrationViewController.cancelling
            text: MigrationViewController.cancelling ? qsTr("正在取消…") : qsTr("取消迁移")
            onClicked: MigrationViewController.requestCancel()
        }

        // "进入应用"放行入口（终态）。
        Button {
            Accessible.name: "EnterAppButton"
            Layout.alignment: Qt.AlignHCenter
            visible: terminalVisible
            text: qsTr("进入应用")
            onClicked: MigrationViewController.enterApp()
        }
    }

    function runningTitle() {
        return qsTr("正在升级笔记数据，请勿关闭应用");
    }

    function terminalTitle() {
        var s = MigrationViewController.terminalState;
        if (s === "Completed") return qsTr("升级完成");
        if (s === "PartialCompleted") return qsTr("升级部分完成");
        if (s === "Failed") return qsTr("升级失败");
        return qsTr("升级完成");
    }

    function stageText(stage) {
        switch (stage) {
            case "BackingUp": return qsTr("正在备份数据…");
            case "Scanning": return qsTr("正在扫描笔记…");
            case "Migrating": return qsTr("正在迁移笔记…");
            default: return qsTr("正在准备…");
        }
    }
}
