// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebChannel 1.15
import Qt.labs.platform 1.1  // 提供Qt5/Qt6兼容的FileDialog
import "../dialog/"
import "../drag/"
import VNote 1.0
import org.deepin.dtk 1.0

Item {
    id: rootItem

    property bool isPlay: false
    property bool isSearch: false
    property bool isSearching: false
    property int itemSpacing: 6
    property alias model: itemModel
    property alias moveToFolderDialog: dialogWindow
    property var saveFilters: ["TXT(*.txt);;HTML(*.html)", "TXT(*.txt)", "HTML(*.html)", "MP3(*.mp3)"]
    property alias searchLoader: noSearchResultsLoader
    property int selectSize: 0
    property var selectedNoteItem: []
    property int topSize: 0
    property alias view: itemListView
    property bool webVisible: true

    signal deleteFinished
    signal deleteNotes(int number)
    signal dropRelease
    signal emptyItemList
    signal mouseChanged(int mousePosX, int mousePosY)
    signal mulChoices(int choices)
    signal noteItemChanged(int index)

    function changeCurrentIndex(index) {
        itemListView.currentIndex = index;
    }

    function onDeleteNote() {
        if (webVisible) {
            console.log("No notes available, cannot delete");
            return;
        }
        
        messageDialogLoader.messageData = selectedNoteItem.length;
        if (messageDialogLoader.active) {
            messageDialogLoader.item.show();
        }
        messageDialogLoader.showDialog(VNoteMessageDialogHandler.DeleteNote, ret => {
            if (!ret) {
                return;
            }
            var delList = itemListView.sortAndDeduplicate(selectedNoteItem);
            var delIdList = [];
            var deleIndex = Number.MAX_SAFE_INTEGER;
            for (var i = 0; i < delList.length; i++) {
                delIdList.push(itemModel.get(delList[i]).noteId);
                itemModel.remove(delList[i]);
                if (selectedNoteItem[i] < deleIndex)
                    deleIndex = selectedNoteItem[i];
            }
            VNoteMainManager.deleteNote(delIdList);
            deleteNotes(selectedNoteItem.length);
            if (itemModel.count <= deleIndex) {
                itemListView.itemAtIndex(itemModel.count - 1).isSelected = true;
                selectedNoteItem = [itemModel.count - 1];
                noteItemChanged(itemModel.get(itemModel.count - 1).noteId);
            } else {
                itemListView.itemAtIndex(deleIndex).isSelected = true;
                noteItemChanged(itemModel.get(deleIndex).noteId);
            }
            gc();
            deleteFinished();
        });
    }

    function onMoveNote() {
        if (webVisible) {
            console.log("No notes available, cannot move");
            return;
        }
        
        var desText = "";
        if (selectedNoteItem.length > 1) {
            desText = qsTr("move ") + selectedNoteItem.length + qsTr(" notes to :");
        } else {
            desText = qsTr("move ") + itemModel.get(itemListView.contextIndex).name + qsTr(" note to :");
        }
        dialogWindow.name = desText;
        dialogWindow.show();
    }

    function onSaveAudio() {
        if (webVisible) {
            console.log("No notes available, cannot save audio");
            return;
        }
        
        if (!folderDialogLoader.active) {
            folderDialogLoader.saveType = VNoteMainManager.Voice;
            folderDialogLoader.active = true;
        } else {
            folderDialogLoader.item.open();
        }
    }

    function onSaveNote() {
        if (webVisible) {
            console.log("No notes available, cannot save note");
            return;
        }
        
        if (selectedNoteItem.length > 1) {
            folderDialogLoader.saveType = VNoteMainManager.Text;
            if (!folderDialogLoader.active) {
                folderDialogLoader.active = true;
            } else {
                folderDialogLoader.item.open();
            }
        } else {
            fileDialogLoader.saveType = VNoteMainManager.Text;
            fileDialogLoader.active = false;
            fileDialogLoader.active = true;
        }
    }

    function renameCurrentItem() {
        if (selectedNoteItem.length !== 1)
            return;
        itemListView.itemAtIndex(selectedNoteItem[0]).isRename = true;
    }

    height: 480
    visible: true
    width: 640

    Keys.onDeletePressed: {
        if (webVisible) {
            console.log("No notes available, cannot delete");
            return;
        }
        rootItem.onDeleteNote();
    }
    onSelectSizeChanged: {
        mulChoices(selectSize);
    }

    ListModel {
        id: itemModel

    }

    MoveDialog {
        id: dialogWindow

        onMoveToFolder: {
            var indexList = [];
            for (var i = 0; i < selectedNoteItem.length; i++) {
                indexList.push(itemModel.get(selectedNoteItem[i]).noteId);
            }
            VNoteMainManager.moveNotes(indexList, index);
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

            title: qsTr("Save As")
            fileMode: FileDialog.SaveFile
            nameFilters: saveFilters[saveType]

            property url initialFolder: {
                var savedPath = "";
                if (saveType === VNoteMainManager.Text || saveType === VNoteMainManager.Html) {
                    savedPath = VNoteMainManager.getSavedTextPath();
                } else if (saveType === VNoteMainManager.Voice) {
                    savedPath = VNoteMainManager.getSavedVoicePath();
                }
                
                if (savedPath && savedPath.length > 0) {
                    return "file://" + savedPath;
                } else {
                    return StandardPaths.standardLocations(StandardPaths.DesktopLocation)[0];
                }
            }
            
            Component.onCompleted: {
                // Set folder property for Qt5 compatibility
                try {
                    fileDialog.folder = initialFolder;
                } catch (e) {
                    console.log("folder property not available, trying currentFolder");
                }
                
                // Set currentFolder property for Qt6 compatibility
                try {
                    fileDialog.currentFolder = initialFolder;
                } catch (e) {
                    console.log("currentFolder property not available, using folder");
                }
                
                var defaultFileName = "";
                if (selectedNoteItem.length > 0 && itemModel.get(selectedNoteItem[0])) {
                    defaultFileName = itemModel.get(selectedNoteItem[0]).name;
                    if (saveType === VNoteMainManager.Text && !defaultFileName.toLowerCase().endsWith('.txt')) {
                        defaultFileName += '.txt';
                    } else if (saveType === VNoteMainManager.Html && !defaultFileName.toLowerCase().endsWith('.html')) {
                        defaultFileName += '.html';
                    }
                }
                
                try {
                    if (defaultFileName) {
                        fileDialog.currentFile = Qt.resolvedUrl(initialFolder + "/" + defaultFileName);
                    } else {
                        fileDialog.currentFile = "";
                    }
                } catch (e) {
                    console.log("currentFile property not available");
                }
                
                fileDialog.open();
            }
            onAccepted: {
                var idList = [];
                for (var i = 0; i < selectedNoteItem.length; i++) {
                    idList.push(itemModel.get(selectedNoteItem[i]).noteId);
                }
                
                var selectedFileUrl;
                
                if (typeof fileDialog.selectedFile !== 'undefined' && fileDialog.selectedFile) {
                    selectedFileUrl = fileDialog.selectedFile;
                } 
                else if (typeof fileDialog.fileUrl !== 'undefined' && fileDialog.fileUrl) {
                    selectedFileUrl = fileDialog.fileUrl;
                }
                else if (typeof fileDialog.currentFile !== 'undefined' && fileDialog.currentFile) {
                    selectedFileUrl = fileDialog.currentFile;
                }
                
                if (!selectedFileUrl || selectedFileUrl.toString().length === 0) {
                    return;
                }

                var fileUrl = selectedFileUrl.toString();
                var folderPath = "";
                if (typeof fileDialog.folder !== 'undefined' && fileDialog.folder) {
                    folderPath = fileDialog.folder.toString();
                } else if (typeof fileDialog.currentFolder !== 'undefined' && fileDialog.currentFolder) {
                    folderPath = fileDialog.currentFolder.toString();
                }
                
                if (!folderPath) {
                    if (fileUrl.startsWith("file://")) {
                        fileUrl = fileUrl.substring(7);
                    }
                    folderPath = fileUrl.substring(0, fileUrl.lastIndexOf('/'));
                } else {
                    if (folderPath.startsWith("file://")) {
                        folderPath = folderPath.substring(7);
                    }
                }
                
                VNoteMainManager.saveUserSelectedPath(folderPath, saveType);

                VNoteMainManager.saveAs(idList, selectedFileUrl, saveType);
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
            
            property url initialFolder: {
                var savedPath = "";
                if (saveType === VNoteMainManager.Text || saveType === VNoteMainManager.Html) {
                    savedPath = VNoteMainManager.getSavedTextPath();
                } else if (saveType === VNoteMainManager.Voice) {
                    savedPath = VNoteMainManager.getSavedVoicePath();
                }
                
                if (savedPath && savedPath.length > 0) {
                    return "file://" + savedPath;
                } else {
                    return StandardPaths.standardLocations(StandardPaths.DesktopLocation)[0];
                }
            }

            Component.onCompleted: {
                // Set folder property for Qt5 compatibility
                try {
                    folderDialog.folder = initialFolder;
                } catch (e) {
                    console.log("folder property not available for FolderDialog, trying currentFolder");
                }
                
                // Set currentFolder property for Qt6 compatibility
                try {
                    folderDialog.currentFolder = initialFolder;
                } catch (e) {
                    console.log("currentFolder property not available for FolderDialog, using folder");
                }
                
                folderDialog.open();
            }
            onAccepted: {
                var list = [];
                for (var i = 0; i < selectedNoteItem.length; i++) {
                    list.push(itemModel.get(selectedNoteItem[i]).noteId);
                }
                
                var selectedFolderUrl;
                
                if (typeof folderDialog.selectedFolder !== 'undefined' && folderDialog.selectedFolder) {
                    selectedFolderUrl = folderDialog.selectedFolder;
                }
                else if (typeof folderDialog.currentFolder !== 'undefined' && folderDialog.currentFolder) {
                    selectedFolderUrl = folderDialog.currentFolder;
                }
                else if (typeof folderDialog.folder !== 'undefined' && folderDialog.folder) {
                    selectedFolderUrl = folderDialog.folder;
                }
                
                if (!selectedFolderUrl || selectedFolderUrl.toString().length === 0) {
                    return;
                }
                
                var folderPath = selectedFolderUrl.toString();
                if (folderPath.startsWith("file://")) {
                    folderPath = folderPath.substring(7);
                }
                VNoteMainManager.saveUserSelectedPath(folderPath, saveType);
                
                VNoteMainManager.saveAs(list, selectedFolderUrl, saveType);
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

            Label {
                anchors.centerIn: parent
                color: DTK.themeType === ApplicationHelper.LightType ? "#66000000" : "#66ffffff"
                font: DTK.fontManager.t6
                text: qsTr("No search results")
            }
        }
    }

    VNoteRightMenu {
        id: noteCtxMenu

        menuType: ActionManager.NoteCtxMenu

        onAboutToShow: {
            var setTop = true;
            if (itemModel.get(itemListView.contextIndex).isTop === "top") {
                setTop = false;
            }
            var topItem = ActionManager.getActionById(ActionManager.NoteTop);
            topItem.text = setTop ? qsTr("Sticky on Top") : qsTr("Unpin");

            ActionManager.enableVoicePlayActions(!isPlay);
            
            // 在搜索状态下禁用移动、置顶和新建笔记选项
            if (isSearching) {
                ActionManager.enableAction(ActionManager.NoteMove, false);
                ActionManager.enableAction(ActionManager.NoteTop, false);
                ActionManager.enableAction(ActionManager.NoteAddNew, false);
            } else {
                ActionManager.enableAction(ActionManager.NoteMove, true);
                ActionManager.enableAction(ActionManager.NoteTop, true);
                ActionManager.enableAction(ActionManager.NoteAddNew, true);
            }
        }
        onActionTrigger: actionId => {
            // 检查是否有笔记内容，如果处于初始界面则不执行操作
            if (webVisible) {
                console.log("No notes available, cannot perform menu action:", actionId);
                return;
            }
            
            switch (actionId) {
            case ActionManager.NoteRename:
                var currentItem = itemListView.itemAtIndex(itemListView.contextIndex);
                if (currentItem) {
                    currentItem.isRename = true;
                }
                break;
            case ActionManager.NoteTop:
                var setTop = true;
                if (itemModel.get(itemListView.contextIndex).isTop === "top") {
                    setTop = false;
                }
                VNoteMainManager.updateTop(itemModel.get(itemListView.contextIndex).noteId, setTop);
                break;
            case ActionManager.NoteMove:
                onMoveNote();
                break;
            case ActionManager.NoteDelete:
                rootItem.onDeleteNote();
                break;
            case ActionManager.SaveNoteAsText:
                rootItem.onSaveNote();
                break;
            case ActionManager.SaveNoteAsHtml:
                onTriggered: {
                    if (selectedNoteItem.length > 1) {
                        folderDialogLoader.saveType = VNoteMainManager.Html;
                        if (!folderDialogLoader.active) {
                            folderDialogLoader.active = true;
                        } else {
                            folderDialogLoader.item.open();
                        }
                    } else {
                        fileDialogLoader.saveType = VNoteMainManager.Html;
                        fileDialogLoader.active = false;
                        fileDialogLoader.active = true;
                    }
                }
                break;
            case ActionManager.NoteSaveVoice:
                rootItem.onSaveAudio();
                break;
            case ActionManager.NoteAddNew:
                VNoteMainManager.createNote();
                break;
            default:
                break;
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
            selectedNoteItem = [0];
            noteItemChanged(notesData[0].noteId);
            selectSize = 1;
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
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        model: itemModel
        spacing: itemSpacing
        visible: true

        delegate: Rectangle {
            id: rootItemDelegate

            property bool hovered: false
            property bool isRename: false
            property bool isSelected: false
            property var startMove: [-1, -1]

            color: isSelected ? (rootItem.activeFocus ? "#FF1F6EE7" : (DTK.themeType === ApplicationHelper.LightType ? "#33000000" : "#33FFFFFF")) : (DTK.themeType === ApplicationHelper.LightType ? "white" : "#0CFFFFFF")
            enabled: !isPlay || itemListView.currentIndex === index
            height: isSearch ? 67 : 50
            radius: 6
            width: itemListView.width

            Component.onCompleted: {
                isSelected = (selectedNoteItem.indexOf(index) !== -1);
            }
            Keys.onPressed: function(event) {
                if (isRename) {
                    if (event.key === Qt.Key_Escape) {
                        renameLine.text = model.name;
                        isRename = false;
                        rootItem.forceActiveFocus();
                    } else if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                        isRename = false;
                        rootItem.forceActiveFocus();
                    }
                } else {
                    if (event.key === Qt.key_delete) {
                        if (webVisible) {
                            console.log("No notes available, cannot delete");
                            return;
                        }
                        rootItem.onDeleteNote();
                    }
                }
            }
            onIsRenameChanged: {
                renameLine.forceActiveFocus();
            }

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

                    // width: parent.width - 10
                    Layout.fillWidth: true
                    color: DTK.themeType === ApplicationHelper.LightType ? (isSelected ? (rootItem.activeFocus ? "white" : "black") : "black") : "white"
                    elide: Text.ElideRight
                    font: DTK.fontManager.t6
                    height: 18
                    horizontalAlignment: Text.AlignHLeft
                    text: model.name
                    visible: !isRename
                }

                Label {
                    id: timeLabel

                    Layout.fillWidth: true
                    color: isSelected ? (rootItem.activeFocus ? "#7FFFFFFF" : DTK.themeType === ApplicationHelper.LightType ? "#7F000000" : "#7FFFFFFF") : (DTK.themeType === ApplicationHelper.LightType ? "#7F000000" : "#7FFFFFFF")
                    font.pixelSize: 10
                    height: 15
                    horizontalAlignment: Text.AlignHLeft
                    text: model.time
                    visible: !isRename
                }

                ToolTip {
                    id: itemNameTip

                    text: model.name
                    visible: rootItemDelegate.hovered
                }

                LineEdit {
                    id: renameLine

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    text: model.name
                    visible: rootItemDelegate.isRename

                    backgroundColor: Palette {
                        normal: Qt.rgba(1, 1, 1, 0.85)
                        normalDark: Qt.rgba(0, 0, 0, 0.5)
                    }

                    onActiveFocusChanged: {
                        noteItemMouseArea.enabled = false;
                        if (activeFocus) {
                            selectAll();
                        } else {
                            if (text.length !== 0 && text !== model.name) {
                                model.name = text;
                                VNoteMainManager.renameNote(model.noteId, text);
                            } else {
                                renameLine.text = model.name;
                            }
                            noteItemMouseArea.enabled = true;
                            rootItemDelegate.isRename = false;
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
                hoverEnabled: true

                onClicked: {
                    if (mouse.button === Qt.RightButton) {
                        if (selectedNoteItem.length > 1) {
                            ActionManager.visibleMulChoicesActions(false);
                        } else {
                            if (itemListView.itemAtIndex(itemListView.lastCurrentIndex)) {
                                var lastItem = itemListView.itemAtIndex(itemListView.lastCurrentIndex);
                                lastItem.isRename = false;
                                lastItem.isSelected = false;
                            }
                            var item = itemListView.itemAtIndex(selectedNoteItem[0]);
                            item.isSelected = false;
                            selectedNoteItem = [index];
                            selectSize = 1;
                            itemListView.itemAtIndex(index).isSelected = true;
                            itemListView.currentIndex = index;
                            itemListView.lastCurrentIndex = index;
                            itemListView.contextIndex = index;
                            ActionManager.visibleMulChoicesActions(true);
                        }
                        var list = [];
                        for (var i = 0; i < selectedNoteItem.length; i++) {
                            list.push(itemModel.get(selectedNoteItem[i]).noteId);
                        }
                        VNoteMainManager.checkNoteVoice(list);
                        noteCtxMenu.popup();
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
                            selectedNoteItem = [];
                            selectSize = 0;
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
                onDoubleClicked: {
                    itemListView.itemAtIndex(index).isRename = true;
                }
                onEntered: {
                    if (noteNameLabel.implicitWidth > noteNameLabel.width)
                        rootItemDelegate.hovered = true;
                }
                onExited: {
                    rootItemDelegate.hovered = false;
                }
                onPositionChanged: function(mouse) {
                    if (!held) {
                        if ((startMove[0] !== -1 || startMove[1] !== -1) && ((Math.abs(mouse.x - startMove[0]) > 5) || (Math.abs(mouse.y - startMove[1]) > 5))) {
                            held = true;
                        } else {
                            return;
                        }
                    }
                    if (itemModel.get(selectedNoteItem[0])) {
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
                onPressed: function(mouse) {
                    startMove[0] = mouse.x;
                    startMove[1] = mouse.y;
                }
                onReleased: function(mouse) {
                    startMove = [-1, -1];
                    if (held && dragControl.visible && mouse.button === Qt.LeftButton) {
                        dropRelease();
                    }
                    held = false;
                    dragControl.visible = false;
                }
            }
        }
        footer: Component {
            id: footerComponent

            Rectangle {
                color: "transparent"
                height: 10
                width: itemListView.width
            }
        }

        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true

            onPressed: {
                var index = itemListView.currentIndex;
                var item = itemListView.itemAtIndex(index);
                if (item.isRename) {
                    var clickX = mouse.x;
                    var clickY = mouse.y;
                    var localPoint = item.mapFromItem(parent, clickX, clickY);
                    if (!item.contains(localPoint)) {
                        rootItem.forceActiveFocus();
                    }
                } else {
                    rootItem.forceActiveFocus();
                }
                mouse.accepted = false;
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
    }
}
