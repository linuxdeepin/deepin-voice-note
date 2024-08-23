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
                folderListView.model.append({name: foldersData[i].name, count: foldersData[i].notesCount, icon: foldersData[i].icon})
            }
        }

        function handleUpdateNoteList(notesData) {
            itemListView.model.clear()
            for (var i = 0; i < notesData.length; i++) {
                var itemIsTop = notesData[i].isTop ? "top" : "normal"
                itemListView.model.append({name: notesData[i].name, time: notesData[i].time, isTop: itemIsTop,
                                              icon: notesData[i].icon, folderName: notesData[i].folderName})
            }
        }
        target: VNoteMainManager
        onFinishedFolderLoad: {
            if (foldersData.length > 0)
                initRect.visible = false
            initiaInterface.loadFinished(foldersData.length > 0)
            handleFinishedFolderLoad(foldersData)
        }
        onUpdateNotes : {
            handleUpdateNoteList(notesData)
        }
        onAddNoteAtHead : {
            itemListView.model.insert(0, {name: noteData.name, time: noteData.time})
        }
        onNoSearchResult: {
            label.visible = false
            folderListView.opacity = 0.4
            folderListView.enabled = false
            createFolderButton.enabled = false
        }
        onSearchFinished: {
            label.visible = false
            folderListView.opacity = 0.4
            folderListView.enabled = false
            createFolderButton.enabled = false
        }
    }

    IconLabel {
        id: appImage
        icon.name: "deepin-voice-note"
        width: 20
        height: 20
        x: 10
        y: 10
        z: 100
    }

    RowLayout {
        id: rowLayout
        anchors.fill: parent

        Rectangle {
            id: leftBgArea
            Layout.preferredWidth: leftViewWidth
            Layout.fillHeight: true
            color: DTK.themeType === ApplicationHelper.LightType ? Qt.rgba(1, 1, 1, 0.9)
                                                                  : Qt.rgba(16.0/255.0, 16.0/255.0, 16.0/255.0, 0.85)
            Rectangle {
                width: 1
                height: parent.height
                anchors.right: parent.right
                color: DTK.themeType === ApplicationHelper.LightType ? "#eee7e7e7"
                                                                     : "#ee252525"
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 50
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.bottomMargin: 10

                FolderListView {
                    id: folderListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    onItemChanged: {
                        label.text = name
                        VNoteMainManager.vNoteFloderChanged(index)
                    }
                }
                Button {
                    id: createFolderButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: createFolderBtnHeight
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
            Layout.preferredWidth: 5
            Layout.fillHeight: true
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
                            leftBgArea.Layout.preferredWidth = newWidth;
                            rightBgArea.Layout.preferredWidth = rowLayout.width - leftBgArea.width - leftDragHandle.width;
                        }
                    }
                }
            }
        }
        Rectangle {
            id: middeleBgArea
            Layout.preferredWidth: leftViewWidth
            Layout.fillHeight: true
            color: Qt.rgba(0, 0, 0, 0.05)

            ColumnLayout {
                anchors.fill: parent
                Layout.topMargin: 7
                SearchEdit {
                    id: search
                    Layout.topMargin: 8
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    placeholder: qsTr("Search")
                    Keys.onPressed: {
                        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                            VNoteMainManager.vNoteSearch(text)
                        }
                    }
                    Connections {
                        function onClicked(mouse) {
                            search.focus = false
                            if (itemListView.searchLoader.active) {
                                itemListView.searchLoader.item.visible = false
                            }
                            itemListView.view.visible = true
                            label.visible = true
                            folderListView.opacity = 1
                            folderListView.enabled = true
                            createFolderButton.enabled = true
                            itemListView.isSearch = false
                        }
                        target: search.clearButton.item
                    }
                }
                Label {
                    id: label
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    font.pixelSize: 16
                    color: "#BB000000"
                    text: ""
                }
                ItemListView {
                    id: itemListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    onNoteItemChanged : {
                        VNoteMainManager.vNoteChanged(index)
                    }
                }
            }
        }
        Rectangle {
            id: rightDragHandle
            Layout.preferredWidth: 5
            Layout.fillHeight: true
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
                            middeleBgArea.Layout.preferredWidth = newWidth;
                            rightBgArea.Layout.preferredWidth = rowLayout.width - middeleBgArea.width - rightDragHandle.width;
                        }
                    }
                }
            }
        }
        Rectangle {
            id: rightBgArea
            Layout.fillWidth: true
            Layout.fillHeight: true
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
            ColumnLayout {
                anchors.fill: parent

                WebEngineView {
                    id: webEngineView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }

    Rectangle {
        id: initRect
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent

            InitialInterface {
                id: initiaInterface
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        Connections {
            onCreateFolder: {
                folderListView.addFolder()
                initRect.visible = false
            }

            target: initiaInterface
        }
    }
}
