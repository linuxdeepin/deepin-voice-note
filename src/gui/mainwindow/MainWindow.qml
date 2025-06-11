// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import QtQuick.Controls 2.15
import VNote 1.0
import "../dialog"
import org.deepin.dtk 1.0

ApplicationWindow {
    id: rootWindow

    property int createFolderBtnHeight: 40
    property bool isRecording: webEngineView.isRecording
    property int leftAreaMaxWidth: 300
    property int leftAreaMinWidth: 125
    property int leftViewWidth: 220
    property int middleAreaMinWidth: 175
    property int middleViewWidth: 260
    property bool needHideSearch: false
    property int rightAreaMinWidth: 380
    property int tmpLeftAreaWidth: 220
    property int tmpWebViewWidth: 0
    property int tmprightDragX: 0
    property int windowMiniHeight: 300
    property int windowMiniWidth: 685

    function toggleTwoColumnMode() {
        if (leftBgArea.visible === false) {
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
    height: 681
    minimumHeight: windowMiniHeight
    minimumWidth: windowMiniWidth
    visible: true
    width: 1096

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2;
        y = Screen.height / 2 - height / 2;
        checkAndCreateDataPath();
    }
    onClosing: {
        close.accepted = false;
        if (isRecording) {
            close.accepted = false;
            messageDialogLoader.showDialog(VNoteMessageDialogHandler.AbortRecord, ret => {
                if (ret) {
                    webEngineView.stopAndClose();
                    VNoteMainManager.forceExit(true);
                }
            });
        } else {
            if (VNoteMainManager.isVoiceToText()) {
                messageDialogLoader.showDialog(VNoteMessageDialogHandler.AborteAsr, ret => {
                    if (ret) {
                        VNoteMainManager.forceExit();
                    }
                });
            } else
                VNoteMainManager.forceExit();
        }
    }
    onWidthChanged: {
        if (rightBgArea.width < rightAreaMinWidth) {
            var reduce = rightAreaMinWidth - rightBgArea.width;
            if (middleBgArea.width > middleAreaMinWidth) {
                middleBgArea.Layout.preferredWidth = middleBgArea.width - reduce;
            } else {
                leftBgArea.Layout.preferredWidth = leftBgArea.width - reduce;
            }
            rightBgArea.width = rightAreaMinWidth;
        }
    }

    Shortcuts {
        id: shortcuts

        enabled: rootWindow.active

        onCopy: {
            webEngineView.copy();
        }
        onCreateFolder: {
            VNoteMainManager.vNoteCreateFolder();
        }
        onCreateNote: {
            VNoteMainManager.createNote();
        }
        onPlayPauseVoice: {
            VNoteMainManager.resumeVoicePlayer();
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

    VNoteMessageDialogLoader {
        id: messageDialogLoader

    }

    Loader {
        id: settingDlgLoader

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

        function handleUpdateNote(noteId, time) {
            var currentIndex = -1;
            var topSize = 0;
            var topNote = false;
            for (var i = 0; i < itemListView.model.count; i++) {
                var note = itemListView.model.get(i);
                if (note.isTop === "top")
                    ++topSize;
                if (note.noteId === noteId) {
                    note.time = time;
                    currentIndex = i;
                    topNote = (note.isTop === "top");
                }
            }
            if (topNote) {
                if (currentIndex !== 0)
                    itemListView.model.move(currentIndex, 0, 1);
                return 0;
            } else {
                if (currentIndex !== topSize)
                    itemListView.model.move(currentIndex, topSize, 1);
                return topSize;
            }
        }

        function handleUpdateNoteList(notesData) {
            itemListView.model.clear();
            if (notesData.length === 0) {
                webEngineView.webVisible = false;
            } else {
                webEngineView.webVisible = true;
            }
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

        function handleaddNote(noteData) {
            for (var i = 0; i < itemListView.selectedNoteItem.length; i++) {
                var item = itemListView.view.itemAtIndex(itemListView.selectedNoteItem[i]);
                if (item)
                    item.isSelected = false;
            }
            var topSize = 0;
            for (var j = 0; j < itemListView.model.count; j++) {
                var note = itemListView.model.get(j);
                if (note.isTop === "top")
                    ++topSize;
                else
                    break;
            }
            var itemIsTop = noteData.isTop ? "top" : "normal";
            itemListView.model.insert(topSize, {
                name: noteData.name,
                time: noteData.time,
                isTop: itemIsTop,
                icon: noteData.icon,
                folderName: noteData.folderName,
                noteId: noteData.noteId
            });
            itemListView.selectedNoteItem = [topSize];
            itemListView.selectSize = 1;
            itemListView.changeCurrentIndex(topSize);
            folderListView.addNote(1);
            itemListView.forceActiveFocus();
        }

        function sortDescending(array) {
            return array.slice().sort(function (a, b) {
                return b - a; // 进行降序排序
            });
        }

        target: VNoteMainManager

        onAddFolderFinished: {
            if (initRect.visible) {
                initRect.visible = false;
            }
        }
        onAddNoteAtHead: {
            handleaddNote(noteData);
        }
        onFinishedFolderLoad: {
            if (foldersData.length > 0) {
                initRect.visible = false;
            }
            initiaInterface.loadFinished(foldersData.length > 0);
            handleFinishedFolderLoad(foldersData);
            itemListView.selectedNoteItem = [0];
            itemListView.selectSize = 1;
        }
        onMoveFinished: {
            folderListView.model.get(srcFolderIndex).count = (Number(folderListView.model.get(srcFolderIndex).count) - index.length).toString();
            folderListView.model.get(dstFolderIndex).count = (Number(folderListView.model.get(dstFolderIndex).count) + index.length).toString();
            var sortedArray = sortDescending(itemListView.selectedNoteItem);
            var minIndex = sortedArray[sortedArray.length - 1];
            for (var i = 0; i < sortedArray.length; i++) {
                itemListView.model.remove(sortedArray[i]);
            }
            itemListView.selectedNoteItem = [];
            if (Number(folderListView.model.get(srcFolderIndex).count) === 0) {
                webEngineView.webVisible = false;
            } else {
                webEngineView.webVisible = true;
            }
            if (!itemListView.view.itemAtIndex(minIndex)) {
                minIndex = itemListView.model.count - 1;
            }
            itemListView.selectedNoteItem.push(minIndex);
            itemListView.view.itemAtIndex(minIndex).isSelected = true;
            itemListView.selectSize = 1;
            VNoteMainManager.vNoteChanged(itemListView.model.get(minIndex).noteId);
        }
        onNoSearchResult: {
            label.visible = false;
            folderListView.opacity = 0.4;
            folderListView.enabled = false;
            createFolderButton.enabled = false;
            webEngineView.webVisible = false;
            webEngineView.noSearchResult = true;
        }
        onSearchFinished: {
            label.visible = false;
            folderListView.opacity = 0.4;
            folderListView.enabled = false;
            createFolderButton.enabled = false;
        }
        onUpdateEditNote: {
            var currentIndex = handleUpdateNote(noteId, time);
            itemListView.selectedNoteItem = [currentIndex];
            itemListView.selectSize = 1;
        }
        onUpdateNotes: {
            handleUpdateNoteList(notesData);
            itemListView.selectedNoteItem = [selectIndex];
            itemListView.selectSize = 1;
            itemListView.changeCurrentIndex(selectIndex);
        }
    }

    IconLabel {
        id: appImage

        height: 30
        icon.height: 30
        icon.name: "deepin-voice-note"
        icon.width: 30
        width: 30
        x: 10
        y: 10
        z: 100
    }

    Item {
        id: twoColumnModeBtn
        width: 30
        height: 30
        x: 50
        y: 10
        z: 100
        visible: !(needHideSearch && search.visible) || leftBgArea.visible

        DciIcon {
            anchors.fill: parent
            name: "topleft"
        }

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: "black"
            opacity: mouseArea.hovered ? 0.1 : 0
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                toggleTwoColumnMode();
            }
        }
    }

    RowLayout {
        id: rowLayout

        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: leftBgArea

            Layout.fillHeight: true//#F2F6F8
            Layout.preferredWidth: leftViewWidth - leftDragHandle.width
            color: DTK.themeType === ApplicationHelper.LightType ? "#FFFFFF" : "#101010"

            ColumnLayout {
                anchors.bottomMargin: 10
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 5
                anchors.topMargin: 50

                FolderListView {
                    id: folderListView

                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    onEmptyItemList: isEmpty => {
                        webEngineView.webVisible = !isEmpty;
                    }
                    onFolderEmpty: {
                        initRect.visible = true;
                        initiaInterface.loadFinished(false);
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
                    var indexList = [];
                    for (var i = 0; i < itemListView.selectedNoteItem.length; i++) {
                        indexList.push(itemListView.model.get(itemListView.selectedNoteItem[i]).noteId);
                    }
                    folderListView.dropItems(indexList);
                }
                onMouseChanged: {
                    if (mousePosY < rootWindow.y) {
                        folderListView.rollUp();
                    } else if (mousePosY > rootWindow.y + rootWindow.height) {
                        folderListView.rollDown();
                    } else {
                        folderListView.rollStop();
                    }

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
            color: leftBgArea.color

            Rectangle {
                anchors.right: parent.right
                color: DTK.themeType === ApplicationHelper.LightType ? "#eee7e7e7" : "black"
                height: parent.height
                width: 1
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.axis: Drag.XAxis
                drag.maximumX: rootWindow.width > (leftAreaMaxWidth + middleAreaMinWidth + rightAreaMinWidth - leftDragHandle.width) ? leftAreaMaxWidth : rootWindow.width - (middleAreaMinWidth + rightAreaMinWidth)
                drag.minimumX: leftAreaMinWidth
                drag.target: leftDragHandle

                onPositionChanged: {
                    if (drag.active) {
                        var newWidth = leftDragHandle.x;
                        if (newWidth >= leftBgArea.width) {
                            var shrinkWidth = newWidth - leftBgArea.width;
                            if ((middleBgArea.width + rightDragHandle.width) > middleAreaMinWidth) {
                                middleBgArea.Layout.preferredWidth = middleBgArea.width - shrinkWidth;
                            } else {
                                rightBgArea.Layout.preferredWidth = rightBgArea.width - shrinkWidth;
                            }
                        } else {
                            var middleShrinkWidth = leftBgArea.width - newWidth;
                            middleBgArea.Layout.preferredWidth = middleBgArea.width + middleShrinkWidth;
                        }
                        leftBgArea.Layout.preferredWidth = newWidth;
                        tmpLeftAreaWidth = newWidth;
                    }
                }
            }
        }

        Rectangle {
            id: middleBgArea

            Layout.fillHeight: true
            Layout.preferredWidth: middleViewWidth - rightDragHandle.width
            color: DTK.themeType === ApplicationHelper.LightType ? "#F8F8F8" : "#181818"

            onWidthChanged: {
                if (!leftBgArea.visible) {
                    if (width >= 240) {
                        search.visible = true;
                        needHideSearch = false;
                        search.offect = 85;
                    } else {
                        search.visible = false;
                        needHideSearch = true;
                        search.offect = 50;
                    }
                }
            }

            ColumnLayout {
                Layout.topMargin: 7
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 5
                spacing: 10

                ToolButton {
                    Layout.alignment: Text.AlignRight
                    Layout.rightMargin: 10
                    Layout.topMargin: 10
                    height: 30
                    icon.name: "search"
                    visible: !search.visible
                    width: 30

                    onClicked: {
                        search.visible = true;
                        search.offect = 50;
                        search.forceActiveFocus();
                    }
                }

                SearchEdit {
                    id: search

                    property int offect: 0

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
                        webEngineView.webVisible = true;
                        webEngineView.noSearchResult = false;
                        VNoteMainManager.clearSearch();
                        if (needHideSearch)
                            search.visible = false;
                    }

                    Layout.fillWidth: true
                    Layout.leftMargin: offect
                    Layout.preferredHeight: 30
                    Layout.topMargin: 12
                    placeholder: qsTr("Search")

                    Keys.onPressed: {
                        if (text.length === 0)
                            return;
                        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                            VNoteMainManager.vNoteSearch(text);
                        }
                    }
                    onActiveFocusChanged: {
                        if (!activeFocus && needHideSearch)
                            search.visible = false;
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
                    Layout.preferredHeight: 18
                    Layout.topMargin: 5
                    color: DTK.themeType === ApplicationHelper.LightType ? "#BB000000" : "#BBFFFFFF"
                    font.pixelSize: 16
                    text: ""
                }

                ItemListView {
                    id: itemListView

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    moveToFolderDialog.folderModel: folderListView.model

                    onDeleteFinished: {
                        webEngineView.toggleMultCho(1);
                    }
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
            color: middleBgArea.color

            Component.onCompleted: {
                tmprightDragX = rightDragHandle.x;
            }

            MouseArea {
                id: rightMouseArea

                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.axis: Drag.XAxis
                drag.maximumX: leftBgArea.visible ? ((rootWindow.width - leftBgArea.width) > (middleAreaMinWidth + rightAreaMinWidth) ? rootWindow.width - rightAreaMinWidth : (rootWindow.width - rightAreaMinWidth)) : (rootWindow.width - rightAreaMinWidth)
                drag.minimumX: leftBgArea.visible ? (leftAreaMinWidth + middleAreaMinWidth + leftDragHandle.width) : (middleAreaMinWidth - rightDragHandle.width)
                drag.target: rightDragHandle

                onPositionChanged: {
                    if (drag.active) {
                        tmprightDragX = rightDragHandle.x;
                        var newWidth = rightDragHandle.x - middleBgArea.x;
                        if (newWidth < (middleAreaMinWidth - rightDragHandle.width)) {
                            middleBgArea.Layout.preferredWidth = middleAreaMinWidth - rightDragHandle.width;
                            var shrinkWidth = middleAreaMinWidth - newWidth - rightDragHandle.width;
                            leftBgArea.Layout.preferredWidth = leftBgArea.width - shrinkWidth - leftDragHandle.width;
                            tmpLeftAreaWidth = leftBgArea.width;
                        } else {
                            middleBgArea.Layout.preferredWidth = newWidth;
                        }
                        rightBgArea.Layout.preferredWidth = rowLayout.width - middleBgArea.width - rightDragHandle.width;
                        tmpWebViewWidth = rightBgArea.width;
                        if (search.activeFocus) {
                            middleBgArea.forceActiveFocus();
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
                    initialVisible: initRect.visible

                    onDeleteNote: {
                        itemListView.onDeleteNote();
                    }
                    onMoveNote: {
                        itemListView.onMoveNote();
                    }
                    onOpenSetting: {
                        if (settingDlgLoader.status === Loader.Null)
                            settingDlgLoader.setSource("../dialog/SettingDialog.qml");
                        if (settingDlgLoader.status === Loader.Ready)
                            settingDlgLoader.item.show();
                    }
                    onPlayStateChange: state => {
                        folderListView.isPlay = state;
                        itemListView.isPlay = state;
                        createFolderButton.enabled = !state;
                        search.enabled = !state;
                    }
                    onSaveAudio: {
                        itemListView.onSaveAudio();
                    }
                    onSaveNote: {
                        itemListView.onSaveNote();
                    }
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

                onTitleOpenSetting: {
                    if (settingDlgLoader.status === Loader.Null)
                        settingDlgLoader.setSource("../dialog/SettingDialog.qml");
                    if (settingDlgLoader.status === Loader.Ready)
                        settingDlgLoader.item.show();
                }
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
            tmprightDragX = middleBgArea.width;
        }
        onStarted: {
            needHideSearch = false;
            if (middleBgArea.width < 240) {
                needHideSearch = true;
                search.visible = false;
            }
            leftDragHandle.visible = false;
            tmpWebViewWidth = rightBgArea.width;
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
            from: leftBgArea.width + leftDragHandle.width
            property: "x"
            target: middleBgArea
            to: 0
        }

        NumberAnimation {
            duration: 200
            from: rightBgArea.x
            property: "x"
            target: rightBgArea
            to: rightBgArea.x - tmpLeftAreaWidth - rightDragHandle.width
        }

        NumberAnimation {
            duration: 200
            from: tmpWebViewWidth
            property: "width"
            target: webEngineView
            to: tmpLeftAreaWidth + rightDragHandle.width + tmpWebViewWidth
        }

        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
            from: 0
            property: "offect"
            target: search
            to: 85
        }
    }

    ParallelAnimation {
        id: showLeftArea

        property int currentMiddleWidth: 0
        property int currentRightX: 0
        property int tmpOffect: 0

        onFinished: {
            search.visible = true;
            leftDragHandle.x = leftBgArea.width;
            rightDragHandle.x = middleBgArea.x + middleBgArea.width;
            tmpWebViewWidth = rightBgArea.width;
        }
        onStarted: {
            currentRightX = rightBgArea.x;
            currentMiddleWidth = middleBgArea.width;
            if (rightBgArea.width > (rightAreaMinWidth + tmpLeftAreaWidth)) {
                showLeftArea.tmpOffect = middleBgArea.width + rightDragHandle.width;
            } else {
                showLeftArea.tmpOffect = middleBgArea.width - (rightAreaMinWidth + tmpLeftAreaWidth - rightBgArea.width);
            }
        }

        NumberAnimation {
            duration: 200
            from: 0
            property: "width"
            target: leftBgArea
            to: tmpLeftAreaWidth
        }

        NumberAnimation {
            duration: 200
            from: 0
            property: "x"
            target: middleBgArea
            to: tmpLeftAreaWidth + rightDragHandle.width
        }

        NumberAnimation {
            duration: 200
            from: currentMiddleWidth
            property: "width"
            target: middleBgArea
            to: showLeftArea.tmpOffect
        }

        NumberAnimation {
            duration: 200
            from: showLeftArea.currentRightX
            property: "x"
            target: rightBgArea
            to: showLeftArea.tmpOffect + tmpLeftAreaWidth + rightDragHandle.width
        }

        NumberAnimation {
            duration: 200
            from: rightBgArea.width
            property: "width"
            target: rightBgArea
            to: tmpWebViewWidth
        }

        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
            from: 85
            property: "offect"
            target: search
            to: 0
        }
    }
}

