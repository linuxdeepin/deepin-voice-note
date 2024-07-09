// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.1
import org.deepin.dtk

import VNote 1.0
import "../drag/"

Item {
    property int itemHeight: 30
    property int listWidth: 200
    property int listHeight: 700
    width: listWidth
    height: listHeight
    visible: true
    signal itemChanged(int index, string name)

    property alias model: folderListView.model
    property int lastDropIndex: -1
    property int currentDropIndex: -1

    ListModel {
        id: folderModel
    }

    DragControl {
        id: dragControl
    }

    function updateItems(mousePosX, mousePosY) {
        var pos = mapFromGlobal(mousePosX, mousePosY)
        if (pos.x < 0 || pos.x > listWidth) {
            if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
                folderListView.itemAtIndex(lastDropIndex).backgroundColor = "white"
            }
            lastDropIndex = -1;
            return;
        }
        //判断当前鼠标所在的行
        var startY = pos.y - 50 > 0? pos.y - 50 : 0
        var index = Math.floor(pos.y / itemHeight)
        if (index < 0 || index >= folderModel.count) {
            if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
                folderListView.itemAtIndex(lastDropIndex).backgroundColor = "white"
            }
            currentDropIndex = -1;
            return;
        }
        currentDropIndex = index
        if (index != lastDropIndex) {
            if (lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
                folderListView.itemAtIndex(lastDropIndex).backgroundColor = "white"
            }
            lastDropIndex = index;
            if (index === folderListView.currentIndex)  {
                return;
            }
        } else {
            return;
        }
        //更新当前行的颜色
        if (folderListView.itemAtIndex(index)) {
            folderListView.itemAtIndex(index).backgroundColor = "#1A000000"
        }
    }

    function dropItems(selectedNoteItem) {
        if (currentDropIndex != -1) {
            VNoteMainManager.moveNotes(selectedNoteItem, currentDropIndex)
        }
        if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
            folderListView.itemAtIndex(lastDropIndex).backgroundColor = "white"
        }
        lastDropIndex = -1;
        currentDropIndex = -1;
    }

    function addFolder() {
        VNoteMainManager.vNoteCreateFolder()
    }

    function renameFolder(index, isRename) {
        var item = folderListView.itemAtIndex(index)
        item.folderNameLabel.visible = !isRename
        item.folderCountLabel.visible = !isRename
        item.renameLine.visible = isRename
    }

    Connections {
        target: VNoteMainManager
        onAddFolderFinished: {
            folderListView.model.insert(0, {name: folderData.name, count: folderData.notesCount})

            if (folderListView.itemAtIndex(folderListView.currentIndex + 1)) {
                folderListView.itemAtIndex(folderListView.currentIndex + 1).backgroundColor = "white"
            }

            folderListView.currentIndex = 0
            folderListView.lastCurrentIndex = 0
            VNoteMainManager.createNote()
        }
    }

    ListView {
        id: folderListView
        anchors.fill: parent
        property var lastCurrentIndex: -1
        property int dropIndex: -1
        property var contextIndex: -1
        model: folderModel
        function indexAt(mousePosX, mousePosY) {
            var pos = mapFromGlobal(mousePosX, mousePosY)
            var startY = itemHeight * 0.5
            var index = Math.floor((pos.y - startY) / itemHeight) + 1
            if (index < 0) {
                index = 0;
            }
            if (index >= folderModel.count) {
                index = folderModel.count;
            }
            dropIndex = index
        }
        delegate: Item {
            width: parent.width
            height: itemHeight
            property color backgroundColor: index === folderListView.currentIndex ? "#33000000" : "white"
            
            Rectangle {
                id: rect
                anchors.fill: parent
                color: backgroundColor
                radius: 6
            }
            RowLayout {
                id: rowlayout
                anchors.fill: parent
                property int imageWidth: 16
                spacing: 10
                Layout.fillWidth: true
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.rightMargin: 10
                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    anchors.top: parent.top
                    anchors.topMargin: 7
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 7
                    Image {
                        id: _image
                        smooth: true
                        visible: false
                        width: 16
                        height: 16
                        source: "file:///home/V23/Desktop/123.jpg"
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
                LineEdit {
                    id: renameLine
                    anchors.fill: parent
                    visible: false
                    text: model.name
                    onFocusChanged: {
                        folderMouseArea.enabled = false;
                        if (focus) {
                            focus = true
                            selectAll();
                        } else {
                            folderMouseArea.enabled = true;
                            deselect();
                            visible = false
                            folderNameLabel.visible = true
                            folderCountLabel.visible = true
                        }
                    }
                }
                Label {
                    id: folderNameLabel
                    text: model.name
                    visible: true
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                }
                Label {
                    id: folderCountLabel
                    text: model.count
                    width: 30
                    visible: true
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                }
            }
            MouseArea {
                id: folderMouseArea
                anchors.fill: parent
                drag.target: this
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                property bool held: false
                onClicked: {
                    if (mouse.button === Qt.RightButton) {
                        folderListView.contextIndex = index
                        folderItemContextMenu.popup()
                    } else {
                        if (folderListView.lastCurrentIndex != -1) {
                            if (folderListView.itemAtIndex(folderListView.lastCurrentIndex)) {
                                folderListView.itemAtIndex(folderListView.lastCurrentIndex).backgroundColor = "white"
                            }
                        }

                        folderListView.currentIndex = index
                        parent.backgroundColor = "#33000000"
                        folderListView.lastCurrentIndex = index
                    }
                }
                onDoubleClicked: {
                    console.log("111111111111111")
                    // renameLine.select(0, renameLine.text.length)
                    // renameLine.focus = true
                }

                onEntered: {
                    if (folderListView.currentIndex == index) {
                        return
                    }
                    parent.backgroundColor = "#1A000000"
                }
                onExited: {
                    if (folderListView.currentIndex == index) {
                        return
                    }
                    parent.backgroundColor = "white"
                }
                onPressed: {
                    held = true
                    dragControl.isFolder = true
                }
                onReleased: {
                    if (held) {
                        held = false
                        dragControl.visible = false
                        dragControl.imageSource = ""
                        if (folderListView.dropIndex != -1) {
                            if (folderListView.dropIndex > index) {
                                folderListView.dropIndex -= 1
                            }
                            if (folderListView.dropIndex != index) {
                                var tmpIndex = index
                                folderModel.move(index, folderListView.dropIndex, 1);
                                VNoteMainManager.updateSort(tmpIndex, folderListView.dropIndex)
                            }
                        }
                    }
                }
                onPositionChanged: {
                    if (held) {
                        if (held && folderModel.get(folderListView.currentIndex)) {
                            if (dragControl.imageSource === "") {
                                parent.grabToImage(function(result) {
                                    dragControl.imageSource = result.url
                                })
                            }
                            var globPos = mapToGlobal(mouse.x, mouse.y)
                            dragControl.itemNumber = 1
                            dragControl.visible = true
                            dragControl.x = globPos.x
                            dragControl.y = globPos.y
                            folderListView.indexAt(globPos.x, globPos.y)
                        } else {
                            dragControl.visible = false
                        }
                    }
                }
            }
            Menu {
                id: folderItemContextMenu
                MenuItem {
                    text: qsTr("Rename")
                    onTriggered: {
                        // renameFolder(folderListView.contextIndex, true)
                        folderNameLabel.visible = false
                        folderCountLabel.visible = false
                        renameLine.visible = true
                        renameLine.focus = true
                    }
                }
                MenuItem {
                    text: qsTr("Delete")
                    onTriggered: {
                        VNoteMainManager.vNoteDeleteFolder(folderListView.contextIndex)
                        folderModel.remove(folderListView.contextIndex)
                        if (folderListView.contextIndex === 0) {
                            folderListView.currentIndex = 0
                        }
                    }
                }
                MenuItem {
                    text: qsTr("New Note")
                    onTriggered: {
                        addFolder()
                    }
                }
            }
            Keys.onPressed: {
                switch (event.key)
                {
                case Qt.Key_Enter:
                case Qt.Key_Return:
                    var lastName = model.name
                    if (lastName != renameLine.text) {
                        VNoteMainManager.renameFolder(index, renameLine.text)
                        model.name = renameLine.text
                        renameLine.visible = false
                        folderNameLabel.visible = true
                        folderCountLabel.visible = true
                    }
                    break;
                case Qt.Key_Escape:
                    console.log("esc")
                    break;
                default:
                    break;
                }
            }
        }
        onCurrentItemChanged: {
            var index = folderListView.currentIndex
            // if (folderListView.itemAtIndex(folderListView.lastCurrentIndex).renameLine) {
            //     renameFolder(folderListView.lastCurrentIndex, true)
            // }

            itemChanged(index, folderModel.get(index).name) // 发出 itemChanged 信号
        }
    }
}
