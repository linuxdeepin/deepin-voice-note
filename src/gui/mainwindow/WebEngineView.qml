// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls
import QtWebChannel 1.15
import QtWebEngine 1.15
import VNote 1.0

Item {
    visible: true

    WebEngineView {
        id: webView

        anchors.fill: parent

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

        WebEngineHandler {
            id: handler

        }

        WebChannel {
            id: noteWebChannel

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
}
