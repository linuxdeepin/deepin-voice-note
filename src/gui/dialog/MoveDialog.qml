// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import org.deepin.dtk 1.0
import org.deepin.dtk.style 1.0 as DS
import "../" as VNoteComponents

DialogWindow {
    id: dialog

    property alias folderModel: folderList.model
    property int index: 0
    property string name

    signal moveToFolder(int index)

    height: 365
    width: 360

    onVisibleChanged: if (visible) folderList.currentIndex = 0

    header: DialogTitleBar {
        enableInWindowBlendBlur: true
        title: qsTr("Move Note")
        height: DS.Style.dialogWindow.titleBarHeight
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            id: description

            Layout.alignment: Qt.AlignHCenter
            font: DTK.fontManager.t5
            text: name
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            border.color: DTK.themeType === ApplicationHelper.LightType ? "#1A000000" : "#1AFFFFFF"
            border.width: 1
            height: 228
            radius: 6
            width: 350

            ListView {
                id: folderList
                Accessible.name: "FolderList"

                activeFocusOnTab: true
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                keyNavigationEnabled: true

                // currentIndex 作为选中项的唯一来源，同步给 dialog.index，
                // 使 Tab 可聚焦列表、上/下方向键可切换记事本
                onCurrentIndexChanged: dialog.index = currentIndex

                ScrollBar.vertical: ScrollBar {
                    Accessible.name: "FolderListScrollBar"
                }
                delegate: Rectangle {
                    id: folderItem

                    property bool isHovered: false

                    color: index === dialog.index
                        ? (DTK.themeType === ApplicationHelper.LightType ? "#33000000" : "#33FFFFFF")
                        : (isHovered ? (DTK.themeType === ApplicationHelper.LightType ? "#1A000000" : "#1AFFFFFF") : "transparent")
                    height: 30
                    radius: 6
                    width: 336

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8

                        Rectangle {
                            Layout.alignment: Qt.AlignVCenter
                            height: 16
                            radius: 8
                            width: 16

                            Image {
                                id: _image

                                antialiasing: true
                                fillMode: Image.PreserveAspectCrop
                                height: 16
                                smooth: true
                                source: "image://Provider/" + model.icon
                                visible: false
                                width: 16
                            }

                            Rectangle {
                                id: _mask

                                antialiasing: true
                                color: "red"
                                height: 16
                                radius: 8
                                smooth: true
                                visible: false
                                width: 16
                            }

                            OpacityMask {
                                id: mask_image

                                anchors.fill: _image
                                antialiasing: true
                                maskSource: _mask
                                source: _image
                                visible: true
                            }
                        }

                        Label {
                            id: folderNameLabel

                            Layout.alignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                            color: DTK.palette.windowText
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignLeft
                            text: model.name
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true

                        onClicked: {
                            folderList.currentIndex = index;
                        }
                        onEntered: {
                            folderItem.isHovered = true;
                        }
                        onExited: {
                            folderItem.isHovered = false;
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            Layout.bottomMargin: 10
            Layout.fillWidth: true
            Layout.topMargin: 10

            VNoteComponents.VNoteButton {
                Accessible.name: "CancelButton"
                Accessible.role: Accessible.Button
                Layout.preferredHeight: 30
                Layout.preferredWidth: 171
                text: qsTr("Cancel")

                onClicked: {
                    dialog.close();
                }
            }

            Item {
                Layout.fillWidth: true
            }

            RecommandButton {
                id: accpetBtn

                Accessible.name: "MoveDialogOk"
                Accessible.role: Accessible.Button
                Layout.alignment: Qt.AlignRight
                Layout.preferredHeight: 30
                Layout.preferredWidth: 171
                enabled: index !== -1
                text: qsTr("Ok")

                onClicked: {
                    if (index !== -1) {
                        moveToFolder(index);
                    }
                    dialog.close();
                }
            }
        }
    }
}
