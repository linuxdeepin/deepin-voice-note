// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0

import VNote 1.0

// 升级进度界面全屏覆盖层（回归单一 UpgradeView 身份）。
// 运行态忠实复刻老 QWidget UpgradeView 的视觉：DTK WaterProgressBar 复刻
// DWaterProgress 水波动画 + tooltip 文案，布局对齐老 QWidget（水波居中、
// tooltip 居中置于水波下方）。
// 终态页布局策略：将"进入应用"按钮锚定到窗口底部可视区（位于 Flickable 之外），
// 标题与路径信息放入按钮上方的 Flickable 中独立滚动。这样无论窗口几何如何
// （含小窗口场景），"进入应用"按钮始终可见且可点击，彻底消除
// 原先整页 Flickable 在小窗口下按钮被 clip 截断且不可滚动到达的困死症状。
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

    // 运行态水波进度值（0-100）。Migrating 阶段按已处理/总数推进；
    // 其余运行阶段保持微小值以维持水波动画视觉（对齐老 DWaterProgress 行为）。
    readonly property int waterValue: {
        if (!runningActive) return 0
        if (MigrationViewController.stage === "Migrating" && MigrationViewController.total > 0) {
            return Math.round(MigrationViewController.processed * 100.0 / MigrationViewController.total)
        }
        return 1
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "#FFFFFF"
        opacity: 0.97
    }

    // 吸收鼠标事件，迁移期间/终态放行前阻止与下层主编辑界面交互。
    // 声明于内容层之前，作为底层；上方的内容容器、Flickable、按钮均在其之上，
    // 故按钮点击与 Flickable 滚动手势不会被本 MouseArea 抢占。
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: false
        onClicked: function(mouse) { mouse.accepted = true }
        onPressed: function(mouse) { mouse.accepted = true }
        onPositionChanged: function(mouse) { mouse.accepted = true }
    }

    // 运行态：水波 + tooltip + 阶段 + 计数 + 取消按钮，整体居中。
    // 运行态内容高度有限（水波 80 + 文案数行 + 取消按钮），常规窗口可直接容纳，
    // 不依赖滚动；核心诉求在终态页，运行态布局保持居中复刻老 QWidget。
    ColumnLayout {
        id: runningColumn
        anchors.centerIn: parent
        width: Math.min(420, parent.width - 80)
        visible: runningActive
        spacing: 20

        // 水波进度（复刻老 QWidget DWaterProgress，80x80 居中）。
        // 对齐 src/gui/mainwindow/InitialInterface.qml 的 WaterProgressBar 写法：
        // 只用 value + visible（由父 runningColumn 的 visible 控制运行态显隐），
        // 不设 running——当前 DTK/QML 环境该属性不存在，设置会导致控件创建失败、
        // 中断整个 QML 组件树加载（用户运行时实测根因）。
        WaterProgressBar {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 80
            Layout.preferredHeight: 80
            value: root.waterValue
        }

        // 运行态 tooltip 文案（对齐老 QWidget：水波下方居中提示文本）。
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("正在升级笔记数据，请勿关闭应用")
            font.pixelSize: 18
            font.bold: true
            color: "#303030"
        }

        // 阶段文案（运行态展示当前阶段）。
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: stageText(MigrationViewController.stage)
            font.pixelSize: 14
            color: "#666666"
        }

        // Migrating 计数（已处理/总数/成功/失败）。
        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: MigrationViewController.stage === "Migrating"
            text: qsTr("已处理 %1 / %2　成功 %3　失败 %4")
                  .arg(MigrationViewController.processed)
                  .arg(MigrationViewController.total)
                  .arg(MigrationViewController.success)
                  .arg(MigrationViewController.fail)
            font.pixelSize: 13
            color: "#888888"
        }

        // 取消按钮（运行态可见；cancelling 时置灰）。
        // DTK Button（org.deepin.dtk 1.0），固定宽高保证可见可点。
        Button {
            Accessible.name: "UpgradeView_Button"
            Accessible.role: Accessible.Button

            Layout.alignment: Qt.AlignHCenter
            width: 120
            height: 36
            enabled: !MigrationViewController.cancelling
            text: MigrationViewController.cancelling ? qsTr("正在取消…") : qsTr("取消迁移")
            onClicked: MigrationViewController.requestCancel()
        }
    }

    // 终态层：标题+路径放入 Flickable 独立滚动，"进入应用"按钮锚定窗口底部。
    // 按钮位于 Flickable 之外，任何窗口几何下都恒可见可点。
    Item {
        id: terminalLayer
        anchors.fill: parent
        visible: terminalVisible

        // "进入应用"放行入口：锚定窗口底部居中，始终位于可视区内。
        // DTK RecommandButton（主操作 CTA，对齐 InitialInterface.qml 的主按钮写法），
        // 固定宽高保证可见可点。
        RecommandButton {
            id: enterAppButton
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 24
            anchors.horizontalCenter: parent.horizontalCenter
            width: 120
            height: 36
            text: qsTr("进入应用")
            onClicked: MigrationViewController.enterApp()
            z: 2
        }

        // 可滚动内容区：标题 + 备份/报告路径 + 计数摘要。
        // 顶边贴窗口顶部，底边贴"进入应用"按钮顶部；内容溢出时垂直滚动，
        // 不溢出时直接展示。按钮始终可见，与内容是否溢出无关。
        Flickable {
            id: terminalFlickable
            anchors.top: parent.top
            anchors.topMargin: 40
            anchors.bottom: enterAppButton.top
            anchors.bottomMargin: 20
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(420, parent.width - 80)
            clip: true
            contentHeight: terminalContent.implicitHeight
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            interactive: contentHeight > height

            ScrollBar.vertical: ScrollBar {
                Accessible.name: "UpgradeView_ScrollBar"
                Accessible.role: Accessible.ScrollBar

                policy: terminalFlickable.contentHeight > terminalFlickable.height
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            ColumnLayout {
                id: terminalContent
                width: terminalFlickable.width
                spacing: 16

                // 终态标题。
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: terminalTitle()
                    font.pixelSize: 18
                    font.bold: true
                    color: "#303030"
                }

                // 终态信息：备份与报告路径 + 计数摘要（仅计数，不渲染报告正文）。
                ColumnLayout {
                    Layout.fillWidth: true
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
                        visible: MigrationViewController.terminalState !== "Completed"
                        text: qsTr("成功 %1　失败 %2").arg(MigrationViewController.success).arg(MigrationViewController.fail)
                        font.pixelSize: 13
                        color: "#888888"
                    }
                }
            }
        }
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
