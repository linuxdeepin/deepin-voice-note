// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import QtQuick.Controls 2.15
import VNote 1.0
import "../" as VNoteComponents
import "../dialog"
import org.deepin.dtk 1.0

ApplicationWindow {
    id: rootWindow

    property int createFolderBtnHeight: 30
    property bool isRecording: webEngineView.isRecording
    property bool isRecordingAudio: false
    property bool isVoiceToText: false
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

    function collapsedSearchOffset() {
        // offect 是 ColumnLayout 内的 leftMargin，需要减去 ColumnLayout 本身的 leftMargin
        return twoColumnModeBtn.x + twoColumnModeBtn.width + 8 - middleColumnLayout.anchors.leftMargin;
    }

    function toggleTwoColumnMode() {
        if (hideLeftArea.running) {
            hideLeftArea.stop();
            leftBgArea.visible = true;
            leftDragHandle.visible = true;
            showLeftArea.start();
        } else if (showLeftArea.running) {
            showLeftArea.stop();
            hideLeftArea.start();
        } else if (leftBgArea.visible === false) {
            showLeftArea.start();
        } else {
            hideLeftArea.start();
        }
    }

    DWindow.alphaBufferSize: 8
    DWindow.enabled: true
    color: "transparent"
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
    onClosing: function(close) {
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
            if (middleBgArea.width - reduce >= middleAreaMinWidth - rightDragHandle.width) {
                middleBgArea.width -= reduce;
            } else if (leftBgArea.visible) {
                leftBgArea.width -= reduce;
                tmpLeftAreaWidth = leftBgArea.width;
            }
        }
    }

    Shortcuts {
        id: shortcuts

        enabled: rootWindow.active && !itemListView.isDragging && !folderListView.isDragging

        blockCreateKeys: {
            var shouldBlock = (isRecordingAudio || webEngineView.titleBar.isPlaying || itemListView.isSearch || itemListView.isSearching || webEngineView.titleBar.isSearching || isVoiceToText);
            return shouldBlock;
        }
        // 与右上录音按钮同条件：按钮不可用或正在播放时禁用 Ctrl+R
        blockRecordingKey: isRecordingAudio
                           || webEngineView.titleBar.isPlaying
                           || !webEngineView.titleBar.recordBtnEnabled
        initialOnlyCreateFolder: initRect.visible
        isDragging: itemListView.isDragging || folderListView.isDragging

        onCopy: {
            webEngineView.copy();
        }
        onCreateFolder: {
            // 录音或播放时不允许创建记事本
            if (isRecordingAudio || folderListView.isPlay) {
                console.log("Cannot create notebook while recording or playing");
                return;
            }
            // 语音转文字时不允许创建记事本（会自动创建笔记，导致转文字结果丢失）
            if (isVoiceToText) {
                console.log("Cannot create notebook while voice to text is in progress");
                return;
            }
            VNoteMainManager.vNoteCreateFolder();
        }
        onCreateNote: {
            // 初始页面可见时，屏蔽 Ctrl+B
            if (initRect.visible) {
                console.warn("Cannot create note on initial page");
                return;
            }
            // 录音时不允许创建笔记
            if (isRecordingAudio || folderListView.isPlay) {
                console.log("Cannot create note while recording or playing audio");
                return;
            }
            // 语音转文字时不允许创建笔记，避免转文字结果丢失
            if (isVoiceToText) {
                console.log("Cannot create note while voice to text is in progress");
                return;
            }
            VNoteMainManager.createNote();
        }

        onRenameFolder: {
            if (initRect.visible) {
                console.log("No notes available, cannot rename folder");
                return;
            }
            var inSearchResultMode = itemListView.isSearch || itemListView.isSearching || webEngineView.titleBar.isSearching;
            if (inSearchResultMode) {
                console.log("Cannot rename folder in search mode");
                return;
            }
            itemListView.cancelRename();
            folderListView.renameCurrentItem();
        }
        onRenameNote: {
            if (initRect.visible) {
                console.log("No notes available, cannot rename note");
                return;
            }
            folderListView.cancelRename();
            itemListView.renameCurrentItem();
        }
        onShowShortCutView: {
            VNoteMainManager.preViewShortcut(Qt.point(rootWindow.x + rootWindow.width / 2, rootWindow.y + rootWindow.height / 2));
        }
        onStartRecording: {
            if (initRect.visible) {
                console.log("No notes available, cannot start recording");
                return;
            }
            webEngineView.startRecording();
            VoiceRecoderHandler.startRecoder();
        }
        onSaveNote: {
            // 检查是否有笔记可以保存
            if (initRect.visible) {
                console.warn("当前没有打开任何笔记本，不执行保存操作");
                return;
            }

            // 检查笔记列表是否为空
            if (itemListView.model.count === 0) {
                console.warn("当前笔记列表为空，不执行保存操作");
                return;
            }

            // 检查当前笔记ID是否有效
            var currentNoteId = VNoteMainManager.currentNoteId();
            if (currentNoteId <= 0) {
                console.warn("当前没有选中的笔记，不执行保存操作");
                return;
            }

            // Ctrl+S 保存为 txt，只检查是否有文本内容，与右键菜单逻辑保持一致
            if (!VNoteMainManager.hasNoteText(currentNoteId)) {
                console.warn("当前笔记没有文本内容，不执行保存操作");
                return;
            }

            itemListView.onSaveNote();
        }
        onSaveVoice: {
            // JS keydown 已处理；避免 async Loader item 尚未就绪时被二次 open()
            if (webEngineView.activeFocus) {
                return;
            }
            // 检查是否有笔记可以保存
            if (initRect.visible) {
                console.warn("当前没有打开任何笔记本，不执行保存操作");
                return;
            }

            // 检查笔记列表是否为空
            if (itemListView.model.count === 0) {
                console.warn("当前笔记列表为空，不执行保存操作");
                return;
            }

            // 使用WebEngineView组件提供的方法检查当前笔记是否包含录音条目
            webEngineView.checkHasVoiceContent(function(hasVoice) {
                if (hasVoice) {
                    // 将焦点设置回笔记编辑区域，确保保存操作正确执行
                    webEngineView.forceActiveFocus();
                    itemListView.onSaveAudio();
                } else {
                    console.warn("当前笔记中没有录音条目，不执行保存操作");
                }
            });
        }
        onShowJsContextMenu: {
            if (!rootWindow.active) return;
            // 根据焦点分发：笔记列表 > 文件夹列表 > 编辑区
            if (itemListView.activeFocus) {
                itemListView.showContextMenuOnCurrentItem();
                return;
            }
            if (folderListView.activeFocus) {
                folderListView.showContextMenuOnCurrentItem();
                return;
            }
            if (webEngineView.webVisible) {
                webEngineView.showJsContextMenu();
                return;
            }
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
                    icon: foldersData[i].icon,
                    folderId: foldersData[i].folderId
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
            webEngineView.focusWebView();
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
        onNotesDeleted: {
            
            for (var i = 0; i < folderListView.model.count; i++) {
                var folder = folderListView.model.get(i);
                var fidStr = folder.folderId.toString();
                var dec = folderIdToDeletedCount[fidStr];
                if (dec !== undefined) {
                    var oldCount = Number(folder.count);
                    var newCount = oldCount - Number(dec);
                    folder.count = newCount.toString();
                    console.log("QML: Updated folder", folder.name, "count from", oldCount, "to", newCount);
                }
            }
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
        onMoveFinished: function(index, srcFolderIndex, dstFolderIndex) {
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
            // 使用 model.count / model.get 来判定有效索引，避免 view.itemAtIndex 的不确定性
            var count = itemListView.model.count;
            if (count <= 0) {
                itemListView.selectedNoteItem = [];
                itemListView.selectSize = 0;
                return;
            }
            if (minIndex < 0 || minIndex >= count) {
                minIndex = count - 1;
            }
            var modelItem = itemListView.model.get(minIndex);
            if (!modelItem) {
                minIndex = count - 1;
                modelItem = itemListView.model.get(minIndex);
            }
            itemListView.selectedNoteItem = [minIndex];
            itemListView.selectSize = 1;
            VNoteMainManager.vNoteChanged(modelItem.noteId);
        }
        onNoSearchResult: {
            label.visible = false;
            webEngineView.webVisible = false;
            webEngineView.noSearchResult = true;
            webEngineView.titleBar.isSearching = true;
            itemListView.isSearching = true;
        }
        onSearchFinished: {
            label.visible = false;
            webEngineView.webVisible = true;
            webEngineView.noSearchResult = false;
            webEngineView.titleBar.isSearching = true;
            itemListView.isSearching = true;
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
        onSelectNoteByIndex: function(selectIndex) {
            itemListView.selectNoteItem(selectIndex);
        }
        onVoiceToTextStateChanged: function(isConverting) {
            isVoiceToText = isConverting;
        }
    }

    // 添加录音状态监听
    Connections {
        target: VoiceRecoderHandler

        onRecoderStateChange: function(type) {
            // 录音中或录音暂停时都视为"正在录音"状态，禁用相关操作
            isRecordingAudio = (type === VoiceRecoderHandler.Recording || type === VoiceRecoderHandler.Paused);
            console.log("MainWindow: Recording state changed to:", type, "isRecordingAudio:", isRecordingAudio);
        }
    }

    IconLabel {
        id: appImage

        anchors {
            top: parent.top
            topMargin: 7
            left: parent.left
            leftMargin: 10
        }
        height: 32
        icon.height: 32
        icon.name: "deepin-voice-note"
        icon.width: 32
        width: 32
        z: 100
    }

    VNoteComponents.VNoteToolButton {
        id: twoColumnModeBtn

        anchors {
            top: parent.top
            topMargin: 10
            left: appImage.right
            leftMargin: 19
        }
        icon.height: 16
        icon.name: "sidebar"
        icon.width: 16
        height: 30
        width: 30
        visible: !(needHideSearch && search.visible) || leftBgArea.visible
        z: 100

        onClicked: {
            toggleTwoColumnMode();
        }
    }

    // 全窗级毛玻璃：左侧文件夹栏 + 笔记列表区域透明可见，列表项保持实色卡片
    VNoteComponents.SidebarBlurBackground {
        anchors.fill: parent
        windowControl: rootWindow
        z: 0
    }

    Item {
        id: rowLayout

        z: 1
        anchors.fill: parent

        Rectangle {
            id: leftBgArea

            height: parent.height
            width: leftViewWidth - 5
            clip: true
            color: "transparent"

            ColumnLayout {
                id: leftColumnLayout

                anchors.bottomMargin: 10
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 5
                anchors.topMargin: 50

                FolderListView {
                    id: folderListView

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    enabled: !rootWindow.isVoiceToText && !itemListView.isSearching && !webEngineView.titleBar.isSearching
                    opacity: enabled ? 1.0 : 0.4
                    scrollBarParent: scrollBarOverlay
                    scrollBarRightAnchor: leftDragHandle.x + leftDragHandle.width - 3
                    webVisible: initRect.visible
                    isRecordingAudio: rootWindow.isRecordingAudio  // 传递录音状态
                    isVoiceToText: rootWindow.isVoiceToText  // 传递语音转文字状态

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
                    onMouseChanged: function(mousePosX, mousePosY) {
                        // 获取 leftBgArea 的全局位置
                        var leftAreaY = leftBgArea.mapToGlobal(0, 0).y;
                        // folderListView 在 ColumnLayout 中，ColumnLayout 有 topMargin: 50
                        var folderListTop = leftAreaY + 50;
                        var folderListBottom = folderListTop + folderListView.height;
                        
                        // 判断滚动条件
                        if (mousePosY < folderListTop) {
                            folderListView.rollUp();
                        } else if (mousePosY > folderListBottom) {
                            folderListView.rollDown();
                        } else {
                            folderListView.rollStop();
                        }
                    }
                }

                VNoteComponents.VNoteButton {
                    id: createFolderButton

                    Layout.fillWidth: true
                    Layout.preferredHeight: createFolderBtnHeight
                    enabled: !isRecordingAudio && !folderListView.isPlay && !webEngineView.titleBar.isSearching && !rootWindow.isVoiceToText
                    text: qsTr("Create Notebook")

                    onClicked: {
                        folderListView.addFolder();
                    }
                }
            }

            Connections {
                target: itemListView

                onDeleteNotes: {
                    // 数量更新统一依赖后端 onNotesDeleted(folderId->count)
                }
                onDropRelease: {
                    if (rootWindow.isRecordingAudio || webEngineView.titleBar.isPlaying || folderListView.isPlay || rootWindow.isVoiceToText) {
                        console.log("MainWindow: Drop ignored while recording, playing or voice to text");
                        return;
                    }
                    var indexList = [];
                    for (var i = 0; i < itemListView.selectedNoteItem.length; i++) {
                        indexList.push(itemListView.model.get(itemListView.selectedNoteItem[i]).noteId);
                    }
                    folderListView.dropItems(indexList);
                }
                onMouseChanged: function(mousePosX, mousePosY) {
                    // 获取 leftBgArea 的全局位置
                    var leftAreaY = leftBgArea.mapToGlobal(0, 0).y;
                    // folderListView 在 ColumnLayout 中，ColumnLayout 有 topMargin: 50
                    var folderListTop = leftAreaY + 50;
                    var folderListBottom = folderListTop + folderListView.height;
                    
                    // 判断滚动条件
                    if (mousePosY < folderListTop) {
                        folderListView.rollUp();
                    } else if (mousePosY > folderListBottom) {
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

            x: leftBgArea.width
            height: parent.height
            width: 5
            color: "transparent"

            Rectangle {
                anchors.right: parent.right
                color: DTK.themeType === ApplicationHelper.LightType ? "#eee7e7e7" : "black"
                height: parent.height
                width: 1
            }

            MouseArea {
                property real pressGlobalX: 0

                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor

                onPressed: function(mouse) {
                    pressGlobalX = mapToItem(null, mouse.x, 0).x;
                }
                onPositionChanged: function(mouse) {
                    if (!pressed) return;
                    var globalX = mapToItem(null, mouse.x, 0).x;
                    var delta = globalX - pressGlobalX;
                    pressGlobalX = globalX;

                    var maxX = rootWindow.width > (leftAreaMaxWidth + middleAreaMinWidth + rightAreaMinWidth - leftDragHandle.width) ? leftAreaMaxWidth : rootWindow.width - (middleAreaMinWidth + rightAreaMinWidth);
                    var newWidth = Math.max(leftAreaMinWidth, Math.min(maxX, leftBgArea.width + delta));
                    var actualDelta = newWidth - leftBgArea.width;

                    if (actualDelta > 0) {
                        if ((middleBgArea.width + rightDragHandle.width) > middleAreaMinWidth) {
                            middleBgArea.width -= actualDelta;
                        }
                    } else if (actualDelta < 0) {
                        middleBgArea.width -= actualDelta;
                    }
                    leftBgArea.width = newWidth;
                    tmpLeftAreaWidth = newWidth;
                }
            }
        }

        Rectangle {
            id: middleBgArea

            x: leftBgArea.width + leftDragHandle.width
            height: parent.height
            width: middleViewWidth - 5
            clip: true
            color: DTK.themeType === ApplicationHelper.LightType ? Qt.rgba(248 / 255, 248 / 255, 248 / 255, 0.95)
                                                                   : Qt.rgba(24 / 255, 24 / 255, 24 / 255, 0.95)

            onWidthChanged: {
                if (!leftBgArea.visible) {
                    if (width >= 240) {
                        search.visible = true;
                        needHideSearch = false;
                        search.offect = collapsedSearchOffset();
                    } else {
                        search.visible = false;
                        needHideSearch = true;
                        search.offect = 50;
                    }
                }
            }

            ColumnLayout {
                id: middleColumnLayout

                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 5
                spacing: 10

                VNoteComponents.VNoteToolButton {
                    Layout.alignment: Text.AlignRight
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 30
                    Layout.rightMargin: 10
                    Layout.topMargin: 10
                    display: AbstractButton.IconOnly
                    icon.height: 16
                    icon.name: "action_search"
                    icon.width: 16
                    height: 30
                    width: 30
                    visible: !search.visible

                    onClicked: {
                        search.visible = true;
                        search.offect = leftBgArea.visible ? 50 : collapsedSearchOffset();
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
                        itemListView.isSearch = false;
                        itemListView.isSearching = false;
                        // 退出搜索时，只有当前记事本有笔记时才显示富文本编辑器
                        webEngineView.webVisible = (itemListView.model.count > 0);
                        webEngineView.noSearchResult = false;
                        webEngineView.titleBar.isSearching = false;
                        VNoteMainManager.clearSearch();
                        // 退出搜索时清空输入框
                        search.text = "";
                        if (needHideSearch)
                            search.visible = false;
                    }



                    Layout.fillWidth: true
                    Layout.leftMargin: offect
                    Layout.preferredHeight: (DTK.fontManager.t6.pixelSize > 18)
                                           ? Math.max(36, Math.ceil(DTK.fontManager.t6.pixelSize * 1.6))
                                           : 30
                    Layout.topMargin: 12
                    enabled: !isRecordingAudio && !folderListView.isPlay && !rootWindow.isVoiceToText
                    placeholder: qsTr("Search")
                    topPadding: 0
                    bottomPadding: 0

                    Keys.onPressed: function(event) {
                        if (event.key === Qt.Key_Escape) {
                            exitSearch();
                            return;
                        }
                        if (text.length === 0)
                            return;
                        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                            VNoteMainManager.vNoteSearch(text);
                        }
                    }
                    onActiveFocusChanged: {
                        if (!activeFocus) {
                            // 失去焦点时，保留用户输入的文本，不自动清空
                            // 用户输入内容后可能只是暂时切换焦点，不应该清空已输入的内容
                            // 清空操作应该由用户主动触发（点击清空按钮或按ESC）
                            
                            if (needHideSearch && text.length === 0)
                                search.visible = false;
                        }
                    }
                    onTextChanged: {
                        if (text.length === 0) {
                            folderListView.toggleSearch(false);
                            if (itemListView.searchLoader.active) {
                                itemListView.searchLoader.item.visible = false;
                            }
                            itemListView.view.visible = true;
                            label.visible = true;
                            itemListView.isSearch = false;
                            itemListView.isSearching = false;
                            // 清空搜索文本时，只有当前记事本有笔记时才显示富文本编辑器
                            webEngineView.webVisible = (itemListView.model.count > 0);
                            webEngineView.noSearchResult = false;
                            webEngineView.titleBar.isSearching = false;
                            VNoteMainManager.clearSearch();
                        }
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
                    enabled: !rootWindow.isVoiceToText
                    opacity: enabled ? 1.0 : 0.4
                    scrollBarParent: itemScrollBarOverlay
                    scrollBarRightAnchor: rightDragHandle.x + rightDragHandle.width - 2
                    sourceFolderModel: folderListView.model
                    currentFolderIndex: folderListView.currentFolderIndex
                    webVisible: initRect.visible
                    isRecordingAudio: rootWindow.isRecordingAudio
                    isVoiceToText: rootWindow.isVoiceToText

                    onDeleteFinished: {
                        // 只有当列表不为空时才调用 toggleMultCho，避免覆盖 emptyItemList 设置的 webVisible = false
                        if (itemListView.model.count > 0) {
                            webEngineView.toggleMultCho(1);
                        }
                    }
                    onEmptyItemList: {
                        webEngineView.webVisible = false;
                    }
                    onNoteItemChanged: {
                        VNoteMainManager.vNoteChanged(index);
                    }
                }
            }
        }

        Rectangle {
            id: rightDragHandle

            x: middleBgArea.x + middleBgArea.width
            height: parent.height
            width: 5
            color: middleBgArea.color

            Component.onCompleted: {
                tmprightDragX = rightDragHandle.x;
            }

            MouseArea {
                id: rightMouseArea

                property real pressGlobalX: 0

                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor

                onPressed: function(mouse) {
                    pressGlobalX = mapToItem(null, mouse.x, 0).x;
                }
                onPositionChanged: function(mouse) {
                    if (!pressed) return;
                    var globalX = mapToItem(null, mouse.x, 0).x;
                    var delta = globalX - pressGlobalX;
                    pressGlobalX = globalX;

                    var maxX = rootWindow.width - rightAreaMinWidth;
                    var minX = leftBgArea.visible ? (leftAreaMinWidth + middleAreaMinWidth + leftDragHandle.width) : (middleAreaMinWidth - rightDragHandle.width);
                    var targetHandleX = rightDragHandle.x + delta;
                    targetHandleX = Math.max(minX, Math.min(maxX, targetHandleX));

                    var newMiddleWidth = targetHandleX - middleBgArea.x;
                    if (newMiddleWidth < (middleAreaMinWidth - rightDragHandle.width)) {
                        var shrinkWidth = middleAreaMinWidth - newMiddleWidth - rightDragHandle.width;
                        middleBgArea.width = middleAreaMinWidth - rightDragHandle.width;
                        leftBgArea.width = leftBgArea.width - shrinkWidth;
                        tmpLeftAreaWidth = leftBgArea.width;
                    } else {
                        middleBgArea.width = newMiddleWidth;
                    }
                    tmprightDragX = middleBgArea.width;
                    tmpWebViewWidth = rightBgArea.width;
                    if (search.activeFocus) {
                        middleBgArea.forceActiveFocus();
                    }
                }
            }
        }

        Rectangle {
            id: rightBgArea

            x: rightDragHandle.x + rightDragHandle.width
            height: parent.height
            width: parent.width - x
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
                    isRecordingAudio: rootWindow.isRecordingAudio
                    isVoiceToText: rootWindow.isVoiceToText  // 传递语音转文字状态

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

    Item {
        id: scrollBarOverlay

        function updateGeometry() {
            y = folderListView.mapToItem(parent, 0, 0).y;
        }

        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        height: folderListView.height
        visible: leftBgArea.visible
        z: 50

        Component.onCompleted: Qt.callLater(updateGeometry)
        onHeightChanged: Qt.callLater(updateGeometry)
        onVisibleChanged: Qt.callLater(updateGeometry)

        Connections {
            target: folderListView

            function onHeightChanged() {
                Qt.callLater(scrollBarOverlay.updateGeometry);
            }
            function onYChanged() {
                Qt.callLater(scrollBarOverlay.updateGeometry);
            }
        }

        Connections {
            target: leftBgArea

            function onYChanged() {
                Qt.callLater(scrollBarOverlay.updateGeometry);
            }
        }

        Connections {
            target: leftColumnLayout

            function onYChanged() {
                Qt.callLater(scrollBarOverlay.updateGeometry);
            }
        }
    }

    Item {
        id: itemScrollBarOverlay

        function updateGeometry() {
            y = itemListView.mapToItem(parent, 0, 0).y;
        }

        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        height: itemListView.height
        visible: middleBgArea.visible
        z: 50

        Component.onCompleted: Qt.callLater(updateGeometry)
        onHeightChanged: Qt.callLater(updateGeometry)
        onVisibleChanged: Qt.callLater(updateGeometry)

        Connections {
            target: itemListView

            function onHeightChanged() {
                Qt.callLater(itemScrollBarOverlay.updateGeometry);
            }
            function onYChanged() {
                Qt.callLater(itemScrollBarOverlay.updateGeometry);
            }
        }

        Connections {
            target: middleBgArea

            function onYChanged() {
                Qt.callLater(itemScrollBarOverlay.updateGeometry);
            }
        }

        Connections {
            target: middleColumnLayout

            function onYChanged() {
                Qt.callLater(itemScrollBarOverlay.updateGeometry);
            }
        }

        Connections {
            target: hideLeftArea

            function onFinished() {
                Qt.callLater(itemScrollBarOverlay.updateGeometry);
            }
        }

        Connections {
            target: showLeftArea

            function onFinished() {
                Qt.callLater(itemScrollBarOverlay.updateGeometry);
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

        onStarted: {
            needHideSearch = false;
            if (middleBgArea.width < 240) {
                needHideSearch = true;
                search.visible = false;
            }
            tmpWebViewWidth = rightBgArea.width;
        }
        onFinished: {
            leftBgArea.visible = false;
            leftDragHandle.visible = false;
            tmprightDragX = middleBgArea.width;
        }

        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
            from: leftBgArea.width
            property: "width"
            target: leftBgArea
            to: 0
        }

        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
            from: leftDragHandle.width
            property: "width"
            target: leftDragHandle
            to: 0
        }

        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
            from: search.offect
            property: "offect"
            target: search
            to: collapsedSearchOffset()
        }
    }

    ParallelAnimation {
        id: showLeftArea

        property int targetMiddleWidth: 0

        onStarted: {
            leftBgArea.visible = true;
            leftDragHandle.visible = true;

            var totalNeeded = tmpLeftAreaWidth + 5 + middleBgArea.width + rightDragHandle.width;
            var rightAvailable = rowLayout.width - totalNeeded;
            if (rightAvailable < rightAreaMinWidth) {
                var deficit = rightAreaMinWidth - rightAvailable;
                targetMiddleWidth = Math.max(middleAreaMinWidth - rightDragHandle.width,
                                             middleBgArea.width - deficit);
            } else {
                targetMiddleWidth = middleBgArea.width;
            }
        }
        onFinished: {
            search.visible = true;
            search.offect = 0;
            tmpWebViewWidth = rightBgArea.width;
        }

        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
            from: leftBgArea.width
            property: "width"
            target: leftBgArea
            to: tmpLeftAreaWidth
        }

        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
            from: leftDragHandle.width
            property: "width"
            target: leftDragHandle
            to: 5
        }

        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
            from: middleBgArea.width
            property: "width"
            target: middleBgArea
            to: showLeftArea.targetMiddleWidth
        }

        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
            from: search.offect
            property: "offect"
            target: search
            to: 0
        }
    }
}
