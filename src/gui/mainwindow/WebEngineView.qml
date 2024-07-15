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

        WebEngineHandler {
        }

        WebChannel {
            id: noteWebChannel

        }
    }
}
