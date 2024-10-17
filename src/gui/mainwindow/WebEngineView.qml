// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebChannel 1.15
import QtWebEngine 1.15
import Qt.labs.platform
import org.deepin.dtk 1.0
import VNote 1.0
import "../dialog"

Item {
    id: rootItem

    property bool hasScroll: false
    property bool noSearchResult: false
    property bool webVisible: true

    signal deleteNote
    signal moveNote
    signal saveAudio
    signal saveNote

    function startRecording() {
        if (!recorderViewLoader.active) {
            recorderViewLoader.active = true;
        } else {
            recorderViewLoader.item.visible = true;
        }
        VoiceRecoderHandler.startRecoder();
        title.recorderBtnEnable = false;
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
        }

        Rectangle {
            id: noSearchRect

            Layout.fillHeight: true
            Layout.fillWidth: true
            // 设置基础背景颜色，和 web 前端共同实现背景色
            color: webView.backgroundColor
            visible: noSearchResult

            IconLabel {
                id: noSearchIcon

                anchors.centerIn: noSearchRect
                icon.source: "qrc:/dsg/icons/search_no_results.dci"
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
                    // 响应右键菜单，处理完成后 handler 抛出 requestShowMenu() 信号
                    handler.onContextMenuRequested(req);
                }
                onJavaScriptConsoleMessage: {
                    // 调试使用，打印控制台输出
                    console.debug("--- from web: ", message, sourceID, lineNumber);
                }

                WebEngineHandler {
                    id: handler

                    target: webView

                    onLoadRichText: {
                        VNoteMainManager.vNoteChanged(itemListView.model.get(0).noteId);
                        itemListView.selectedNoteItem = [0];
                        itemListView.selectSize = 1;
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
                }

                WebChannel {
                    id: noteWebChannel

                }
            }

            Rectangle {
                anchors.top: webRect.top
                border.width: 0
                color: hasScroll ? (DTK.themeType === ApplicationHelper.LightType ? "#0C000000" : "#0CFFFFFF") : "transparent"
                height: 1
                width: parent.width
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

    VNoteMessageDialogLoader {
        id: messageDialogLoader

    }

    Loader {
        asynchronous: true

        VNoteRightMenu {
            id: picturCtxMenu

            menuType: ActionManager.PictureCtxMenu

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

        function onPauseRecording() {
            VoiceRecoderHandler.pauseRecoder();
        }

        function onStopRecording() {
            VoiceRecoderHandler.stopRecoder();
            title.recorderBtnEnable = true;
        }

        active: false
        anchors.bottom: rootItem.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: rootItem.horizontalCenter
        asynchronous: true
        height: 42
        width: 364

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
        }
    }

    Connections {
        target: title

        onCreateNote: {
            VNoteMainManager.createNote();
        }
        onInsertImage: {
            if (!selectImgLoader.active) {
                selectImgLoader.active = true;
            } else {
                selectImgLoader.item.open();
            }
        }
        onStartRecording: {
            startRecording();
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
            recorderViewLoader.item.isRecording = (VoiceRecoderHandler.getRecoderType() === VoiceRecoderHandler.Recording);
        }
        onUpdateRecordBtnState: {
            title.recorderBtnEnable = enable;
        }
        onUpdateRecorderTime: {
            recorderViewLoader.item.time = time;
        }
    }
}
