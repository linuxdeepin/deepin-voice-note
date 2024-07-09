// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import "../dialog/"
import "../drag/"

import org.deepin.dtk 1.0

import VNote 1.0

Item {
    property int noteItemHeight: 50
    width: 640
    height: 480
    visible: true

    property var selectedNoteItem: []
    property var itemSpacing: 6
    property alias model: itemModel
    property var saveFilters: ["TXT(*.txt);;HTML(*.html)", "TXT(*.txt)", "HTML(*.html)", "MP3(*.mp3)"]

    signal noteItemChanged(int index)
    signal mouseChanged(int mousePosX, int mousePosY)
    signal dropRelease()

    ListModel {
        id: itemModel
    }

    MoveDialog {
        id: dialogWindow
    }

    DragControl {
        id: dragControl
    }

    Loader {
        id: fileDialogLoader
        property var saveType: VNoteMainManager.Note
        active: false
        asynchronous: true
        sourceComponent: FileDialog {
            id: fileDialog
            title: qsTr("Save As")
            currentFile: "file:///" + selectedNoteItem[0].name
            fileMode: FileDialog.SaveFile
            currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
            nameFilters: saveFilters[saveType]
            Component.onCompleted: {
                fileDialog.open()
            }
            onAccepted: {
                VNoteMainManager.saveAs(selectedNoteItem, fileDialog.selectedFile, saveType)
            }
        }
    }

    Loader {
        id: folderDialogLoader
        property var saveType: VNoteMainManager.Note
        active: false
        asynchronous: true
        sourceComponent: FolderDialog {
            id: folderDialog
            title: qsTr("Save As")
            currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
            Component.onCompleted: {
                folderDialog.open()
            }
            onAccepted: {
                VNoteMainManager.saveAs(selectedNoteItem, folderDialog.selectedFolder, saveType)
            }
        }
    }

    ListView {
        id: itemListView
        anchors.fill: parent
        model: itemModel
        property var lastCurrentIndex: -1
        property var contextIndex: -1
        spacing: itemSpacing
        delegate: Rectangle {
            width: parent.width
            height: noteItemHeight
            property alias setNameColor: noteNameLabel.color
            property alias setTimeColor: timeLabel.color
            property color nameColor: "black"
            property color timeColor: "#7F000000"
            
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
                    color: nameColor
                    font.pixelSize: 20
                }
                Label {
                    id: timeLabel
                    width: parent.width
                    horizontalAlignment: Text.AlignHLeft
                    text: model.name
                    color: timeColor
                    font.pixelSize: 10
                }
            }
            MouseArea {
                id : noteItemMouseArea
                anchors.fill: parent
                drag.target: this
                property bool held: false
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: {
                    if (mouse.button === Qt.RightButton) {
                        // if (selectedNoteItem.length === 1) {
                            itemListView.contextIndex = index
                            noteContextMenu.popup()
                        // }
                    } else {
                        if (itemListView.currentIndex === index) {
                            return;
                        }
                        switch (mouse.modifiers) {
                            case Qt.ControlModifier:
                                if (selectedNoteItem.indexOf(index) != -1) {
                                    selectedNoteItem.splice(selectedNoteItem.indexOf(index), 1)
                                    itemListView.itemAtIndex(index).color = "white"
                                    noteNameLabel.color = "black"
                                    timeLabel.color = "#7F000000"
                                } else {
                                    selectedNoteItem.push(index)
                                    itemListView.itemAtIndex(index).color = "#FF1F6EE7"
                                    noteNameLabel.color = "white"
                                    timeLabel.color = "#7FFFFFFF"
                                }
                            break;
                            case Qt.ShiftModifier:
                                var startIndex = Math.min(index, itemListView.lastCurrentIndex)
                                var endIndex = Math.max(index, itemListView.lastCurrentIndex)
                                for (var i = startIndex; i <= endIndex; i++) {
                                    selectedNoteItem.push(i)
                                    itemListView.itemAtIndex(i).color = "#FF1F6EE7"
                                    noteNameLabel.color = "white"
                                    timeLabel.color = "#7FFFFFFF"
                                }
                            break;
                            default:
                                if (itemListView.lastCurrentIndex != -1) {
                                    if (itemListView.itemAtIndex(itemListView.lastCurrentIndex)) {
                                        var lastItem = itemListView.itemAtIndex(itemListView.lastCurrentIndex)
                                        lastItem.color = "white"
                                        lastItem.setNameColor = "black"
                                        lastItem.setTimeColor = "#7F000000"
                                    }
                                }
                                for (var i = 0; i < selectedNoteItem.length; i++) {
                                    var item = itemListView.itemAtIndex(selectedNoteItem[i])
                                    item.color = "white"
                                    item.setNameColor = "black"
                                    item.setTimeColor = "#7F000000"
                                }
                                selectedNoteItem = []
                                var item = itemListView.model.get(index)
                                selectedNoteItem.push(index)
                                itemListView.currentIndex = index
                                parent.color = "#FF1F6EE7"
                                noteNameLabel.color = "white"
                                timeLabel.color = "#7FFFFFFF"
                                itemListView.lastCurrentIndex = index
                            break;
                        }
                    }
                }
                onPressed: {
                    held = true
                }
                onReleased: {
                    if (held && dragControl.visible && mouse.button === Qt.LeftButton) {
                        if (mouse.x < 0 || mouse.x > parent.width || mouse.y < 0 || mouse.y > parent.height) {
                            held = false
                            dragControl.visible = false
                            return;
                        }
                        dropRelease()
                        //从model中删除选中的item
                        for (var i = 0; i < selectedNoteItem.length; i++) {
                            var item = itemModel.get(selectedNoteItem[i])
                            itemModel.remove(selectedNoteItem[i])
                        }
                        selectedNoteItem = []
                        itemListView.lastCurrentIndex = -1
                        parent.color = "white"
                        noteNameLabel.color = "black"
                        timeLabel.color = "#7F000000"
                    }
                    held = false
                    dragControl.visible = false
                }
                onPositionChanged: {
                    if (held && itemModel.get(selectedNoteItem[0])) {
                        var globPos = mapToGlobal(mouse.x, mouse.y)
                        dragControl.itemNumber = selectedNoteItem.length
                        dragControl.itemText = itemModel.get(selectedNoteItem[0]).name
                        dragControl.visible = true
                        dragControl.x = globPos.x
                        dragControl.y = globPos.y
                        mouseChanged(globPos.x, globPos.y)
                    } else {
                        dragControl.visible = false
                    }
                }
            }
        }

        section {
            property: "isTop"
            criteria: ViewSection.FullString

            delegate: Rectangle {
                width: parent.width
                height: section == "top" ? 16 : 10
                color: "transparent"
                Text {
                    visible: section == "top"
                    anchors.left: parent.left
                    text: qsTr("Sticky Notes")
                    font.pixelSize: 12
                    color: "#b3000000"
                }
                Rectangle {
                    visible: section != "top" && VNoteMainManager.getTop()
                    anchors.centerIn: parent
                    width: parent.width
                    height: 1
                    color: "#0D000000"
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
                    var setTop = true;
                    if (itemModel.get(itemListView.contextIndex).isTop === "top") {
                        setTop = false;
                    }
                    VNoteMainManager.updateTop(itemListView.contextIndex, setTop)
                }
            }
            MenuItem {
                text: qsTr("Move")
                onTriggered: {
                    dialogWindow.show()
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
            Menu {
                title: qsTr("Save note")
                id: saveNoteSubMenu
                MenuItem {
                    text: qsTr("Save as HTML")
                    onTriggered: {
                        if (!fileDialogLoader.active) {
                            fileDialogLoader.saveType = VNoteMainManager.Html
                            fileDialogLoader.active = true
                        } else {
                            fileDialogLoader.item.open()
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Save as TXT")
                    onTriggered: {
                        if (!fileDialogLoader.active) {
                            fileDialogLoader.saveType = VNoteMainManager.Text
                            fileDialogLoader.active = true
                        } else {
                            fileDialogLoader.item.open()
                        }
                    }
                }
            }   
            MenuItem {
                text: qsTr("Save recordings")
                onTriggered: {
                    if (!folderDialogLoader.active) {
                        folderDialogLoader.saveType = VNoteMainManager.Voice
                        folderDialogLoader.active = true
                    } else {
                        folderDialogLoader.item.open()
                    }
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
            var index = itemListView.currentIndex
            noteItemChanged(index)
        }
    }
}
