// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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
        readonly property bool isUpdateUosAiDialog: loader.messageType === VNoteMessageDialogHandler.UpdateUosAi
        readonly property int btnWidth: isUpdateUosAiDialog ? 171 : 167
        readonly property int dialogContentHeight: isUpdateUosAiDialog ? 164 : 158

        flags: Qt.Window | Qt.WindowCloseButtonHint
        height: dialogContentHeight
        icon: "dialog-warning"
        maximumHeight: dialogContentHeight
        maximumWidth: 360
        minimumHeight: dialogContentHeight
        minimumWidth: 360
        modality: Qt.ApplicationModal
        visible: false
        width: 360

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
                Layout.maximumHeight: dialog.isUpdateUosAiDialog ? 64 : 58
                Layout.minimumHeight: dialog.isUpdateUosAiDialog ? 64 : 58
                Layout.preferredHeight: dialog.isUpdateUosAiDialog ? 64 : 58
                spacing: dialog.isUpdateUosAiDialog ? 8 : 0

                Label {
                    id: notifyText

                    property Palette textColor: Palette {
                        normal: ("black")
                        normalDark: ("white")
                    }

                    Layout.alignment: Qt.AlignCenter
                    Layout.fillWidth: true
                    Layout.leftMargin: dialog.isUpdateUosAiDialog ? 20 : 0
                    Layout.preferredHeight: dialog.isUpdateUosAiDialog ? 18 : implicitHeight
                    Layout.rightMargin: dialog.isUpdateUosAiDialog ? 20 : 0
                    color: ColorSelector.textColor
                    elide: Text.ElideNone
                    font: DTK.fontManager.t6
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: dialog.isUpdateUosAiDialog ? 1 : 2
                    text: handler.mainMessage
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.Wrap
                }

                Label {
                    id: messageText

                    property Palette textColor: Palette {
                        normal: Qt.rgba(0, 0, 0, dialog.isUpdateUosAiDialog ? 0.7 : 1)
                        normalDark: Qt.rgba(1, 1, 1, dialog.isUpdateUosAiDialog ? 0.7 : 1)
                    }

                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: visible && !dialog.isUpdateUosAiDialog
                    Layout.preferredHeight: dialog.isUpdateUosAiDialog ? 34 : implicitHeight
                    Layout.preferredWidth: dialog.isUpdateUosAiDialog ? 302 : dialog.width
                    color: dialog.isUpdateUosAiDialog ? ColorSelector.textColor : DTK.palette.windowText
                    elide: Text.ElideNone
                    font: DTK.fontManager.t7
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: dialog.isUpdateUosAiDialog ? 2 : 3
                    text: handler.detailMessage
                    verticalAlignment: Text.AlignVCenter
                    visible: handler.detailMessage.length
                    wrapMode: Text.Wrap
                    width: dialog.isUpdateUosAiDialog ? 302 : dialog.width
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
                        Accessible.name: "CancelButton"
                        anchors.fill: parent
                        font: DTK.fontManager.t6
                        text: handler.cancelText

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
                        Accessible.name: dialog.isUpdateUosAiDialog ? "UpdateUosAiButton" : "ConfirmButton"
                        anchors.fill: parent
                        font: DTK.fontManager.t6
                        text: handler.confirmText

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
