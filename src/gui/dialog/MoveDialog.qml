// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import org.deepin.dtk 1.0

DialogWindow {
    property alias folderModel: folderList.model
    property int index: -1

    signal moveToFolder(int index)

    id: dialog
    width: 370
    height: 365

    ColumnLayout {
        anchors.fill: parent

        Label {
            Layout.alignment: Qt.AlignHCenter
            font: DTK.fontManager.t5
            text: qsTr("move note to:")
        }

        ListView {
            id: folderList
            width: 348
            height: 226
            clip: true
            delegate: ItemDelegate {
                width: 336
                height: 30
                spacing: 8
                backgroundVisible: index % 2 === 0
                // text: model.name
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        Layout.alignment: Qt.AlignVCenter
                        Image {
                            id: _image
                            smooth: true
                            visible: false
                            width: 16
                            height: 16
                            source: "image://Provider/" + model.icon
                            fillMode: Image.PreserveAspectCrop
                            antialiasing: true
                        }
                        Rectangle { //矩形
                            id: _mask
                            width: 16
                            height: 16
                            radius: 8
                            color: "red"
                            visible: false  //不可见
                            smooth: true
                            antialiasing: true
                        }

                        OpacityMask {
                            id: mask_image
                            anchors.fill: _image
                            source: _image
                            maskSource: _mask    //用作遮罩的项目
                            visible: true
                            antialiasing: true
                        }
                    }
                    Label {
                        id: folderNameLabel
                        Layout.fillWidth: true
                        text: model.name
                        Layout.alignment: Qt.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                    }
                }
                onClicked: {
                    dialog.index = index
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }

        RowLayout {
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            Layout.bottomMargin: 10
            Layout.topMargin: 10
            Layout.fillWidth: true
            Button {
                text: qsTr("Cancel")
                Layout.preferredWidth: 171
                onClicked: {
                    dialog.close();
                }
            }
            Item {Layout.fillWidth: true}
            RecommandButton {
                id: accpetBtn
                text: qsTr("Ok")
                enabled: index !== -1
                Layout.preferredWidth: 171
                Layout.alignment: Qt.AlignRight

                onClicked: {
                    if (index !== -1) {
                        moveToFolder(index)
                    }
                    dialog.close();
                }
            }
        }
    }
}

