// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.deepin.dtk
import Qt.labs.platform
import QtQuick.Controls

import VNote 1.0

ApplicationWindow {
    property int windowMiniWidth: 1070
    property int windowMiniHeight: 680
    property int leftViewWidth: 200
    property int createFolderBtnHeight: 40

    id: rootWindow
    visible: true
    minimumWidth: windowMiniWidth
    minimumHeight: windowMiniHeight
    width: windowMiniWidth
    height: windowMiniHeight
    DWindow.enabled: true
    DWindow.alphaBufferSize: 8
    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    Connections {
        function handleFinishedFolderLoad(foldersData) {
            for (var i = 0; i < foldersData.length; i++) {
                folderListView.model.append({name: foldersData[i].name, count: foldersData[i].notesCount})
            }
        }

        function handleUpdateNoteList(notesData) {
            itemListView.model.clear()
            for (var i = 0; i < notesData.length; i++) {
                var itemIsTop = notesData[i].isTop ? "top" : "normal"
                itemListView.model.append({name: notesData[i].name, color: "red", isTop: itemIsTop})
            }
        }
        target: VNoteMainManager
        onFinishedFolderLoad: {
            handleFinishedFolderLoad(foldersData)
        }
        onUpdateNotes : {
            handleUpdateNoteList(notesData)
        }
        onAddNoteAtHead : {
            itemListView.model.insert(0, {name: noteData.name, color: "red"})
        }
    }

    Row {
        Rectangle {
            id: leftBgArea
            width: leftViewWidth
            height: windowMiniHeight
            color: DTK.themeType === ApplicationHelper.LightType ? Qt.rgba(1, 1, 1, 0.9)
                                                                      : Qt.rgba(16.0/255.0, 16.0/255.0, 16.0/255.0, 0.85)
            Rectangle {
                width: 1
                height: parent.height
                anchors.right: parent.right
                color: DTK.themeType === ApplicationHelper.LightType ? "#eee7e7e7"
                                                                     : "#ee252525"
            }
            Column {
                width: parent.width
                height: parent.height

                anchors.top: parent.top
                anchors.topMargin: 50
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10

                FolderListView {
                    id: folderListView
                    width: parent.width
                    height: parent.height - button.height

                    onItemChanged: {
                        label.text = name
                        VNoteMainManager.vNoteFloderChanged(index)
                    }
                }
                Button {
                    id: button
                    width: parent.width
                    height: createFolderBtnHeight
                    text: qsTr("Create Notebook")
                    onClicked: {
                        folderListView.addFolder()
                    }
                }
            }
            Connections {
                target: itemListView
                onMouseChanged: {
                    folderListView.updateItems(mousePosX, mousePosY)
                }
                onDropRelease: {
                    folderListView.dropItems(itemListView.selectedNoteItem)
                }
            }
        }
        Rectangle {
            id: leftDragHandle
            width: 5
            height: windowMiniHeight
            color: "transparent"
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.target: leftDragHandle
                drag.axis: Drag.XAxis
                onPositionChanged: {
                    if (drag.active) {
                        var newWidth = leftDragHandle.x + leftDragHandle.width - leftBgArea.x;
                        if (newWidth >= 100 && newWidth <= 400) {
                            leftBgArea.width = newWidth;
                            rightBgArea.width = rowLayout.width - leftBgArea.width - leftDragHandle.width;
                        }
                    }
                }
            }
        }
        Rectangle {
            id: middeleBgArea
            width: leftViewWidth
            height: windowMiniHeight
            // color: DTK.themeType === ApplicationHelper.LightType ? Qt.rgba(1, 1, 1, 0.9)
            //                                                           : Qt.rgba(16.0/255.0, 16.0/255.0, 16.0/255.0, 0.85)
            color: Qt.rgba(0, 0, 0, 0.05)

            Column {
                width: parent.width
                height: parent.height
                Label {
                    id: label
                    width: parent.width
                    height: 40
                    font.pixelSize: 16
                    color: "#BB000000"
                    text: ""
                }
                ItemListView {
                    id: itemListView
                    z: 1000
                    width: parent.width
                    height: parent.height - label.height

                    onNoteItemChanged : {
                        VNoteMainManager.vNoteChanged(index)
                    }
                }
            }
        }
        Rectangle {
            id: rightDragHandle
            width: 5
            height: windowMiniHeight
            color: "transparent"
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.target: rightDragHandle
                drag.axis: Drag.XAxis
                onPositionChanged: {
                    if (drag.active) {
                        var newWidth = rightDragHandle.x + rightDragHandle.width - middeleBgArea.x;
                        if (newWidth >= 100 && newWidth <= 400) {
                            middeleBgArea.width = newWidth;
                            rightBgArea.width = rowLayout.width - middeleBgArea.width - rightDragHandle.width;
                        }
                    }
                }
            }
        }
        Rectangle {
            id: rightBgArea
            z: 500
            width: windowMiniWidth - leftBgArea.width
            height: windowMiniHeight
            color: Qt.rgba(0, 0, 0, 0.01)
            BoxShadow {
                anchors.fill: rightBgArea
                shadowOffsetX: 0
                shadowOffsetY: 4
                shadowColor: Qt.rgba(0, 0, 0, 0.05)
                shadowBlur: 10
                cornerRadius: rightBgArea.radius
                spread: 0
                hollow: true
            }
            Column {
                width: parent.width
                height: parent.height

                WebEngineView {
                    id: webEngineView
                    z: 400
                    width: parent.width
                    height: parent.height - label.height
                }
            }
        }
    }
}