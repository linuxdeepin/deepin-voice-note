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
    width: 640
    height: 480
    visible: true

    property var selectedNoteItem: []
    property int itemSpacing: 6
    property int selectSize: 1
    property int topSize: 0
    property bool isSearch: false
    property alias model: itemModel
    property alias searchLoader: noSearchResultsLoader
    property alias view: itemListView
    property alias moveToFolderDialog: dialogWindow
    property var saveFilters: ["TXT(*.txt);;HTML(*.html)", "TXT(*.txt)", "HTML(*.html)", "MP3(*.mp3)"]
    property int selectIndex: 0

    signal noteItemChanged(int index)
    signal mouseChanged(int mousePosX, int mousePosY)
    signal dropRelease()
    signal mulChoices(int choices)
    signal deleteNotes(int number)

    ListModel {
        id: itemModel
    }

    MoveDialog {
        id: dialogWindow

        onMoveToFolder: {
            VNoteMainManager.moveNotes(selectedNoteItem, index)
        }
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
            // currentFile: Url.fromLocalFile(selectedNoteItem[0].name)
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

    Loader {
        id: noSearchResultsLoader
        active: false
        asynchronous: true
        sourceComponent: Rectangle {
            id: noSearchResultsRect
            width: itemListView.width
            height: itemListView.height
            color: "transparent"
            visible: false
            Component.onCompleted: {
                noSearchResultsRect.visible = true
                itemListView.visible = false
            }
            Text {
                anchors.centerIn: parent
                text: qsTr("No search results")
                font.pixelSize: 14
                color: "#B3000000"
            }
        }
    }

    Connections {
        function handleUpdateNoteList(notesData) {
            itemModel.clear()
            for (var i = 0; i < notesData.length; i++) {
                var itemIsTop = notesData[i].isTop ? "top" : "normal"
                itemModel.append({name: notesData[i].name, time: notesData[i].time, isTop: itemIsTop,
                                 icon: notesData[i].icon, folderName: notesData[i].folderName, noteId: notesData[i].noteId})
            }
        }
        target: VNoteMainManager
        onNoSearchResult: {
            if (!noSearchResultsLoader.active) {
                noSearchResultsLoader.active = true
            } else {
                noSearchResultsLoader.item.visible = true
                itemListView.visible = false
            }
        }
        onSearchFinished: {
            if (noSearchResultsLoader.active && noSearchResultsLoader.item.visible) {
                noSearchResultsLoader.item.visible = false
                itemListView.visible = true
            }

            isSearch = true
            handleUpdateNoteList(notesData)
        }
    }

    ListView {
        id: itemListView
        anchors.fill: parent
        model: itemModel
        property var lastCurrentIndex: 0
        property var contextIndex: -1
        spacing: itemSpacing
        visible: true
        delegate: Rectangle {
            property bool isRename: false
            width: parent.width
            height: isSearch ? 67 : 50
            property alias renameFocus: renameLine.focus
            property bool isSelected: selectIndex === index
            
            color: isSelected ? "#FF1F6EE7" : "white"
            radius: 6

            ColumnLayout {
                id: itemLayout
                anchors.fill: parent
                anchors.topMargin: 8
                anchors.bottomMargin: 8
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: isRename ? 7 : 1
                Label {
                    id: noteNameLabel
                    width: parent.width
                    height: 18
                    visible: !isRename
                    horizontalAlignment: Text.AlignHLeft
                    text: model.name
                    color: isSelected ? "white" : "black"
                    font.pixelSize: 14
                }
                Label {
                    id: timeLabel
                    width: parent.width
                    height: 15
                    visible: !isRename
                    horizontalAlignment: Text.AlignHLeft
                    text: model.time
                    color: isSelected ? "#7FFFFFFF" : "#7F000000"
                    font.pixelSize: 10
                }
                LineEdit {
                    id: renameLine
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.topMargin: isRename ? 4 : 0
                    visible: isRename
                    text: model.name
                    onFocusChanged: {
                        noteItemMouseArea.enabled = false;
                        if (focus) {
                            focus = true
                            selectAll();
                        } else {
                            noteItemMouseArea.enabled = true;
                            deselect();
                            visible = false
                            noteNameLabel.visible = true
                            timeLabel.visible = true
                        }
                    }
                }
                RowLayout {
                    id: rowlayout
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    visible: isSearch
                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
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
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: model.folderName
                        color: isSelected ? "#7FFFFFFF" : "#7F000000"
                        font.pixelSize: 10
                    }
                }
            }
            MouseArea {
                id : noteItemMouseArea
                anchors.fill: parent
                drag.target: this
                property bool held: false
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: {
                    selectIndex = -1
                    if (mouse.button === Qt.RightButton) {
                        itemListView.contextIndex = index
                        if (itemListView.itemAtIndex(itemListView.lastCurrentIndex)) {
                            var lastItem = itemListView.itemAtIndex(itemListView.lastCurrentIndex)
                            lastItem.isRename = false
                            lastItem.isSelected = false
                        }
                        for (var j = 0; j < selectedNoteItem.length; j++) {
                            var item = itemListView.itemAtIndex(selectedNoteItem[j])
                            item.isSelected = false
                        }
                        selectedNoteItem = []
                        selectedNoteItem.push(index)
                        itemListView.itemAtIndex(index).isSelected = true
                        selectSize = 1
                        itemListView.currentIndex = index
                        itemListView.lastCurrentIndex = index
                        noteContextMenu.popup()
                    } else {
                        if (itemListView.currentIndex === index && selectedNoteItem.length === 1) {
                            return;
                        }
                        switch (mouse.modifiers) {
                            case Qt.ControlModifier:
                                if (selectedNoteItem.indexOf(index) != -1) {
                                    itemListView.itemAtIndex(index).isSelected = false
                                    selectedNoteItem.splice(selectedNoteItem.indexOf(index), 1)
                                    selectSize = selectSize - 1
                                } else {
                                    itemListView.itemAtIndex(index).isSelected = true
                                    selectedNoteItem.push(index)
                                    selectSize = selectSize + 1
                                }
                            break;
                            case Qt.ShiftModifier:
                                var startIndex = Math.min(index, itemListView.lastCurrentIndex)
                                var endIndex = Math.max(index, itemListView.lastCurrentIndex)
                                for (var i = startIndex; i <= endIndex; i++) {
                                    itemListView.itemAtIndex(i).isSelected = true
                                    selectedNoteItem.push(i)
                                    selectSize = selectSize + 1
                                }
                            break;
                            default:
                                if (itemListView.itemAtIndex(itemListView.lastCurrentIndex)) {
                                    var lastSelectItem = itemListView.itemAtIndex(itemListView.lastCurrentIndex)
                                    lastSelectItem.isRename = false
                                    lastSelectItem.isSelected = false
                                }
                                for (var m = 0; m < selectedNoteItem.length; m++) {
                                    var selectItem = itemListView.itemAtIndex(selectedNoteItem[m])
                                    selectItem.isSelected = false
                                }
                                selectedNoteItem = []
                                selectedNoteItem.push(index)
                                itemListView.itemAtIndex(index).isSelected = true
                                selectSize = 1
                                itemListView.currentIndex = index
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
                        selectSize = 1
                        itemListView.lastCurrentIndex = -1
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
                    visible: section == "top" && !isSearch
                    anchors.left: parent.left
                    text: qsTr("Sticky Notes")
                    font.pixelSize: 12
                    color: "#b3000000"
                }
                Rectangle {
                    visible: section != "top" && VNoteMainManager.getTop() && !isSearch
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
                    var currentItem = itemListView.itemAtIndex(itemListView.contextIndex)
                    console.log("Current item:", currentItem)
                    if (currentItem) {
                        currentItem.isRename = true
                        currentItem.renameFocus = true
                    }
                }
            }
            MenuItem {
                text: itemModel.get(itemListView.contextIndex).isTop === "top" ? qsTr("untop") : qsTr("Top")
                onTriggered: {
                    var setTop = true;
                    if (itemModel.get(itemListView.contextIndex).isTop === "top") {
                        setTop = false;
                    }
                    VNoteMainManager.updateTop(itemModel.get(itemListView.contextIndex).noteId, setTop)
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
                    if (selectedNoteItem.length > 1) {
                        var delList = sortAndDeduplicate(selectedNoteItem)
                        //TODO: 弹出对话框
                        for (var i = 0; i < delList.length; i++)
                            itemModel.remove(delList[i])
                        VNoteMainManager.deleteNote(delList)
                        deleteNotes(selectedNoteItem.length)
                    } else {
                        itemModel.remove(itemListView.contextIndex)
                        var indexList = [itemListView.contextIndex]
                        VNoteMainManager.deleteNote(indexList)
                        deleteNotes(1)
                        if (itemModel.count <= itemListView.contextIndex) {
                            itemListView.itemAtIndex(itemModel.count-1).isSelected = true
                            selectedNoteItem = [itemModel.count-1]
                            selectSize = 1
                        } else {
                            itemListView.itemAtIndex(itemListView.contextIndex).isSelected = true
                        }
                    }
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
        function sortAndDeduplicate(arr) {
            // Convert to integers and remove duplicates using a Set
            var uniqueInts = [...new Set(arr)].map(Number);

            // Sort the array in descending order
            uniqueInts.sort(function(a, b) { return b - a; });

            return uniqueInts;
        }
        onCurrentItemChanged: {
            var index = itemListView.currentIndex
            noteItemChanged(index)
        }
    }

    onSelectSizeChanged: {
        mulChoices(selectSize)
    }
}
