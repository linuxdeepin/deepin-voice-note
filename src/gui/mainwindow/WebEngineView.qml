// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtWebChannel 1.15
import QtWebEngine 1.15

import VNote 1.0

Item {
    visible: true

    WebEngineView {
        id: webView
        anchors.fill: parent

        WebChannel {
            id: noteWebChannel
        }

        Component.onCompleted: {
            noteWebChannel.registerObject("webobj", Webobj);
            // console.log("registerObject ret: " + ret)
            webView.webChannel = noteWebChannel
            webView.url = Qt.resolvedUrl(Webobj.webPath())
        }
    }
}
