// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.1
import VNote 1.0
import "../drag/"
import "../dialog/"
import org.deepin.dtk 1.0

Item {
    id: root

    property int currentDropIndex: -1
    property bool isPlay: false
    property int itemHeight: 30
    property int lastDropIndex: -1
    property int listHeight: 700
    property int listWidth: 200
    property alias model: folderListView.model

    signal emptyItemList(bool isEmpty)
    signal folderEmpty
    signal itemChanged(int index, string name)
    signal updateFolderName(string name)

    function addFolder() {
        VNoteMainManager.vNoteCreateFolder();
    }

    function addNote(size) {
        var cout = model.get(folderListView.currentIndex).count;
        model.get(folderListView.currentIndex).count = (Number(cout) + size).toString();
        root.emptyItemList(false);
    }

    function delNote(size) {
        var cout = model.get(folderListView.currentIndex).count;
        var new_cout = Number(cout) - size;
        root.emptyItemList(new_cout <= 0);
        model.get(folderListView.currentIndex).count = new_cout.toString();
    }

    function dropItems(selectedNoteItem) {
        if (currentDropIndex != -1) {
            VNoteMainManager.moveNotes(selectedNoteItem, currentDropIndex);
        }
        if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
            folderListView.itemAtIndex(lastDropIndex).isHovered = false;
        }
        lastDropIndex = -1;
        currentDropIndex = -1;
    }

    function getCurrentFolder() {
        return folderListView.model.get(folderListView.currentIndex);
    }

    function renameCurrentItem() {
        folderListView.currentItem.isRename = true;
    }

    function rollDown() {
        if (!scrollTimer.isUp && scrollTimer.running)
            return;
        if (folderListView.contentY + folderListView.height < folderListView.contentHeight) {
            scrollTimer.isUp = false;
            scrollTimer.running = true;
        }
    }

    function rollStop() {
        scrollTimer.running = false;
    }

    function rollUp() {
        if (scrollTimer.isUp && scrollTimer.running)
            return;
        if (folderListView.contentY > 0) {
            scrollTimer.isUp = true;
            scrollTimer.running = true;
        }
    }

    function toggleSearch(isSearch) {
        if (!isSearch) {
            var index = folderListView.currentIndex;
            itemChanged(index, folderModel.get(index).name); // 发出 itemChanged 信号
        }
    }

    function updateItems(mousePosX, mousePosY) {
        var pos = mapFromGlobal(mousePosX, mousePosY);
        if (pos.x < 0 || pos.x > listWidth) {
            if (currentDropIndex != -1)
                currentDropIndex = -1;
            if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
                folderListView.itemAtIndex(lastDropIndex).isHovered = false;
            }
            lastDropIndex = -1;
            return;
        }
        //判断当前鼠标所在的行
        var startY = pos.y - 50 > 0 ? pos.y - 50 : 0;
        var index = Math.floor((pos.y + (verticalScrollBar.position * folderModel.count * itemHeight)) / itemHeight);
        if (index < 0 || index >= folderModel.count) {
            if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
                folderListView.itemAtIndex(lastDropIndex).isHovered = false;
            }
            currentDropIndex = -1;
            lastDropIndex = -1;
            return;
        }
        currentDropIndex = index;
        if (index != lastDropIndex) {
            if (lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
                folderListView.itemAtIndex(lastDropIndex).isHovered = false;
            }
            lastDropIndex = index;
            if (index === folderListView.currentIndex) {
                return;
            }
        } else {
            return;
        }
        //更新当前行的颜色
        if (folderListView.itemAtIndex(index)) {
            folderListView.itemAtIndex(index).isHovered = true;
        }
    }

    height: listHeight
    visible: true
    width: listWidth

    Component.onCompleted: {
        root.forceActiveFocus();
    }
    Keys.onDeletePressed: {
        messageDialogLoader.showDialog(VNoteMessageDialogHandler.DeleteFolder, ret => {
            if (ret) {
                VNoteMainManager.vNoteDeleteFolder(folderListView.currentIndex);
                if (folderModel.count === 1)
                    folderEmpty();
                folderModel.remove(folderListView.currentIndex);
                if (folderListView.currentIndex === 0) {
                    folderListView.currentIndex = 0;
                }
            }
        });
    }

    ListModel {
        id: folderModel

    }

    DragControl {
        id: dragControl

    }

    VNoteMessageDialogLoader {
        id: messageDialogLoader

    }

    Timer {
        id: scrollTimer

        property bool isUp: true

        interval: 100
        repeat: true
        running: false

        onTriggered: {
            if (isUp) {
                if (folderListView.contentY <= 0) {
                    running = false;
                    folderListView.contentY = 0;
                    return;
                }
                folderListView.contentY -= 10;
            } else {
                if (folderListView.contentY + folderListView.height >= folderListView.contentHeight) {
                    running = false;
                    folderListView.contentY = folderListView.contentHeight - folderListView.height;
                    return;
                }
                folderListView.contentY += 10;
            }
        }
    }

    Connections {
        target: VNoteMainManager

        onAddFolderFinished: {
            folderListView.model.insert(0, {
                name: folderData.name,
                count: folderData.notesCount,
                icon: folderData.icon
            });
            folderListView.currentIndex = 0;
            folderListView.lastCurrentIndex = 0;
            VNoteMainManager.createNote();
            if (folderListView.itemAtIndex(folderListView.currentIndex + 1)) {
                folderListView.itemAtIndex(folderListView.currentIndex + 1).isHovered = false;
            }
            root.forceActiveFocus();
        }
    }

    Rectangle {
        id: dropLine

        color: "#0058DE"
        implicitHeight: 3
        implicitWidth: folderListView.width
        visible: false
    }

    ListView {
        id: folderListView

        property var contextIndex: -1
        property int dropIndex: -1
        property var lastCurrentIndex: -1

        function indexAt(mousePosX, mousePosY) {
            var pos = mapFromGlobal(mousePosX, mousePosY);
            var startY = itemHeight * 0.5;
            var index = Math.floor((pos.y - startY + (verticalScrollBar.position * folderModel.count * itemHeight)) / itemHeight) + 1;
            if (index < 0) {
                index = 0;
            }
            if (index >= folderModel.count) {
                index = folderModel.count;
            }
            dropIndex = index;
            dropLine.visible = true;
            if (folderListView.itemAtIndex(dropIndex)) {
                dropLine.y = folderListView.itemAtIndex(dropIndex).y;
            } else {
                dropLine.y = folderListView.itemAtIndex(dropIndex - 1).y + folderListView.itemAtIndex(dropIndex - 1).height;
            }
        }

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        enabled: parent.enabled
        model: folderModel

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar

        }
        delegate: Rectangle {
            id: rootItem

            property bool isHovered: false
            property bool isRename: false
            property var startMove: [-1, -1]
            property bool tooltipVisible: false

            color: index === folderListView.currentIndex ? (root.activeFocus ? "#1F6DE4" : DTK.themeType === ApplicationHelper.LightType ? "#33000000" : "#33FFFFFF") : (isHovered ? (DTK.themeType === ApplicationHelper.LightType ? "#1A000000" : "#1AFFFFFF") : "transparent")
            enabled: !isPlay || index === folderListView.currentIndex
            height: itemHeight
            radius: 6
            width: parent.width

            Keys.onPressed: {
                switch (event.key) {
                case Qt.Key_Enter:
                case Qt.Key_Return:
                    var newName = renameLine.text;
                    if (newName.length !== 0 && newName !== model.text) {
                        VNoteMainManager.renameFolder(index, newName);
                        model.name = newName;
                        updateFolderName(newName);
                    }
                    isRename = false;
                    root.forceActiveFocus();
                    break;
                case Qt.Key_Escape:
                    isRename = false;
                    root.forceActiveFocus();
                    break;
                default:
                    break;
                }
            }
            onIsRenameChanged: {
                renameLine.forceActiveFocus();
            }

            ToolTip {
                id: folderItemTip

                text: model.name
                visible: tooltipVisible
            }

            RowLayout {
                id: rowlayout

                property int imageWidth: 16

                Layout.fillWidth: true
                anchors.fill: parent
                spacing: 10

                Rectangle {
                    Layout.leftMargin: 10
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

                LineEdit {
                    id: renameLine

                    Layout.fillWidth: true
                    Layout.rightMargin: 5
                    bottomPadding: 0
                    implicitHeight: 24
                    text: model.name
                    topPadding: 0
                    visible: rootItem.isRename
                    z: 100

                    backgroundColor: Palette {
                        normal: Qt.rgba(1, 1, 1, 0.85)
                        normalDark: Qt.rgba(0, 0, 0, 0.5)
                    }

                    onActiveFocusChanged: {
                        folderMouseArea.enabled = false;
                        if (activeFocus) {
                            selectAll();
                        } else {
                            if (text.length !== 0 && text !== model.name) {
                                model.name = text;
                                VNoteMainManager.renameFolder(index, text);
                            } else {
                                renameLine.text = model.name;
                            }
                            folderMouseArea.enabled = true;
                            deselect();
                            rootItem.isRename = false;
                        }
                    }
                }

                Label {
                    id: folderNameLabel

                    Layout.fillWidth: true
                    color: index === folderListView.currentIndex ? (root.activeFocus ? "white" : (DTK.themeType === ApplicationHelper.LightType ? "black" : "#B2FFFFFF")) : (DTK.themeType === ApplicationHelper.LightType ? "black" : "#B2FFFFFF")
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    text: model.name
                    verticalAlignment: Text.AlignVCenter
                    visible: !rootItem.isRename
                }

                Label {
                    id: folderCountLabel

                    Layout.rightMargin: 10
                    color: folderNameLabel.color
                    horizontalAlignment: Text.AlignRight
                    text: model.count
                    verticalAlignment: Text.AlignVCenter
                    visible: !rootItem.isRename
                    width: 30
                }
            }

            MouseArea {
                id: folderMouseArea

                property bool held: false

                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                drag.target: this
                enabled: parent.enabled
                hoverEnabled: true

                onClicked: {
                    root.forceActiveFocus();
                    tooltipVisible = false;
                    if (folderListView.itemAtIndex(folderListView.lastCurrentIndex)) {
                        folderListView.itemAtIndex(folderListView.lastCurrentIndex).isRename = false;
                    }
                    folderListView.currentIndex = index;
                    folderListView.lastCurrentIndex = index;
                    if (mouse.button === Qt.RightButton) {
                        folderListView.contextIndex = index;
                        folderItemContextMenu.popup();
                    }
                    rootItem.isHovered = false;
                }
                onDoubleClicked: {
                    folderListView.itemAtIndex(folderListView.currentIndex).isRename = true;
                }
                onEntered: {
                    if (folderNameLabel.implicitWidth > folderNameLabel.width)
                        tooltipVisible = true;
                    if (folderListView.currentIndex == index) {
                        return;
                    }

                    parent.isHovered = true;
                }
                onExited: {
                    tooltipVisible = false;
                    if (folderListView.currentIndex == index) {
                        return;
                    }
                    parent.isHovered = false;
                }
                onPositionChanged: {
                    if (!held) {
                        if ((startMove[0] !== -1 || startMove[1] !== -1) && ((Math.abs(mouse.x - startMove[0]) > 5) || (Math.abs(mouse.y - startMove[1]) > 5))) {
                            dragControl.isFolder = true;
                            held = true;
                        } else {
                            return;
                        }
                    }
                    if (held && folderModel.get(folderListView.currentIndex)) {
                        if (dragControl.imageSource === "") {
                            parent.grabToImage(function (result) {
                                dragControl.imageSource = result.url;
                            });
                        }
                        var globPos = mapToGlobal(mouse.x, mouse.y);
                        dragControl.itemNumber = 1;
                        dragControl.visible = true;
                        dragControl.x = globPos.x;
                        dragControl.y = globPos.y;
                        folderListView.indexAt(globPos.x, globPos.y);
                    } else {
                        dragControl.visible = false;
                    }
                }
                onPressed: {
                    startMove[0] = mouse.x;
                    startMove[1] = mouse.y;
                }
                onReleased: {
                    startMove = [-1, -1];
                    if (held) {
                        held = false;
                        dropLine.visible = false;
                        dragControl.visible = false;
                        dragControl.imageSource = "";
                        if (folderListView.dropIndex != -1) {
                            if (folderListView.dropIndex > index) {
                                folderListView.dropIndex -= 1;
                            }
                            if (folderListView.dropIndex != index) {
                                var tmpIndex = index;
                                folderModel.move(index, folderListView.dropIndex, 1);
                                VNoteMainManager.updateSort(tmpIndex, folderListView.dropIndex);
                            }
                        }
                    }
                }
            }

            Menu {
                id: folderItemContextMenu

                MenuItem {
                    text: qsTr("Rename")

                    onTriggered: {
                        folderListView.itemAtIndex(folderListView.currentIndex).isRename = true;
                    }
                }

                MenuItem {
                    enabled: !isPlay
                    text: qsTr("Delete")

                    onTriggered: {
                        messageDialogLoader.showDialog(VNoteMessageDialogHandler.DeleteFolder, ret => {
                            if (ret) {
                                VNoteMainManager.vNoteDeleteFolder(folderListView.contextIndex);
                                if (folderModel.count === 1)
                                    folderEmpty();
                                folderModel.remove(folderListView.contextIndex);
                                if (folderListView.contextIndex === 0) {
                                    folderListView.currentIndex = 0;
                                }
                            }
                        });
                    }
                }

                MenuItem {
                    enabled: !isPlay
                    text: qsTr("New Note")

                    onTriggered: {
                        VNoteMainManager.createNote();
                    }
                }
            }
        }

        onCurrentItemChanged: {
            var index = folderListView.currentIndex;
            itemChanged(index, folderModel.get(index).name); // 发出 itemChanged 信号
        }

        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true

            onPressed: {
                var index = folderListView.currentIndex;
                var item = folderListView.itemAtIndex(index);
                if (item.isRename) {
                    var clickX = mouse.x;
                    var clickY = mouse.y;
                    var localPoint = item.mapFromItem(parent, clickX, clickY);
                    if (!item.contains(localPoint)) {
                        root.forceActiveFocus();
                    }
                } else {
                    root.forceActiveFocus();
                }

                mouse.accepted = false;
            }
        }
    }
}
