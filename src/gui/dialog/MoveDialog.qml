// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import org.deepin.dtk 1.0

DialogWindow {
    id: dialog

    property alias folderModel: folderList.model
    property int index: -1
    property string name

    signal moveToFolder(int index)

    height: 365
    width: 370

    header: DialogTitleBar {
        enableInWindowBlendBlur: true
        title: qsTr("Move Note")
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            id: description

            Layout.alignment: Qt.AlignHCenter
            font: DTK.fontManager.t5
            text: name
        }

        ListView {
            id: folderList

            clip: true
            height: 226
            width: 348

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                backgroundVisible: true
                height: 30
                spacing: 8
                width: 336

                // 条件设置normalBackgroundVisible属性，仅在支持的DTK版本中使用
                Component.onCompleted: {
                    if (DTK.majorVersion >= 6) {
                        // 在DTK 6.x中可能支持此属性
                        if (typeof normalBackgroundVisible !== "undefined") {
                            normalBackgroundVisible = index % 2 === 0;
                        }
                    }
                }

                onClicked: {
                    dialog.index = index;
                }

                // text: model.name
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
                            //矩形
                            id: _mask

                            antialiasing: true
                            color: "red"
                            height: 16
                            radius: 8
                            smooth: true
                            visible: false  //不可见
                            width: 16
                        }

                        OpacityMask {
                            id: mask_image

                            anchors.fill: _image
                            antialiasing: true
                            maskSource: _mask    //用作遮罩的项目
                            source: _image
                            visible: true
                        }
                    }

                    Label {
                        id: folderNameLabel

                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignLeft
                        text: model.name
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            Layout.bottomMargin: 10
            Layout.fillWidth: true
            Layout.topMargin: 10

            Button {
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

                Layout.alignment: Qt.AlignRight
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
