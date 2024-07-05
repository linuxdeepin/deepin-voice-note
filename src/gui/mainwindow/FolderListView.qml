// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15

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

    ListView {
        id: folderListView
        anchors.fill: parent
        property var lastCurrentIndex: -1
        property int dropIndex: -1
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
            Text {
                text: model.name
                anchors.centerIn: parent
                font.pixelSize: 14
            }
            MouseArea {
                id: folderMouseArea
                anchors.fill: parent
                drag.target: this
                hoverEnabled: true
                property bool held: false
                onClicked: {
                    if (folderListView.lastCurrentIndex != -1) {
                        if (folderListView.itemAtIndex(folderListView.lastCurrentIndex)) {
                            folderListView.itemAtIndex(folderListView.lastCurrentIndex).backgroundColor = "white"
                        }
                    }

                    folderListView.currentIndex = index
                    parent.backgroundColor = "#33000000"
                    folderListView.lastCurrentIndex = index
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
                                folderModel.move(index, folderListView.dropIndex, 1);
                            }
                            var orderList = []
                            // for (var i = 0; i < folderModel.count; i++) {
                            //     console.log("sortNumber is:", folderModel.get(i).sortNumber)
                            //     orderList.push(folderModel.get(i).sortNumber)
                            // }
                            // VNoteMainManager.updateOrder(orderList)
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
        }
        onCurrentItemChanged: {            
            var index = folderListView.currentIndex
            itemChanged(index, folderModel.get(index).name) // 发出 itemChanged 信号
        }
    }
}
