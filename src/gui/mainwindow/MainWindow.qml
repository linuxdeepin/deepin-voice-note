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
    id: rootWindow

    property int createFolderBtnHeight: 40
    property int leftViewWidth: 200
    property int windowMiniHeight: 680
    property int windowMiniWidth: 1070

    function toggleTwoColumnMode() {
        if (leftBgArea.visible === false) {
            //TODO: 这加个动画
            leftBgArea.visible = true;
            leftDragHandle.visible = true;
            showLeftArea.start();
        } else {
            hideLeftArea.start();
        }
    }

    DWindow.alphaBufferSize: 8
    DWindow.enabled: true
    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: windowMiniHeight
    minimumHeight: windowMiniHeight
    minimumWidth: windowMiniWidth
    visible: true
    width: windowMiniWidth
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2

    Shortcuts {
        id: shortcuts

        enabled: rootWindow.active

        onCreateFolder: {
            VNoteMainManager.vNoteCreateFolder();
        }
        onCreateNote: {
            VNoteMainManager.createNote();
        }
        onRenameFolder: {
            folderListView.renameCurrentItem();
        }
        onRenameNote: {
            itemListView.renameCurrentItem();
        }
        onShowShortCutView: {
            VNoteMainManager.preViewShortcut(Qt.point(rootWindow.x + rootWindow.width / 2, rootWindow.y + rootWindow.height / 2));
        }
        onStartRecording: {
            webEngineView.startRecording();
        }
    }

    Connections {
        function handleFinishedFolderLoad(foldersData) {
            for (var i = 0; i < foldersData.length; i++) {
                folderListView.model.append({
                        name: foldersData[i].name,
                        count: foldersData[i].notesCount,
                        icon: foldersData[i].icon
                    });
            }
        }

        function handleUpdateNoteList(notesData) {
            itemListView.model.clear();
            for (var i = 0; i < notesData.length; i++) {
                var itemIsTop = notesData[i].isTop ? "top" : "normal";
                itemListView.model.append({
                        name: notesData[i].name,
                        time: notesData[i].time,
                        isTop: itemIsTop,
                        icon: notesData[i].icon,
                        folderName: notesData[i].folderName,
                        noteId: notesData[i].noteId
                    });
            }
        }

        function handleaddNote(notesData) {
            itemListView.model.clear();
            var getSelect = -1;
            for (var i = 0; i < notesData.length; i++) {
                var itemIsTop = notesData[i].isTop ? "top" : "normal";
                itemListView.model.append({
                        name: notesData[i].name,
                        time: notesData[i].time,
                        isTop: itemIsTop,
                        icon: notesData[i].icon,
                        folderName: notesData[i].folderName,
                        noteId: notesData[i].noteId
                    });
                if (getSelect === -1 && !notesData[i].isTop)
                    getSelect = i;
            }
            itemListView.selectedNoteItem = [getSelect];
            itemListView.selectSize = 1;
            folderListView.addNote(1);
        }

        target: VNoteMainManager

        onAddNoteAtHead: {
            handleaddNote(notesData);
        }
        onFinishedFolderLoad: {
            if (foldersData.length > 0)
                initRect.visible = false;
            initiaInterface.loadFinished(foldersData.length > 0);
            handleFinishedFolderLoad(foldersData);
            itemListView.selectedNoteItem = [0];
            itemListView.selectSize = 1;
        }
        onMoveFinished: {
            folderListView.model.get(srcFolderIndex).count = (Number(folderListView.model.get(srcFolderIndex).count) - index.length).toString();
            folderListView.model.get(dstFolderIndex).count = (Number(folderListView.model.get(dstFolderIndex).count) + index.length).toString();
            for (var i = 0; i < index.length; i++) {
                itemListView.model.remove(index[i]);
            }
        }
        onNoSearchResult: {
            label.visible = false;
            folderListView.opacity = 0.4;
            folderListView.enabled = false;
            createFolderButton.enabled = false;
        }
        onSearchFinished: {
            label.visible = false;
            folderListView.opacity = 0.4;
            folderListView.enabled = false;
            createFolderButton.enabled = false;
        }
        onUpdateNotes: {
            handleUpdateNoteList(notesData);
            itemListView.selectedNoteItem = [selectIndex];
            itemListView.selectSize = 1;
        }
    }

    IconLabel {
        id: appImage

        height: 20
        icon.name: "deepin-voice-note"
        width: 20
        x: 10
        y: 10
        z: 100
    }

    ToolButton {
        id: twoColumnModeBtn

        height: 30
        icon.height: 30
        icon.name: "topleft"
        icon.width: 30
        width: 30
        x: 50
        y: 5
        z: 100

        onClicked: {
            toggleTwoColumnMode();
        }
    }

    RowLayout {
        id: rowLayout

        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: leftBgArea

            Layout.fillHeight: true//#F2F6F8
            Layout.preferredWidth: leftViewWidth
            color: DTK.themeType === ApplicationHelper.LightType ? Qt.rgba(242 / 255, 246 / 255, 248 / 255, 1) : Qt.rgba(16.0 / 255.0, 16.0 / 255.0, 16.0 / 255.0, 0.85)

            Rectangle {
                anchors.right: parent.right
                color: DTK.themeType === ApplicationHelper.LightType ? "#eee7e7e7" : "#ee252525"
                height: parent.height
                width: 1
            }

            ColumnLayout {
                anchors.bottomMargin: 10
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 50

                FolderListView {
                    id: folderListView

                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    onFolderEmpty: {
                        initRect.visible = true;
                    }
                    onItemChanged: {
                        label.text = name;
                        itemListView.selectedNoteItem = [0];
                        itemListView.selectSize = 1;
                        VNoteMainManager.vNoteFloderChanged(index);
                    }
                    onUpdateFolderName: {
                        label.text = name;
                    }
                }

                Button {
                    id: createFolderButton

                    Layout.fillWidth: true
                    Layout.preferredHeight: createFolderBtnHeight
                    text: qsTr("Create Notebook")

                    onClicked: {
                        folderListView.addFolder();
                    }
                }
            }

            Connections {
                target: itemListView

                onDeleteNotes: {
                    folderListView.delNote(number);
                }
                onDropRelease: {
                    folderListView.dropItems(itemListView.selectedNoteItem);
                }
                onMouseChanged: {
                    folderListView.updateItems(mousePosX, mousePosY);
                }
                onMulChoices: {
                    webEngineView.toggleMultCho(choices);
                }
            }
        }

        Rectangle {
            id: leftDragHandle

            Layout.fillHeight: true
            Layout.preferredWidth: 5
            color: middeleBgArea.color

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.axis: Drag.XAxis
                drag.target: leftDragHandle

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

            Layout.fillHeight: true
            Layout.preferredWidth: leftViewWidth
            color: Qt.rgba(0, 0, 0, 0.05)

            ColumnLayout {
                Layout.topMargin: 7
                anchors.fill: parent

                SearchEdit {
                    id: search

                    function exitSearch() {
                        folderListView.toggleSearch(false);
                        search.focus = false;
                        if (itemListView.searchLoader.active) {
                            itemListView.searchLoader.item.visible = false;
                        }
                        itemListView.view.visible = true;
                        label.visible = true;
                        folderListView.opacity = 1;
                        folderListView.enabled = true;
                        createFolderButton.enabled = true;
                        itemListView.isSearch = false;
                        VNoteMainManager.clearSearch();
                    }

                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    Layout.topMargin: 8
                    placeholder: qsTr("Search")

                    Keys.onPressed: {
                        if (text.length === 0)
                            return;
                        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                            VNoteMainManager.vNoteSearch(text);
                        }
                    }
                    onTextChanged: {
                        if (text.length === 0)
                            exitSearch();
                    }

                    Connections {
                        function onClicked(mouse) {
                            search.exitSearch();
                        }

                        target: search.clearButton.item
                    }
                }

                Label {
                    id: label

                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#BB000000"
                    font.pixelSize: 16
                    text: ""
                }

                ItemListView {
                    id: itemListView

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    moveToFolderDialog.folderModel: folderListView.model

                    onNoteItemChanged: {
                        VNoteMainManager.vNoteChanged(index);
                    }
                }
            }
        }

        Rectangle {
            id: rightDragHandle

            Layout.fillHeight: true
            Layout.preferredWidth: 5
            color: middeleBgArea.color

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.axis: Drag.XAxis
                drag.target: rightDragHandle

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

            Layout.fillHeight: true
            Layout.fillWidth: true
            color: Qt.rgba(0, 0, 0, 0.01)

            BoxShadow {
                anchors.fill: rightBgArea
                cornerRadius: rightBgArea.radius
                hollow: true
                shadowBlur: 10
                shadowColor: Qt.rgba(0, 0, 0, 0.05)
                shadowOffsetX: 0
                shadowOffsetY: 4
                spread: 0
            }

            ColumnLayout {
                anchors.fill: parent

                WebEngineView {
                    id: webEngineView

                    Layout.fillHeight: true
                    Layout.fillWidth: true
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

                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }

        Connections {
            target: initiaInterface

            onCreateFolder: {
                folderListView.addFolder();
                initRect.visible = false;
            }
        }
    }

    ParallelAnimation {
        id: hideLeftArea

        onFinished: {
            leftBgArea.visible = false;
        }
        onStarted: {
            leftDragHandle.visible = false;
        }

        NumberAnimation {
            duration: 200
            from: leftBgArea.width
            property: "width"
            target: leftBgArea
            to: 0
        }

        NumberAnimation {
            duration: 200
            from: leftViewWidth + 5
            property: "x"
            target: middeleBgArea
            to: 0
        }

        NumberAnimation {
            duration: 200
            from: leftViewWidth
            property: "width"
            target: middeleBgArea
            to: 260
        }
    }

    ParallelAnimation {
        id: showLeftArea

        NumberAnimation {
            duration: 200
            from: 0
            property: "width"
            target: leftBgArea
            to: leftViewWidth
        }

        NumberAnimation {
            duration: 200
            from: 0
            property: "x"
            target: middeleBgArea
            to: leftViewWidth + 5
        }

        NumberAnimation {
            duration: 200
            from: 260
            property: "width"
            target: middeleBgArea
            to: leftViewWidth
        }
    }
}
