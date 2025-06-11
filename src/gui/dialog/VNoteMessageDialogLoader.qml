// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import VNote 1.0
import org.deepin.dtk 1.0

Loader {
    id: loader

    // 操作回调函数，若有则在关闭时传入操作结果执行
    property var callback: null
    // 消息数据，用于部分消息类型切换展示信息（例如删除文件数量）
    property var messageData
    // 消息类型，不同类型提供不同展示信息
    property int messageType: 0
    // 对话框操作结果 true: 确认/删除 false: 取消
    property bool result: false

    // 操作结束 true: 确认/删除 false: 取消
    signal finished(bool ret)

    // 弹出提示对话框， type 为消息类型，cb 为操作回调函数，回调函数有传入对话框结果 ret，
    // e.g.: dialog.showDialog(VNoteMessageDialogHandler.DeleteNote, function (ret) {})
    //       dialog.showDialog(VNoteMessageDialogHandler.DeleteNote, ret => {})
    function showDialog(type, cb = null) {
        result = false;
        messageType = type;
        callback = cb;
        active = true;
    }

    active: false
    asynchronous: true

    sourceComponent: DialogWindow {
        id: dialog

        readonly property int btnHeight: 30
        readonly property int btnWidth: 167

        flags: Qt.Window | Qt.WindowCloseButtonHint
        height: 158
        maximumHeight: 158
        maximumWidth: 360
        minimumHeight: 158
        minimumWidth: 360
        modality: Qt.ApplicationModal
        visible: false
        width: 360

        header: DialogTitleBar {
            enableInWindowBlendBlur: false

            // 仅保留默认状态，否则 hover 上会有变化效果
            icon {
                mode: DTK.NormalState
                name: "icon_warning"
            }
        }

        Component.onCompleted: {
            show();
        }
        onClosing: {
            finished(result);

            // 如果有回调函数，执行
            if ((null != callback) && (typeof callback == "function")) {
                callback(result);
            }

            // 关闭即销毁
            loader.active = false;
        }
        onVisibleChanged: {
            if (visible) {
                var parentWindow = parent.Window.window;
                if (parentWindow) {
                    setX(parentWindow.x + parentWindow.width / 2 - width / 2);
                    setY(parentWindow.y + parentWindow.height / 2 - height / 2);
                }
            }
        }

        ColumnLayout {
            height: parent.height
            spacing: 10
            width: parent.width

            VNoteMessageDialogHandler {
                id: handler

                messageData: loader.messageData
                messageType: loader.messageType
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.fillWidth: true
                Layout.maximumHeight: 58
                Layout.minimumHeight: 58
                Layout.preferredHeight: 58

                Label {
                    id: notifyText

                    property Palette textColor: Palette {
                        normal: ("black")
                        normalDark: ("white")
                    }

                    Layout.alignment: Qt.AlignCenter
                    Layout.fillWidth: true
                    color: ColorSelector.textColor
                    font: DTK.fontManager.t6
                    horizontalAlignment: Text.AlignHCenter
                    text: handler.mainMessage
                    wrapMode: Text.Wrap
                }

                Label {
                    id: messageText

                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: visible
                    font: DTK.fontManager.t7
                    text: handler.detailMessage
                    visible: handler.detailMessage.length
                    width: dialog.width
                }
            }

            Row {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                Layout.fillHeight: true
                spacing: 6

                Loader {
                    active: !handler.singleButton
                    height: btnHeight
                    width: active ? btnWidth : 0

                    sourceComponent: Button {
                        anchors.fill: parent
                        font: DTK.fontManager.t6
                        text: qsTr("Cancel")

                        onClicked: {
                            dialog.close();
                        }
                    }
                }

                Loader {
                    active: handler.warnConfirm.length
                    height: btnHeight
                    width: active ? btnWidth : 0

                    sourceComponent: WarningButton {
                        anchors.fill: parent
                        font: DTK.fontManager.t6
                        text: handler.warnConfirm

                        onClicked: {
                            result = true;
                            dialog.close();
                        }
                    }
                }

                Loader {
                    active: !handler.warnConfirm.length
                    height: btnHeight
                    width: active ? btnWidth : 0

                    sourceComponent: RecommandButton {
                        anchors.fill: parent
                        font: DTK.fontManager.t6
                        text: qsTr("Confirm")

                        onClicked: {
                            result = true;
                            dialog.close();
                        }
                    }
                }
            }
        }
    }
}
