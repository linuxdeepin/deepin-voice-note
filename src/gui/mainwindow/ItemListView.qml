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
    property bool isSearch: false
    property int itemSpacing: 6
    property alias model: itemModel
    property alias moveToFolderDialog: dialogWindow
    property var saveFilters: ["TXT(*.txt);;HTML(*.html)", "TXT(*.txt)", "HTML(*.html)", "MP3(*.mp3)"]
    property alias searchLoader: noSearchResultsLoader
    property int selectSize: 0
    property var selectedNoteItem: []
    property int topSize: 0
    property alias view: itemListView

    signal deleteNotes(int number)
    signal dropRelease
    signal mouseChanged(int mousePosX, int mousePosY)
    signal mulChoices(int choices)
    signal noteItemChanged(int index)

    height: 480
    visible: true
    width: 640

    onSelectSizeChanged: {
        mulChoices(selectSize);
    }

    ListModel {
        id: itemModel

    }

    MoveDialog {
        id: dialogWindow

        onMoveToFolder: {
            VNoteMainManager.moveNotes(selectedNoteItem, index);
        }
    }

    VNoteMessageDialogLoader {
        id: messageDialogLoader

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

            currentFile: "file:///" + itemModel.get(selectedNoteItem[0]).name
            currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
            fileMode: FileDialog.SaveFile
            nameFilters: saveFilters[saveType]
            title: qsTr("Save As")

            Component.onCompleted: {
                fileDialog.open();
            }
            onAccepted: {
                var idList = [];
                for (var i = 0; i < selectedNoteItem.length; i++) {
                    idList.push(itemModel.get(selectedNoteItem[0]).noteId);
                }
                VNoteMainManager.saveAs(idList, fileDialog.selectedFile, saveType);
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

            currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
            title: qsTr("Save As")

            Component.onCompleted: {
                folderDialog.open();
            }
            onAccepted: {
                VNoteMainManager.saveAs(selectedNoteItem, folderDialog.selectedFolder, saveType);
            }
        }
    }

    Loader {
        id: noSearchResultsLoader

        active: false
        asynchronous: true

        sourceComponent: Rectangle {
            id: noSearchResultsRect

            color: "transparent"
            height: itemListView.height
            visible: false
            width: itemListView.width

            Component.onCompleted: {
                noSearchResultsRect.visible = true;
                itemListView.visible = false;
            }

            Text {
                anchors.centerIn: parent
                color: "#B3000000"
                font.pixelSize: 14
                text: qsTr("No search results")
            }
        }
    }

    Connections {
        function handleUpdateNoteList(notesData) {
            itemModel.clear();
            for (var i = 0; i < notesData.length; i++) {
                var itemIsTop = notesData[i].isTop ? "top" : "normal";
                itemModel.append({
                        name: notesData[i].name,
                        time: notesData[i].time,
                        isTop: itemIsTop,
                        icon: notesData[i].icon,
                        folderName: notesData[i].folderName,
                        noteId: notesData[i].noteId
                    });
            }
        }

        target: VNoteMainManager

        onNoSearchResult: {
            if (!noSearchResultsLoader.active) {
                noSearchResultsLoader.active = true;
            } else {
                noSearchResultsLoader.item.visible = true;
                itemListView.visible = false;
            }
        }
        onSearchFinished: {
            if (noSearchResultsLoader.active && noSearchResultsLoader.item.visible) {
                noSearchResultsLoader.item.visible = false;
                itemListView.visible = true;
            }
            isSearch = true;
            handleUpdateNoteList(notesData);
        }
    }

    ListView {
        id: itemListView

        property var contextIndex: -1
        property var lastCurrentIndex: 0

        function sortAndDeduplicate(arr) {
            // Convert to integers and remove duplicates using a Set
            var uniqueInts = [...new Set(arr)].map(Number);

            // Sort the array in descending order
            uniqueInts.sort(function (a, b) {
                    return b - a;
                });
            return uniqueInts;
        }

        anchors.fill: parent
        model: itemModel
        spacing: itemSpacing
        visible: true

        delegate: Rectangle {
            property bool isRename: false
            property bool isSelected: false
            property alias renameFocus: renameLine.focus

            color: isSelected ? "#FF1F6EE7" : "white"
            height: isSearch ? 67 : 50
            radius: 6
            width: itemListView.width

            Component.onCompleted: {
                // console.warn("--- create view item", this, model.name, isSelected);
                isSelected = (selectedNoteItem.indexOf(index) !== -1);
            }

            // onIsSelectedChanged: {
            //     console.warn("---", this, model.name, isSelected);
            // }

            ColumnLayout {
                id: itemLayout

                anchors.bottomMargin: 8
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 8
                spacing: isRename ? 7 : 1

                Label {
                    id: noteNameLabel

                    color: isSelected ? "white" : "black"
                    font.pixelSize: 14
                    height: 18
                    horizontalAlignment: Text.AlignHLeft
                    text: model.name
                    visible: !isRename
                    width: parent.width
                }

                Label {
                    id: timeLabel

                    color: isSelected ? "#7FFFFFFF" : "#7F000000"
                    font.pixelSize: 10
                    height: 15
                    horizontalAlignment: Text.AlignHLeft
                    text: model.time
                    visible: !isRename
                    width: parent.width
                }

                LineEdit {
                    id: renameLine

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.topMargin: isRename ? 4 : 0
                    text: model.name
                    visible: isRename

                    onFocusChanged: {
                        noteItemMouseArea.enabled = false;
                        if (focus) {
                            focus = true;
                            selectAll();
                        } else {
                            noteItemMouseArea.enabled = true;
                            deselect();
                            visible = false;
                            noteNameLabel.visible = true;
                            timeLabel.visible = true;
                        }
                    }
                }

                RowLayout {
                    id: rowlayout

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    visible: isSearch

                    Rectangle {
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

                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: isSelected ? "#7FFFFFFF" : "#7F000000"
                        font.pixelSize: 10
                        horizontalAlignment: Text.AlignLeft
                        text: model.folderName
                    }
                }
            }

            MouseArea {
                id: noteItemMouseArea

                property bool held: false

                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                drag.target: this

                onClicked: {
                    if (mouse.button === Qt.RightButton) {
                        itemListView.contextIndex = index;
                        if (itemListView.itemAtIndex(itemListView.lastCurrentIndex)) {
                            var lastItem = itemListView.itemAtIndex(itemListView.lastCurrentIndex);
                            lastItem.isRename = false;
                            lastItem.isSelected = false;
                        }
                        for (var j = 0; j < selectedNoteItem.length; j++) {
                            var item = itemListView.itemAtIndex(selectedNoteItem[j]);
                            item.isSelected = false;
                        }
                        selectedNoteItem = [index];
                        selectSize = 1;
                        itemListView.itemAtIndex(index).isSelected = true;
                        itemListView.currentIndex = index;
                        itemListView.lastCurrentIndex = index;
                        noteItemChanged(Number(itemModel.get(index).noteId));
                        noteContextMenu.popup();
                    } else {
                        if (itemListView.currentIndex === index && selectedNoteItem.length === 1) {
                            return;
                        }
                        switch (mouse.modifiers) {
                        case Qt.ControlModifier:
                            if (selectedNoteItem.indexOf(index) != -1) {
                                itemListView.itemAtIndex(index).isSelected = false;
                                selectedNoteItem.splice(selectedNoteItem.indexOf(index), 1);
                                selectSize--;
                            } else {
                                itemListView.itemAtIndex(index).isSelected = true;
                                selectedNoteItem.push(index);
                                selectSize++;
                            }
                            break;
                        case Qt.ShiftModifier:
                            var startIndex = Math.min(index, itemListView.lastCurrentIndex);
                            var endIndex = Math.max(index, itemListView.lastCurrentIndex);
                            for (var i = startIndex; i <= endIndex; i++) {
                                itemListView.itemAtIndex(i).isSelected = true;
                                selectedNoteItem.push(i);
                                selectSize++;
                            }
                            break;
                        default:
                            if (itemListView.itemAtIndex(itemListView.lastCurrentIndex)) {
                                var lastSelectItem = itemListView.itemAtIndex(itemListView.lastCurrentIndex);
                                lastSelectItem.isRename = false;
                                lastSelectItem.isSelected = false;
                            }
                            for (var m = 0; m < selectedNoteItem.length; m++) {
                                var selectItem = itemListView.itemAtIndex(selectedNoteItem[m]);
                                selectItem.isSelected = false;
                            }
                            selectedNoteItem = [index];
                            selectSize = 1;
                            itemListView.itemAtIndex(index).isSelected = true;
                            itemListView.currentIndex = index;
                            itemListView.lastCurrentIndex = index;
                            noteItemChanged(Number(itemModel.get(index).noteId));
                            break;
                        }
                    }
                }
                onPositionChanged: {
                    if (held && itemModel.get(selectedNoteItem[0])) {
                        var globPos = mapToGlobal(mouse.x, mouse.y);
                        dragControl.itemNumber = selectedNoteItem.length;
                        dragControl.itemText = itemModel.get(selectedNoteItem[0]).name;
                        dragControl.visible = true;
                        dragControl.x = globPos.x;
                        dragControl.y = globPos.y;
                        mouseChanged(globPos.x, globPos.y);
                    } else {
                        dragControl.visible = false;
                    }
                }
                onPressed: {
                    held = true;
                }
                onReleased: {
                    if (held && dragControl.visible && mouse.button === Qt.LeftButton) {
                        if (mouse.x < 0 || mouse.x > parent.width || mouse.y < 0 || mouse.y > parent.height) {
                            held = false;
                            dragControl.visible = false;
                            return;
                        }
                        dropRelease();
                        //从model中删除选中的item
                        for (var i = 0; i < selectedNoteItem.length; i++) {
                            var item = itemModel.get(selectedNoteItem[i]);
                            itemModel.remove(selectedNoteItem[i]);
                        }
                        selectedNoteItem = [];
                        selectSize = 0;
                        itemListView.lastCurrentIndex = -1;
                    }
                    held = false;
                    dragControl.visible = false;
                }
            }
        }

        section {
            criteria: ViewSection.FullString
            property: "isTop"

            delegate: Rectangle {
                color: "transparent"
                height: section == "top" ? 16 : 10
                width: parent.width

                Text {
                    anchors.left: parent.left
                    color: "#b3000000"
                    font.pixelSize: 12
                    text: qsTr("Sticky Notes")
                    visible: section == "top" && !isSearch
                }

                Rectangle {
                    anchors.centerIn: parent
                    color: "#0D000000"
                    height: 1
                    visible: section != "top" && VNoteMainManager.getTop() && !isSearch
                    width: parent.width
                }
            }
        }

        Menu {
            id: noteContextMenu

            MenuItem {
                text: qsTr("Rename")

                onTriggered: {
                    var currentItem = itemListView.itemAtIndex(itemListView.contextIndex);
                    console.log("Current item:", currentItem);
                    if (currentItem) {
                        currentItem.isRename = true;
                        currentItem.renameFocus = true;
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
                    VNoteMainManager.updateTop(itemModel.get(itemListView.contextIndex).noteId, setTop);
                }
            }

            MenuItem {
                text: qsTr("Move")

                onTriggered: {
                    dialogWindow.show();
                }
            }

            MenuItem {
                text: qsTr("Delete")

                onTriggered: {
                    messageDialogLoader.messageData = selectedNoteItem.length;
                    messageDialogLoader.showDialog(VNoteMessageDialogHandler.DeleteNote, ret => {
                            if (!ret) {
                                return;
                            }
                            var delList = itemListView.sortAndDeduplicate(selectedNoteItem);
                            var delIdList = [];
                            for (var i = 0; i < delList.length; i++) {
                                delIdList.push(itemModel.get(delList[i]).noteId);
                                itemModel.remove(delList[i]);
                            }
                            VNoteMainManager.deleteNote(delIdList);
                            deleteNotes(selectedNoteItem.length);
                            if (itemModel.count <= itemListView.contextIndex) {
                                itemListView.itemAtIndex(itemModel.count - 1).isSelected = true;
                                selectedNoteItem = [itemModel.count - 1];
                                noteItemChanged(itemModel.get(itemModel.count - 1).noteId);
                            } else {
                                itemListView.itemAtIndex(itemListView.contextIndex).isSelected = true;
                                noteItemChanged(itemModel.get(itemListView.contextIndex).noteId);
                            }
                        });
                }
            }

            Menu {
                id: saveNoteSubMenu

                title: qsTr("Save note")

                MenuItem {
                    text: qsTr("Save as HTML")

                    onTriggered: {
                        fileDialogLoader.saveType = VNoteMainManager.Html;
                        if (!fileDialogLoader.active) {
                            fileDialogLoader.active = true;
                        } else {
                            fileDialogLoader.item.open();
                        }
                    }
                }

                MenuItem {
                    text: qsTr("Save as TXT")

                    onTriggered: {
                        fileDialogLoader.saveType = VNoteMainManager.Text;
                        if (!fileDialogLoader.active) {
                            fileDialogLoader.active = true;
                        } else {
                            fileDialogLoader.item.open();
                        }
                    }
                }
            }

            MenuItem {
                text: qsTr("Save recordings")

                onTriggered: {
                    if (!folderDialogLoader.active) {
                        folderDialogLoader.saveType = VNoteMainManager.Voice;
                        folderDialogLoader.active = true;
                    } else {
                        folderDialogLoader.item.open();
                    }
                }
            }

            MenuSeparator {
            }

            MenuItem {
                text: qsTr("New note")

                onTriggered: {
                    VNoteMainManager.createNote();
                }
            }
        }
    }
}
