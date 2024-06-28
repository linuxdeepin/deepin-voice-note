// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

import VNote 1.0

Item {
    property int noteItemHeight: 50
    width: 640
    height: 480
    visible: true
    signal noteItemChanged(int index)

    property alias model: itemModel

    ListModel {
        id: itemModel
    }

    ListView {
        id: itemListView
        anchors.fill: parent
        model: itemModel
        property var lastCurrentIndex: -1
        property var contextIndex: -1
        spacing: 6
        delegate: Rectangle {
            width: parent.width
            height: noteItemHeight
            color: "white"
            radius: 6

            Column {
                anchors.top: parent.top
                // anchors.topMargin: 8
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                // anchors.bottomMargin: 8
                spacing: 1
                Label {
                    id: noteNameLabel
                    width: parent.width
                    horizontalAlignment: Text.AlignHLeft
                    text: model.name
                    color: "black"
                    font.pixelSize: 20
                }
                Label {
                    id: timeLabel
                    width: parent.width
                    horizontalAlignment: Text.AlignHLeft
                    text: model.name
                    color: "#7F000000"
                    font.pixelSize: 10
                }
            }
            MouseArea {
                id : noteItemMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: {
                    if (mouse.button === Qt.RightButton) {
                        itemListView.contextIndex = index
                        noteContextMenu.popup()
                    } else {
                        console.log("lastCurrentIndex: " + itemListView.lastCurrentIndex)
                        if (itemListView.lastCurrentIndex != -1) {
                            if (itemListView.itemAtIndex(itemListView.lastCurrentIndex)) {
                                itemListView.itemAtIndex(itemListView.lastCurrentIndex).color = "white"
                                noteNameLabel.color = "black"
                                timeLabel.color = "#7F000000"
                            }
                        }
                        console.log("current index: " + index)
                        itemListView.currentIndex = index
                        parent.color = "#FF1F6EE7"
                        noteNameLabel.color = "white"
                        timeLabel.color = "#7FFFFFFF"
                        itemListView.lastCurrentIndex = index
                    }
                }
            }
        }

        Menu {
            id: noteContextMenu
            MenuItem {
                text: qsTr("Rename")
                onTriggered: {
                }
            }
            MenuItem {
                text: qsTr("Sticky on Top")
                onTriggered: {
                }
            }
            MenuItem {
                text: qsTr("Move")
                onTriggered: {
                }
            }
            MenuItem {
                text: qsTr("Delete")
                onTriggered: {
                    itemModel.remove(itemListView.contextIndex)
                    var indexList = [itemListView.contextIndex]
                    VNoteMainManager.deleteNote(indexList)
                }
            }
            MenuItem {
                text: qsTr("Save note")
                //子菜单
                Menu {
                    id: saveNoteSubMenu
                    MenuItem {
                        text: qsTr("Save as HTML")
                        onTriggered: {
                        }
                    }
                    MenuItem {
                        text: qsTr("Save as TXT")
                        onTriggered: {
                        }
                    }
                }
            }   
            MenuItem {
                text: qsTr("Save recordings")
                onTriggered: {
                }
            }
            MenuSeparator { }
            MenuItem {
                text: qsTr("New note")
                onTriggered: {
                    VNoteMainManager.createNote()
                }
            }
        }
        onCurrentItemChanged: {
            console.log("current item changed: " + itemListView.currentIndex)
            var index = itemListView.currentIndex
            noteItemChanged(index)
        }
    }
}
