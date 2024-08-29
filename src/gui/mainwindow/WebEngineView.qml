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

Item {
    visible: true

    ColumnLayout {
        id: columnLayout

        anchors.fill: parent
        WindowTitleBar {
            id: title
            Layout.fillWidth: true
            height: 40
        }

        WebEngineView {
            id: webView

            Layout.fillHeight: true

            // anchors.fill: parent
            Layout.fillWidth: true

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

            }

            WebChannel {
                id: noteWebChannel

            }
        }

        Loader {
            id: multipleChoicesLoader
            Layout.fillWidth: parent.width
            Layout.fillHeight: parent.height
            visible: false

            sourceComponent: MultipleChoices {
                anchors.fill: parent
            }
        }
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
            nameFilters: ["Image file(*.jpg *.png *.bmp)"]
            folder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
            Component.onCompleted: {
                fileDialog.open()
            }
            onAccepted: {
                if (fileDialog.files.length > 0) {
                    Webobj.callJsInsertImages(fileDialog.files);
                }
            }
        }
    }

    Connections {
        target: title

        onCreateNote: {
            VNoteMainManager.createNote()
        }
        onStartRecording: {
            console.warn("--------------")
        }
        onInsertImage: {
            if (!selectImgLoader.active) {
                selectImgLoader.active = true
            } else {
                selectImgLoader.item.open()
            }
        }
    }

    function toggleMultCho(choices) {
        if (choices > 1) {
            webView.visible = false
            if (!multipleChoicesLoader.active) {
                multipleChoicesLoader.active = true
            }
            multipleChoicesLoader.visible = true
            multipleChoicesLoader.item.visible = true
            multipleChoicesLoader.item.selectSize = choices
        } else {
            multipleChoicesLoader.visible = false
            webView.visible = true
            multipleChoicesLoader.item.visible = false
        }
    }
}
