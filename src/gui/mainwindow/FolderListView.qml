// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
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
    property bool isDragging: false
    property bool isPlay: false
    property bool isRecordingAudio: false
    property bool isVoiceToText: false
    property int itemHeight: 30
    property int lastDropIndex: -1
    property int listHeight: 700
    property int listWidth: 200
    property alias model: folderListView.model
    property alias currentFolderIndex: folderListView.currentIndex
    property Item scrollBarParent: null
    property int scrollBarRightOffset: 0
    property bool webVisible: true

    signal emptyItemList(bool isEmpty)
    signal folderEmpty
    signal itemChanged(int index, string name)
    signal mouseChanged(int mousePosX, int mousePosY)
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
        if (isRecordingAudio) {
            console.log("FolderListView: dropItems ignored while recording");
        } else if (isPlay) {
            console.log("FolderListView: dropItems ignored while playing");
        } else if (isVoiceToText) {
            console.log("FolderListView: dropItems ignored while voice to text is in progress");
        } else if (currentDropIndex != -1) {
            VNoteMainManager.moveNotes(selectedNoteItem, currentDropIndex);
        }
        if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
            folderListView.itemAtIndex(lastDropIndex).isHovered = false;
        }
        lastDropIndex = -1;
        currentDropIndex = -1;
        folderListView.hoverIndex = -1;
        hideScrollHintLine();
    }

    function getCurrentFolder() {
        return folderListView.model.get(folderListView.currentIndex);
    }

    function renameCurrentItem() {
        folderListView.currentItem.isRename = true;
    }

    function cancelRename() {
        for (var i = 0; i < folderListView.count; i++) {
            var item = folderListView.itemAtIndex(i);
            if (item && item.isRename) {
                item.isRename = false;
            }
        }
    }

    function showContextMenuOnCurrentItem() {
        // 在当前选中项位置弹出文件夹右键菜单
        if (folderListView.currentIndex < 0 || folderListView.currentIndex >= folderListView.count)
            return;
        var item = folderListView.itemAtIndex(folderListView.currentIndex);
        if (!item)
            return;
        folderListView.contextIndex = folderListView.currentIndex;
        item.openContextMenuAt(item.width / 2, item.height);
    }

    function rollDown() {
        if (!scrollTimer.isUp && scrollTimer.running)
            return;
        if (folderListView.contentY < folderListView.bottomContentY()) {
            scrollTimer.isUp = false;
            showScrollHintLine(false);
            scrollTimer.running = true;
        } else {
            hideScrollHintLine();
        }
    }

    function rollStop() {
        scrollTimer.running = false;
        hideScrollHintLine();
    }

    function rollUp() {
        if (scrollTimer.isUp && scrollTimer.running)
            return;
        if (folderListView.contentY > folderListView.topContentY()) {
            scrollTimer.isUp = true;
            showScrollHintLine(true);
            scrollTimer.running = true;
        } else {
            hideScrollHintLine();
        }
    }

    function showScrollHintLine(isUp) {
        scrollHintLine.y = isUp ? 0 : Math.max(0, height - scrollHintLine.implicitHeight);
        scrollHintLine.visible = true;
    }

    function hideScrollHintLine() {
        scrollHintLine.visible = false;
    }

    function toggleSearch(isSearch) {
        if (!isSearch) {
            var index = folderListView.currentIndex;
            itemChanged(index, folderModel.get(index).name); // 发出 itemChanged 信号
        }
    }

    function updateItems(mousePosX, mousePosY) {
        folderListView.lastDragMouseX = mousePosX;
        folderListView.lastDragMouseY = mousePosY;
        folderListView.sortingFolder = false;
        var pos = mapFromGlobal(mousePosX, mousePosY);
        if (pos.x < 0 || pos.x > listWidth) {
            if (currentDropIndex != -1)
                currentDropIndex = -1;
            folderListView.hoverIndex = -1;
            if (lastDropIndex != -1 && lastDropIndex != folderListView.currentIndex && folderListView.itemAtIndex(lastDropIndex)) {
                folderListView.itemAtIndex(lastDropIndex).isHovered = false;
            }
            lastDropIndex = -1;
            return;
        }
        // 判断当前鼠标所在的行，使用真实内容偏移避免自动滚动时目标行错位
        var index = Math.floor((pos.y + folderListView.contentY - folderListView.topContentY()) / itemHeight);
        if (index < 0 || index >= folderModel.count) {
            folderListView.hoverIndex = -1;
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
                folderListView.hoverIndex = -1;
                return;
            }
        } else {
            return;
        }
        //更新当前行的颜色
        folderListView.hoverIndex = index;
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
    Keys.onPressed: function(event) {
        switch (event.key) {
        case Qt.Key_Up:
            if (folderListView.count > 0) {
                // 循环切换：如果在第一项，跳到最后一项；否则向上移动
                if (folderListView.currentIndex <= 0) {
                    folderListView.currentIndex = folderListView.count - 1;
                } else {
                    folderListView.currentIndex--;
                }
            }
            event.accepted = true;
            break;
        case Qt.Key_Down:
            if (folderListView.count > 0) {
                // 循环切换：如果在最后一项，跳到第一项；否则向下移动
                if (folderListView.currentIndex >= folderListView.count - 1) {
                    folderListView.currentIndex = 0;
                } else {
                    folderListView.currentIndex++;
                }
            }
            event.accepted = true;
            break;
        case Qt.Key_Delete:
            if (root.isDragging || webVisible || isRecordingAudio || isPlay || isVoiceToText) {
                console.log("No notes available, cannot delete folder");
                return;
            }
            
            messageDialogLoader.showDialog(VNoteMessageDialogHandler.DeleteFolder, ret => {
                if (ret) {
                    if (!VNoteMainManager.vNoteDeleteFolder(folderListView.currentIndex))
                        return;
                    if (folderModel.count === 1)
                        folderEmpty();
                    folderModel.remove(folderListView.currentIndex);
                    if (folderListView.currentIndex === 0) {
                        folderListView.currentIndex = 0;
                    }
                }
            });
            event.accepted = true;
            break;
        default:
            break;
        }
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
                var topY = folderListView.topContentY();
                if (folderListView.contentY <= topY) {
                    running = false;
                    folderListView.contentY = topY;
                    hideScrollHintLine();
                    folderListView.refreshDragTargetAfterScroll();
                    return;
                }
                folderListView.contentY = Math.max(topY, folderListView.contentY - 10);
                folderListView.refreshDragTargetAfterScroll();
            } else {
                var bottomY = folderListView.bottomContentY();
                if (folderListView.contentY >= bottomY) {
                    running = false;
                    folderListView.contentY = bottomY;
                    hideScrollHintLine();
                    folderListView.refreshDragTargetAfterScroll();
                    return;
                }
                folderListView.contentY = Math.min(bottomY, folderListView.contentY + 10);
                folderListView.refreshDragTargetAfterScroll();
            }
        }
    }

    Connections {
        target: VNoteMainManager

        onAddFolderFinished: {
            folderListView.model.insert(0, {
                name: folderData.name,
                count: folderData.notesCount,
                icon: folderData.icon,
                folderId: folderData.folderId
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
        z: 2
    }

    Rectangle {
        id: scrollHintLine

        color: DTK.themeType === ApplicationHelper.LightType ? "#66000000" : "#66FFFFFF"
        implicitHeight: 3
        implicitWidth: folderListView.width
        visible: false
        z: 2
    }

    ListView {
        id: folderListView

        property var contextIndex: -1
        property int dropIndex: -1
        property int hoverIndex: -1
        property int lastDragMouseX: -1
        property int lastDragMouseY: -1
        property var lastCurrentIndex: -1
        property var dragSourceFolderId: -1
        property bool sortingFolder: false

        function topContentY() {
            return originY;
        }

        function bottomContentY() {
            return Math.max(topContentY(), originY + contentHeight - height);
        }

        function indexOfFolderId(folderId) {
            for (var i = 0; i < folderModel.count; i++) {
                if (folderModel.get(i).folderId === folderId)
                    return i;
            }
            return -1;
        }

        function clearDragState() {
            scrollTimer.running = false;
            dropLine.visible = false;
            dropIndex = -1;
            currentDropIndex = -1;
            hoverIndex = -1;
            lastDragMouseX = -1;
            lastDragMouseY = -1;
            dragSourceFolderId = -1;
            sortingFolder = false;
            if (lastDropIndex !== -1) {
                var item = itemAtIndex(lastDropIndex);
                if (item)
                    item.isHovered = false;
            }
            lastDropIndex = -1;
            dragControl.visible = false;
            dragControl.imageSource = "";
            hideScrollHintLine();
        }

        function refreshDragTargetAfterScroll() {
            if (lastDragMouseX < 0 || lastDragMouseY < 0)
                return;

            if (sortingFolder) {
                indexAt(lastDragMouseX, lastDragMouseY);
            } else if (currentDropIndex !== -1 || lastDropIndex !== -1) {
                root.updateItems(lastDragMouseX, lastDragMouseY);
            }
        }

        function updateDropLinePosition(lineY) {
            var maxY = Math.max(0, height - dropLine.implicitHeight);
            dropLine.y = Math.max(0, Math.min(lineY, maxY));
            dropLine.visible = true;
        }

        function indexAt(mousePosX, mousePosY) {
            lastDragMouseX = mousePosX;
            lastDragMouseY = mousePosY;
            sortingFolder = true;
            if (folderModel.count === 0) {
                dropIndex = -1;
                dropLine.visible = false;
                return;
            }
            var pos = mapFromGlobal(mousePosX, mousePosY);
            var startY = itemHeight * 0.5;
            var index = Math.floor((pos.y - startY + contentY - topContentY()) / itemHeight) + 1;
            if (index < 0) {
                index = 0;
            }
            if (index >= folderModel.count) {
                index = folderModel.count;
            }
            dropIndex = index;
            updateDropLinePosition(topContentY() + index * itemHeight - contentY);
        }

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        enabled: parent.enabled
        model: folderModel

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar

            function updateGeometry() {
                if (!parent)
                    return;

                x = root.mapToItem(parent, root.width + root.scrollBarRightOffset - width, 0).x;
                y = root.scrollBarParent ? 0 : root.mapToItem(parent, 0, 0).y;
                height = root.scrollBarParent ? parent.height : folderListView.height;
            }

            parent: root.scrollBarParent ? root.scrollBarParent : folderListView

            Component.onCompleted: Qt.callLater(updateGeometry)
            onParentChanged: Qt.callLater(updateGeometry)
            onWidthChanged: Qt.callLater(updateGeometry)

            Connections {
                target: root

                function onHeightChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
                function onWidthChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
                function onXChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
                function onYChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
            }

            Connections {
                enabled: !!root.scrollBarParent
                target: root.scrollBarParent

                function onHeightChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
                function onWidthChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
                function onXChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
                function onYChanged() {
                    Qt.callLater(verticalScrollBar.updateGeometry);
                }
            }
        }
        delegate: Rectangle {
            id: rootItem

            property bool isHovered: false
            property bool isRename: false
            property var startMove: [-1, -1]
            property bool tooltipVisible: false
            property bool cancelRename: false
            
            // 录音时：当前文件夹保持正常显示，其他文件夹置灰
            opacity: (isRecordingAudio && index !== folderListView.currentIndex) ? 0.5 : 1.0

            color: index === folderListView.currentIndex ? (root.activeFocus ? palette.highlight : DTK.themeType === ApplicationHelper.LightType ? "#33000000" : "#33FFFFFF") : (folderListView.hoverIndex === index || isHovered ? (DTK.themeType === ApplicationHelper.LightType ? "#1A000000" : "#1AFFFFFF") : "transparent")
            enabled: !isPlay || index === folderListView.currentIndex
            height: itemHeight
            radius: 6
            width: parent.width

            Keys.onPressed: function(event) {
                switch (event.key) {
                case Qt.Key_Enter:
                case Qt.Key_Return:
                    var newName = renameLine.text;
                    if (newName.length !== 0 && newName !== model.text) {
                        VNoteMainManager.renameFolder(index, newName);
                        model.name = newName;
                        updateFolderName(newName);
                    }
                    cancelRename = false;
                    isRename = false;
                    root.forceActiveFocus();
                    break;
                case Qt.Key_Escape:
                    cancelRename = true;
                    renameLine.text = model.name;
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
                    maximumLength: 64
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
                            if (!rootItem.cancelRename && text.length !== 0 && text !== model.name) {
                                model.name = text;
                                VNoteMainManager.renameFolder(index, text);
                            } else {
                                renameLine.text = model.name;
                            }
                            rootItem.cancelRename = false;
                            folderMouseArea.enabled = true;
                            deselect();
                            rootItem.isRename = false;
                        }
                    }
                }

                Label {
                    id: folderNameLabel

                    Layout.fillWidth: true
                    color: index === folderListView.currentIndex ? (root.activeFocus ? palette.highlightedText : (DTK.themeType === ApplicationHelper.LightType ? "black" : "#B2FFFFFF")) : (DTK.themeType === ApplicationHelper.LightType ? "black" : "#B2FFFFFF")
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

                onHeldChanged: root.isDragging = held

                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                drag.target: this
                enabled: parent.enabled
                hoverEnabled: true

                onClicked: function(mouse) {
                    // 录音时禁用文件夹切换
                    if (isRecordingAudio) {
                        console.log("Cannot switch folder while recording audio");
                        return;
                    }
                    
                    root.forceActiveFocus();
                    tooltipVisible = false;
                    if (folderListView.itemAtIndex(folderListView.lastCurrentIndex)) {
                        folderListView.itemAtIndex(folderListView.lastCurrentIndex).isRename = false;
                    }
                    folderListView.currentIndex = index;
                    folderListView.lastCurrentIndex = index;
                    if (mouse.button === Qt.RightButton) {
                        // 录音时禁用文件夹右键菜单
                        if (isRecordingAudio) {
                            console.log("Cannot show folder context menu while recording audio");
                            return;
                        }
                        folderListView.contextIndex = index;
                        folderItemContextMenu.popup();
                    }
                    rootItem.isHovered = false;
                }
                onDoubleClicked: {
                    // 录音时禁用双击重命名
                    if (isRecordingAudio) {
                        console.log("Cannot rename folder while recording audio");
                        return;
                    }
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
                onPositionChanged: function(mouse) {
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
                        mouseChanged(globPos.x, globPos.y);
                    } else {
                        dragControl.visible = false;
                    }
                }
                onPressed: function(mouse) {
                    startMove[0] = mouse.x;
                    startMove[1] = mouse.y;
                    folderListView.dragSourceFolderId = model.folderId;
                }
                onReleased: {
                    startMove = [-1, -1];
                    if (held) {
                        held = false;
                        if (folderListView.dropIndex != -1) {
                            var sourceIndex = folderListView.indexOfFolderId(folderListView.dragSourceFolderId);
                            if (sourceIndex === -1) {
                                folderListView.clearDragState();
                                return;
                            }
                            var targetIndex = folderListView.dropIndex;
                            var currentFolderId = folderListView.currentIndex >= 0 && folderListView.currentIndex < folderModel.count
                                    ? folderModel.get(folderListView.currentIndex).folderId : -1;
                            if (targetIndex > sourceIndex) {
                                targetIndex -= 1;
                            }
                            if (targetIndex != sourceIndex) {
                                folderModel.move(sourceIndex, targetIndex, 1);
                                folderListView.positionViewAtIndex(targetIndex, ListView.Contain);
                                var currentIndexAfterMove = folderListView.indexOfFolderId(currentFolderId);
                                if (currentIndexAfterMove !== -1) {
                                    folderListView.currentIndex = currentIndexAfterMove;
                                    folderListView.lastCurrentIndex = currentIndexAfterMove;
                                    folderListView.contextIndex = currentIndexAfterMove;
                                }
                                VNoteMainManager.updateSort(sourceIndex, targetIndex);
                            }
                        }
                        folderListView.clearDragState();
                    }
                }
                onCanceled: {
                    startMove = [-1, -1];
                    held = false;
                    folderListView.clearDragState();
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
                    id: deleteMenuItem
                    enabled: !root.isPlay && !root.isRecordingAudio && !root.isVoiceToText
                    text: qsTr("Delete")

                    onTriggered: {
                        if (webVisible || root.isVoiceToText) {
                            console.log("No notes available, cannot delete folder");
                            return;
                        }
                        
                        messageDialogLoader.showDialog(VNoteMessageDialogHandler.DeleteFolder, ret => {
                            if (ret) {
                                if (!VNoteMainManager.vNoteDeleteFolder(folderListView.contextIndex))
                                    return;
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
                    id: newNoteMenuItem
                    enabled: !root.isPlay && !root.isRecordingAudio && !root.isVoiceToText
                    text: qsTr("New Note")

                    onTriggered: {
                        VNoteMainManager.createNote();
                    }
                }
            }

            function openContextMenuAt(x, y) {
                // 使用 QtQuick Controls 2 Menu 的重载：popup(item, x, y)
                try {
                    folderItemContextMenu.popup(rootItem, x, y);
                } catch (e) {
                    folderItemContextMenu.popup();
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

            onPressed: function(mouse) {
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
