// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebChannel 1.15
// Use Qt5 version 1.10 for compatibility with both Qt5 and Qt6
// Qt6 is backward compatible with Qt5 import syntax
import QtWebEngine 1.10
import Qt.labs.platform 1.1
import VNote 1.0
import "../dialog"
import org.deepin.dtk 1.0

Item {
    id: rootItem

    property bool hasScroll: false
    property bool initialVisible: false
    property bool isRecording: false
    property bool isRecordingAudio: false 
    property bool noSearchResult: false
    property bool webVisible: true
    property alias titleBar: title

    signal deleteNote
    signal moveNote
    signal openSetting
    signal playStateChange(bool state)
    signal saveAudio
    signal saveNote

    function copy() {
        webView.triggerWebAction(5);
    }

    function showJsContextMenu() {
        // 仅在编辑区可见时尝试弹出；是否在编辑器内由JS自行判断
        if (webVisible) {
            // 汇总选区类型、必要信息、以及光标/元素坐标
            var js = "(function(){\n" +
                     "  function isInsideEditable(node){\n" +
                     "    var el = node && (node.nodeType === 1 ? node : node.parentElement);\n" +
                     "    return !!(el && el.closest && el.closest('.note-editable'));\n" +
                     "  }\n" +
                     "  var sel = window.getSelection();\n" +
                     "  if(!sel || sel.rangeCount === 0){ return null; }\n" +
                     "  var range = sel.getRangeAt(0);\n" +
                     "  var collapsed = range.collapsed;\n" +
                     "  var inEditable = isInsideEditable(range.commonAncestorContainer || sel.anchorNode);\n" +
                     "  var hasSelection = !collapsed && sel.toString().length > 0;\n" +
                     "  var flags = {\n" +
                     "    canSelectAll: !!inEditable,\n" +
                     "    canCopy: hasSelection,\n" +
                     "    canCut: hasSelection && inEditable,\n" +
                     "    canDelete: hasSelection && inEditable,\n" +
                     "    canPaste: !!inEditable\n" +
                     "  };\n" +
                     "  // 若不在可编辑区内，直接返回空\n" +
                     "  if (!inEditable) { return null; }\n" +
                     "  // 先判断是否为语音/图片选区\n" +
                     "  var kind = 2;\n" +
                     "  var info = '';\n" +
                     "  try {\n" +
                     "    var selected = window.isRangeVoice && window.isRangeVoice();\n" +
                     "    if (selected && typeof selected.flag === 'number') {\n" +
                     "      kind = selected.flag;\n" +
                     "      info = selected.info || '';\n" +
                     "    }\n" +
                     "  } catch(e) {}\n" +
                     "  var rect = null;\n" +
                     "  if (kind === 0) {\n" +
                     "    // 图片：取选区中的第一张图片的矩形\n" +
                     "    var testDiv = (function(){ var s=window.getSelection(); var r=s.getRangeAt(0); var f=r.cloneContents(); var d=document.createElement('div'); d.appendChild(f); return d; })();\n" +
                     "    var img = testDiv.querySelector('img');\n" +
                     "    if (img) rect = img.getBoundingClientRect();\n" +
                     "  } else if (kind === 1) {\n" +
                     "    // 语音：优先取当前激活语音块的位置\n" +
                     "    var voice = document.querySelector('.li.active .voiceBox') || document.querySelector('.voiceBox');\n" +
                     "    if (voice) rect = voice.getBoundingClientRect();\n" +
                     "  }\n" +
                     "  if (!rect) {\n" +
                     "    if (collapsed) {\n" +
                     "      var span = document.createElement('span');\n" +
                     "      span.textContent = '\\u200b';\n" +
                     "      range.insertNode(span);\n" +
                     "      rect = span.getBoundingClientRect();\n" +
                     "      span.parentNode && span.parentNode.removeChild(span);\n" +
                     "    } else {\n" +
                     "      rect = range.getBoundingClientRect();\n" +
                     "    }\n" +
                     "  }\n" +
                     "  if (!rect) return null;\n" +
                     "  return { type: kind, json: info, x: Math.round(rect.left), y: Math.round(rect.bottom), flags: flags };\n" +
                     "})()";
            webView.runJavaScript(js, function(result) {
                if (!result) {
                    // 回退：弹出文本菜单于编辑区中心
                    ActionManager.resetCtxMenu(ActionManager.TxtCtxMenu, true);
                    var fx = Math.max(4, Math.floor(webView.width / 2));
                    var fy = Math.max(4, Math.floor(webView.height / 2));
                    txtCtxMenu.popup(Qt.point(fx, fy));
                    return;
                }
                var px = Math.max(4, Math.min(webView.width - 4, result.x));
                var py = Math.max(4, Math.min(webView.height - 4, result.y));
                if (result.type === 0) {
                    // 图片菜单
                    handler.onSaveMenuParam(0, result.json);
                    picturCtxMenu.popup(Qt.point(px, py));
                    return;
                }
                if (result.type === 1) {
                    // 语音菜单
                    handler.onSaveMenuParam(1, result.json);
                    voiceCtxMenu.popup(Qt.point(px, py));
                    return;
                }
                // 文本菜单：根据 flags 设置可用性
                ActionManager.resetCtxMenu(ActionManager.TxtCtxMenu, false);
                if (result.flags) {
                    ActionManager.enableAction(ActionManager.TxtSelectAll, !!result.flags.canSelectAll);
                    ActionManager.enableAction(ActionManager.TxtCopy, !!result.flags.canCopy);
                    ActionManager.enableAction(ActionManager.TxtCut, !!result.flags.canCut);
                    ActionManager.enableAction(ActionManager.TxtPaste, !!result.flags.canPaste);
                    ActionManager.enableAction(ActionManager.TxtDelete, !!result.flags.canDelete);
                    ActionManager.enableAction(ActionManager.TxtSpeech, !!result.flags.canCopy);
                    ActionManager.enableAction(ActionManager.TxtDictation, !!result.flags.canPaste);
                }
                txtCtxMenu.popup(Qt.point(px, py));
            });
        }
    }

    function startRecording() {
        if (VNoteMainManager.isInSearchMode()) {
            console.log("Cannot show recording UI while in search mode");
            return;
        }
        
        if (!recorderViewLoader.active) {
            recorderViewLoader.active = true;
        } else {
            recorderViewLoader.item.visible = true;
        }
        isRecording = true;
        title.recorderBtnEnable = false;
    }

    function stopAndClose() {
        recorderViewLoader.item.stop();
    }

    function stopTTS() {
        ActionManager.actionTriggerFromQuick(37);
    }
    
    // 检查当前笔记是否包含录音条目
    function checkHasVoiceContent(callback) {
        if (!webVisible || !webView) {
            callback(false);
            return;
        }
        
        webView.runJavaScript("hasVoice()", function(result) {
            callback(result);
        });
    }

    function toggleMultCho(choices) {
        if (choices > 1) {
            webVisible = false;
            webRect.visible = false;
            if (!multipleChoicesLoader.active) {
                multipleChoicesLoader.active = true;
            }
            multipleChoicesLoader.visible = true;
            multipleChoicesLoader.item.visible = true;
            multipleChoicesLoader.item.selectSize = choices;
        } else {
            multipleChoicesLoader.visible = false;
            webVisible = true;
            webRect.visible = true;
            multipleChoicesLoader.item.visible = false;
        }
    }

    visible: true

    onNoSearchResultChanged: {
        webRect.visible = !noSearchResult;
    }

    ColumnLayout {
        id: columnLayout

        anchors.fill: parent
        spacing: 0

        WindowTitleBar {
            id: title

            Layout.fillWidth: true
            imageBtnEnable: webVisible
            isInitialInterface: initialVisible
            isRecordingAudio: rootItem.isRecordingAudio 

            onTitleOpenSetting: {
                rootItem.openSetting();
            }
        }

        Rectangle {
            id: noSearchRect

            Layout.fillHeight: true
            Layout.fillWidth: true
            // 设置基础背景颜色，和 web 前端共同实现背景色
            color: webView.backgroundColor
            visible: noSearchResult

            DciIcon {
                id: noSearchIcon

                anchors.centerIn: noSearchRect
                name: "search_no_results"
                theme: DTK.themeType
                visible: noSearchResult
            }
        }

        Rectangle {
            id: webRect

            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "transparent"

            WebEngineView {
                id: webView

                anchors.fill: parent
                // 设置基础背景颜色，和 web 前端共同实现背景色
                backgroundColor: DTK.themeType === ApplicationHelper.LightType ? "white" : "black"
                visible: webVisible

                Component.onCompleted: {
                    noteWebChannel.registerObject("webobj", Webobj);
                    // console.log("registerObject ret: " + ret)
                    webView.webChannel = noteWebChannel;
                    webView.url = Qt.resolvedUrl(Webobj.webPath());

                    // 隐藏浮动工具栏
                    Webobj.calllJsShowEditToolbar(0, 0);
                }
                onContextMenuRequested: req => {
                    // 初始页可见或编辑器不可见时，屏蔽右键菜单
                    if (initialVisible || !webVisible) {
                        req.accepted = true;
                        return;
                    }
                    // 仅当菜单是文本类型时，我们才在QML层根据上下文标记(editFlags)更新状态
                    // 这是为了精确修复Qt5下文本菜单状态不更新的问题
                    if (handler.menuTypeFromJs === WebEngineHandler.TxtMenu) {
                        var flags = req.editFlags
                        ActionManager.resetCtxMenu(ActionManager.TxtCtxMenu, false) // false表示禁用全部，然后逐一启用
                        ActionManager.enableAction(ActionManager.TxtSelectAll, (flags & 1) !== 0)
                        ActionManager.enableAction(ActionManager.TxtCopy, (flags & 2) !== 0)
                        ActionManager.enableAction(ActionManager.TxtCut, (flags & 4) !== 0)
                        ActionManager.enableAction(ActionManager.TxtPaste, (flags & 8) !== 0)
                        ActionManager.enableAction(ActionManager.TxtDelete, (flags & 16) !== 0)
                        // 只有能复制时，才能朗读
                        ActionManager.enableAction(ActionManager.TxtSpeech, (flags & 2) !== 0)
                        // 只有能粘贴时，才能听写
                        ActionManager.enableAction(ActionManager.TxtDictation, (flags & 8) !== 0)
                    }

                    // 对于所有菜单类型，都调用C++的处理器
                    // C++将处理特殊逻辑(如解析语音路径)并最终弹出菜单
                    handler.onContextMenuRequested(req)

                    // 阻止默认菜单弹出
                    req.accepted = true;
                }
                onJavaScriptConsoleMessage: {
                    // 调试使用，打印控制台输出
                    console.debug("--- from web: ", message, sourceID, lineNumber);
                }

                DropArea {
                    anchors.fill: parent

                    onDropped: function(drop) {
                        if (drop.hasUrls) {
                            VNoteMainManager.insertImages(drop.urls);
                        }
                    }
                }

                WebEngineHandler {
                    id: handler

                    target: webView

                    onLoadRichText: {
                        VNoteMainManager.vNoteChanged(itemListView.model.get(0).noteId);
                        itemListView.selectedNoteItem = [0];
                        itemListView.selectSize = 1;
                    }
                    onPlayingVoice: isPlay => {
                        playStateChange(isPlay);
                        title.isPlaying = isPlay;
                    }
                    onPopupToast: (message, msgId) => {
                        DTK.sendMessage(webView, message, "icon_warning", 4000, msgId);
                    }
                    onRequesetCallJsSynchronous: func => {
                        webView.runJavaScript(func, function (result) {
                            onCallJsResult(result);
                        });
                    }
                    onRequestMessageDialog: type => {
                        // 触发创建提示对话框
                        messageDialogLoader.showDialog(type);
                    }
                    onTriggerWebAction: action => {
                        webView.triggerWebAction(action);
                    }
                    onViewPicture: filePath => {
                        viewPictureLoader.path = filePath;
                        if (!viewPictureLoader.active)
                            viewPictureLoader.active = true;
                        else
                            viewPictureLoader.item.show();
                    }
                    onSaveAudio: {
                        rootItem.saveAudio();
                    }
                    onCreateNote: {
                        if (!initialVisible) {
                            VNoteMainManager.createNote();
                        }
                    }
                }

                WebChannel {
                    id: noteWebChannel

                }
            }
        }

        Loader {
            id: multipleChoicesLoader

            Layout.fillHeight: parent.height
            Layout.fillWidth: parent.width
            visible: false

            sourceComponent: MultipleChoices {
                anchors.fill: parent

                onDeleteNote: {
                    rootItem.deleteNote();
                }
                onMoveNote: {
                    rootItem.moveNote();
                }
                onSaveAudio: {
                    rootItem.saveAudio();
                }
                onSaveNote: {
                    rootItem.saveNote();
                }
            }
        }
    }

    Rectangle {
        id: line

        color: hasScroll ? (DTK.themeType === ApplicationHelper.LightType ? "#14000000" : "#66000000") : DTK.themeType === ApplicationHelper.LightType ? "white" : "#242424"
        height: 1 / Screen.devicePixelRatio
        width: webRect.width
        y: 50
    }

    VNoteMessageDialogLoader {
        id: messageDialogLoader

    }

    Loader {
        id: viewPictureLoader

        property string path: ""

        asynchronous: true

        sourceComponent: ViewPictureDialog {
            id: viewPictureWindow

            filePath: path
        }

        onActiveChanged: {
            if (active)
                viewPictureWindow.show();
        }
    }

    Loader {
        asynchronous: true

        VNoteRightMenu {
            id: picturCtxMenu

            menuType: ActionManager.PictureCtxMenu

            onAboutToShow: {
                // 对于图片菜单，我们恢复原始逻辑：在菜单显示前，重置所有项目为可用
                // C++后端会处理图片路径等特殊逻辑
                ActionManager.resetCtxMenu(ActionManager.PictureCtxMenu, true);
            }

            Connections {
                target: handler

                onRequestShowMenu: (type, pos) => {
                    if (type === WebEngineHandler.PictureMenu) {
                        picturCtxMenu.popup(pos);
                    }
                }
            }
        }
    }

    Loader {
        asynchronous: true

        VNoteRightMenu {
            id: voiceCtxMenu

            menuType: ActionManager.VoiceCtxMenu

            onAboutToShow: {
                // 对于语音菜单，恢复原始逻辑：重置所有项目为可用，
                // 并根据全局状态单独设置"语音转文字"的可用性
                ActionManager.resetCtxMenu(ActionManager.VoiceCtxMenu, true);
                var isConverting = VNoteMainManager.isVoiceToText();
                ActionManager.enableAction(ActionManager.VoiceToText, !isConverting);
            }

            Connections {
                target: handler

                onRequestShowMenu: (type, pos) => {
                    if (type === WebEngineHandler.VoiceMenu) {
                        voiceCtxMenu.popup(pos);
                    }
                }
            }
        }
    }

    Loader {
        asynchronous: true

        VNoteRightMenu {
            id: txtCtxMenu

            menuType: ActionManager.TxtCtxMenu

            onAboutToShow: {
                // 文本菜单的状态现在完全由 onContextMenuRequested 处理，
                // 此处不再需要任何逻辑。
            }

            Connections {
                target: handler

                onRequestShowMenu: (type, pos) => {
                    if (type === WebEngineHandler.TxtMenu) {
                        txtCtxMenu.popup(pos);
                    }
                }
            }
        }
    }

    Loader {
        id: selectImgLoader

        active: false
        asynchronous: true

        sourceComponent: FileDialog {
            id: fileDialog

            fileMode: FileDialog.OpenFiles
            folder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
            nameFilters: ["Image file(*.jpg *.png *.bmp)"]

            Component.onCompleted: {
                fileDialog.open();
            }
            onAccepted: {
                if (fileDialog.files.length > 0) {
                    VNoteMainManager.insertImages(fileDialog.files);
                }
            }
        }
    }

    Loader {
        id: recorderViewLoader
            z: 1000

        function onPauseRecording() {
            VoiceRecoderHandler.pauseRecoder();
        }

        function onStopRecording() {
            isRecording = false;
            VoiceRecoderHandler.stopRecoder();
            title.recorderBtnEnable = true;
            title.isRecording = false;
        }

        active: false
        anchors.bottom: rootItem.bottom
        anchors.horizontalCenter: rootItem.horizontalCenter
        asynchronous: true
        height: 70
        width: rootItem.width

        sourceComponent: RecordingView {
            id: recordingBar

            anchors.fill: parent

            Component.onCompleted: {
                recordingBar.visible = true;
            }
        }

        onLoaded: {
            recorderViewLoader.item.pauseRecording.connect(onPauseRecording);
            recorderViewLoader.item.stopRecording.connect(onStopRecording);
            recorderViewLoader.item.forceActiveFocus();
        }
    }

    Connections {
        target: title

        onCreateNote: {
            if (!initialVisible) {
                VNoteMainManager.createNote();
            }
        }
        onInsertImage: {
            if (!selectImgLoader.active) {
                selectImgLoader.active = true;
            } else {
                selectImgLoader.item.open();
            }
        }
        onStartRecording: {
            VoiceRecoderHandler.startRecoder();
        }
    }

    Connections {
        target: VNoteMainManager

        onNeedUpdateNote: {
            webView.runJavaScript("getHtml()", function (result) {
                VNoteMainManager.updateNoteWithResult(result);
            });
        }
        onScrollChange: isTop => {
            hasScroll = !isTop;
        }
        onUpdateRichTextSearch: key => {
            webView.findText(key);
        }
    }

    Connections {
        target: VoiceRecoderHandler

        onRecoderStateChange: {
            var currentType = VoiceRecoderHandler.getRecoderType();
            if (recorderViewLoader.item) {
                recorderViewLoader.item.isRecording = (currentType === VoiceRecoderHandler.Recording);
            }
            
            // 当录音状态变为Idle时，完全关闭录音界面并重置状态
            if (currentType === VoiceRecoderHandler.Idle) {
                isRecording = false;
                title.recorderBtnEnable = true;
                title.isRecording = false;
                
                // 完全关闭录音界面
                if (recorderViewLoader.active && recorderViewLoader.item) {
                    recorderViewLoader.item.visible = false;
                    recorderViewLoader.item.time = "00:00:00";
                }
                // 延迟销毁Loader以确保动画完成
                Qt.callLater(function() {
                    recorderViewLoader.active = false;
                });
            }
        }
        onUpdateRecordBtnState: {
            title.recorderBtnEnable = enable;
        }
        onUpdateRecorderTime: {
            recorderViewLoader.item.time = time;
        }
        onVolumeTooLow: isLow => {
            if (isLow) {
                messageDialogLoader.showDialog(VNoteMessageDialogHandler.VolumeTooLow, ret => {
                    if (ret) {
                        startRecording();
                        VoiceRecoderHandler.confirmStartRecoder();
                    }
                });
            } else {
                startRecording();
            }
        }
    }
}
